using System;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Threading;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace SocketSniffer
{
    internal class Program
    {
        private static Socket MainSocket = null;
        private static SocketError Error = SocketError.Success;
        private static readonly byte[] ByteData = new byte[9000];
        private static bool ContinueCapturing = true;
        private static bool UnableToContinueCapturing = false;
        private static int PacketCounter = 0;
        private static int FailedPacketCounter = 0;
        private static readonly ConcurrentQueue<Packet> PacketBuffer = new();

        private static readonly IEnumerable<IPAddress> AvailableAddrs = NetInterface.GetAddrs();
        private static IPAddress BindAddr;
        private static FileStream OutputFileStream;
        private static StreamWriter OutputFileStreamWriter;
        private static Task WriteTask;
        private static Timer PrintTimer;

        private const string USAGE_MSG = "SocketSniffer.exe [-i <BindingAddress>] [-o <OutputFile>]";

        private static bool ProcessArgs(string[] args)
        {
            try
            {
                switch (args.Length)
                {
                    case 0:
                        SetDefaultArgs();
                        break;
                    case 2:
                        SetDefaultArgs();
                        ProcessSubArgs(args[0], args[1]);
                        break;
                    case 4:
                        ProcessSubArgs(args[0], args[1]);
                        ProcessSubArgs(args[2], args[3]);
                        break;
                    default:
                        Console.WriteLine(USAGE_MSG);
                        return false;
                }
                if (BindAddr == null)
                {
                    Console.WriteLine("Invalid IP address or no IP address available");
                    Console.WriteLine("Available addresses:");
                    foreach (var addr in AvailableAddrs)
                    {
                        Console.WriteLine($"    {addr}");
                    }
                    return false;
                }
            }
            catch (ArgumentException e)
            {
                Console.WriteLine($"Invalid Argument: {e.Message}");
                return false;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return false;
            }
            return true;
        }

        private static void ProcessSubArgs(string key, string value)
        {
            switch (key)
            {
                case "-i":
                    BindAddr = AvailableAddrs.Where(a => a.ToString() == value).FirstOrDefault();
                    break;
                case "-o":
                    OutputFileStream = new(value, FileMode.OpenOrCreate, FileAccess.Write, FileShare.Read);
                    OutputFileStreamWriter = new(OutputFileStream);
                    break;
                default:
                    throw new ArgumentException(key);
            }
        }

        private static void SetDefaultArgs()
        {
            OutputFileStream ??= new(".\\sockdump.txt", FileMode.OpenOrCreate, FileAccess.Write, FileShare.Read);
            OutputFileStreamWriter = new(OutputFileStream);
            // Although IPv6 capture appears to be working, it's not receiving valid data, so default is still IPv4 address
            BindAddr ??= AvailableAddrs.Where(a => a.AddressFamily == AddressFamily.InterNetwork).FirstOrDefault();
        }

        static void Main(string[] args)
        {
            if (!ProcessArgs(args))
            {
                CleanUp();
                Environment.Exit(87);
            }

            InitSocket();
            PrintTimer = new(PrintCounter, null, 1000, 1000);
            WriteTask = Task.Run(WriteDump);
            Console.WriteLine("Press 'X' to quit");

            while (true)
            {
                if (Console.KeyAvailable)
                {
                    ConsoleKeyInfo key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.X) break;
                }
                if (UnableToContinueCapturing)
                {
                    Console.WriteLine();
                    Console.WriteLine("Fatal error occurred, exiting...");
                }
                Thread.Sleep(100);
            }
            ContinueCapturing = false;
            Console.WriteLine();
            Console.WriteLine("Waiting for all packets written to file...");
            WriteTask.Wait();
            MainSocket.Close();
            OutputFileStreamWriter.WriteLine();
            OutputFileStreamWriter.Close();
            OutputFileStream.Close();
            CleanUp();
        }

        private static void CleanUp()
        {
            PrintTimer?.Dispose();
            MainSocket?.Dispose();
            OutputFileStreamWriter?.Dispose();
            OutputFileStream?.Dispose();
        }

        private static void InitSocket()
        {
            Console.WriteLine($"Binding address {BindAddr}");
            MainSocket = new Socket(BindAddr.AddressFamily,
                                            SocketType.Raw, ProtocolType.IP);
            //Bind the socket to the selected IP address.
            MainSocket.Bind(new IPEndPoint(BindAddr, 0));
            //Set the socket options.
            MainSocket.SetSocketOption(BindAddr.AddressFamily == AddressFamily.InterNetwork ? SocketOptionLevel.IP : SocketOptionLevel.IPv6,
                                       SocketOptionName.HeaderIncluded, true);
            byte[] byTrue = [1, 0, 0, 0];
            // Capture outgoing packets.
            byte[] byOut = [1, 0, 0, 0];
            // Socket.IOControl is analogous to the WSAIoctl method of Winsock 2.
            MainSocket.IOControl(IOControlCode.ReceiveAll, byTrue, byOut);
            // Start receiving the packets asynchronously.
            MainSocket.BeginReceive(ByteData, 0, ByteData.Length,
                                    SocketFlags.None, out Error,
                                    new AsyncCallback(OnReceive), null);
        }

        private static void OnReceive(IAsyncResult asyncResult)
        {
            try
            {
                int nReceived = MainSocket.EndReceive(asyncResult);
                PacketCounter++;
                PacketBuffer.Enqueue(new(ByteData, nReceived));
            }
            catch (ObjectDisposedException) { }
            catch (SocketException)
            {
                // Somehow there can be UDP packets larger than 9000 bytes, not sure if related to URO🧐
                if (Error == SocketError.NoBufferSpaceAvailable)
                {
                    FailedPacketCounter++;
                }
            }
            catch (Exception exception)
            {
                Console.WriteLine();
                Console.WriteLine(exception.ToString());
                UnableToContinueCapturing = true;
            }
            finally
            {
                if (ContinueCapturing)
                {
                    MainSocket.BeginReceive(ByteData, 0, ByteData.Length,
                                            SocketFlags.None,
                                            new AsyncCallback(OnReceive), null);
                }
            }
        }

        private static void WriteDump()
        {
            while (ContinueCapturing || PacketBuffer.Count != 0)
            {
                if (PacketBuffer.TryDequeue(out var pkt))
                {
                    var sb = new StringBuilder();
                    sb.Append("> ")
                        .Append(pkt.Time.ToString("yyyy-MM-dd HH:mm:ss.fffffff"))
                        .Append(' ')
                        .Append(pkt.Data.Select(b => b.ToString("X2")).Aggregate((a, b) => a + b));
                    OutputFileStreamWriter.WriteLine(sb.ToString());
                }
                else
                {
                    Thread.Sleep(50);
                }
            }
        }

        private static int LastPrintLen = 0;
        private static void PrintCounter(object state)
        {
            var lineMax = Console.BufferWidth;
            var lineBack = LastPrintLen / (lineMax + 1);
            var message = string.Format("Captured {0} packets, saved {1} packets, pending {2} packets, unable to save {3} packets",
                PacketCounter, PacketCounter - PacketBuffer.Count, PacketBuffer.Count, FailedPacketCounter)
                .PadRight(lineMax * (lineBack + 1));
            LastPrintLen = message.Length;
            Console.SetCursorPosition(0, Console.CursorTop - lineBack);
            Console.Write(message);
        }
    }
}
