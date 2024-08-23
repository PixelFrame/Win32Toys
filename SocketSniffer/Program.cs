using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace SocketSniffer
{
    internal class Program
    {
        private static Socket MainSocket = null;
        private static readonly byte[] ByteData = new byte[9000];
        private static bool ContinueCapturing = true;

        private static IPAddress BindAddr;
        private static StreamWriter OutputFileStreamWriter;

        private const string USAGE_MSG = "SocketSniffer.exe <BindingAddress> <OutputFile>";

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine(USAGE_MSG);
                return;
            }
            if(!IPAddress.TryParse(args[0], out BindAddr))
            {
                Console.WriteLine("Invalid address");
                Console.WriteLine(USAGE_MSG);
                return;
            }

            OutputFileStreamWriter = new StreamWriter(args[1]);

            Init();
            while (true)
            {
                Console.WriteLine("Press 'X' to quit");
                var cki = Console.ReadKey(true);

                // Exit if the user pressed the 'X' key.
                if (cki.Key == ConsoleKey.X) break;
            }
            ContinueCapturing = false;
            MainSocket.Close();
            OutputFileStreamWriter.WriteLine();
            OutputFileStreamWriter.Close();

            MainSocket.Dispose();
            OutputFileStreamWriter.Dispose();
        }

        private static void Init()
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
                                    SocketFlags.None,
                                    new AsyncCallback(OnReceive), null);
        }

        private static void OnReceive(IAsyncResult asyncResult)
        {
            try
            {
                int nReceived = MainSocket.EndReceive(asyncResult);
                WriteDump(nReceived);
                if (ContinueCapturing)
                {
                    MainSocket.BeginReceive(ByteData, 0, ByteData.Length,
                                            SocketFlags.None,
                                            new AsyncCallback(OnReceive), null);
                }
            }
            catch (ObjectDisposedException) { }
            catch (Exception exception)
            {
                Console.WriteLine(exception.ToString());
            }
        }

        private static void WriteDump(int recvLen)
        {
            Console.WriteLine("Writing dump file");
            var sb = new StringBuilder();
            sb.Append("> ")
                .Append(HRTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fffffff"))
                .Append(' ')
                .Append(ByteData.Take(recvLen).Select(b => b.ToString("X2")).Aggregate((a, b) => a + b));
            OutputFileStreamWriter.WriteLine(sb.ToString());
        }
    }
}
