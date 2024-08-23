using System;

namespace SocketSniffer
{
    internal class Packet
    {
        public DateTime Time { get; }
        public byte[] Data { get; }

        public Packet(byte[] Buffer, int Length) 
        {
            Time = HRTime.Now;
            Data = new byte[Length];
            Array.Copy(Buffer, Data, Length);
        }
    }
}
