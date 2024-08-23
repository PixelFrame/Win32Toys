using System;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Threading;
using System.Collections.Concurrent;

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

        private static IPAddress BindAddr;
        private static StreamWriter OutputFileStreamWriter;
        private static Task WriteTask;
        private static Timer PrintTimer;

        private const string USAGE_MSG = "SocketSniffer.exe <BindingAddress> <OutputFile>";

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine(USAGE_MSG);
                return;
            }
            if (!IPAddress.TryParse(args[0], out BindAddr))
            {
                Console.WriteLine("Invalid address");
                Console.WriteLine(USAGE_MSG);
                return;
            }

            OutputFileStreamWriter = new StreamWriter(args[1]);

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

            PrintTimer.Dispose();
            MainSocket.Dispose();
            OutputFileStreamWriter.Dispose();
        }

        private static void InitSocket()
        {
            MainSocket = new Socket(AddressFamily.InterNetwork,
                                            SocketType.Raw, ProtocolType.IP);
            //Bind the socket to the selected IP address.
            MainSocket.Bind(new IPEndPoint(BindAddr, 0));
            //Set the socket options.
            MainSocket.SetSocketOption(SocketOptionLevel.IP,
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

        private static void PrintCounter(object state)
        {
            Console.Write("\rCaptured {0} packets, saved {1} packets, pending {2} packets, unable to save {3} packets                             ",
                PacketCounter, PacketCounter - PacketBuffer.Count, PacketBuffer.Count, FailedPacketCounter);
        }
    }
}
