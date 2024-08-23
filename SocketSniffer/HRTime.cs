using System;
using System.Runtime.InteropServices;

namespace SocketSniffer
{
    internal class HRTime
    {
        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.Winapi)]
        private static extern void GetSystemTimePreciseAsFileTime(out long filetime);

        public static DateTime Now
        {
            get
            {
                GetSystemTimePreciseAsFileTime(out long filetime);

                return DateTime.FromFileTime(filetime);
            }
        }
    }
}
