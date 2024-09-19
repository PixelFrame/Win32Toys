using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace fastdel
{
    internal class Program
    {
        static bool Logging = false;
        static void Main(string[] args)
        {
            DateTime start = DateTime.Now;
            if (args.Length == 0 || args.Length > 2) { Environment.Exit(87); }
            if (args.Length == 2 && args[1] == "-l") Logging = true;

            var files = Directory.EnumerateFiles(args[0]);
            var tasks = new List<Task>();
            foreach (var file in files)
            {
                tasks.Add(Task.Factory.StartNew(() => FastDelete(file)));   // Let ThreadPool schedule jobs
            }
            Task.WaitAll(tasks.ToArray());

            DateTime end = DateTime.Now;
            Console.WriteLine($"Done, time spent {end - start}");
#if DEBUG
            Console.ReadKey();
#endif
        }

        // Delete the file by setting FILE_FLAG_DELETE_ON_CLOSE at CreateFile
        static void FastDelete(string path)
        {
            DateTime start = DateTime.Now;
            var hFile = PInvoke.CreateFile(path, 
                0x40000000, // GENERIC_WRITE
                0,          // Non-sharing
                IntPtr.Zero,
                3,          // OPEN_EXISTING
                0x04000000, // FILE_FLAG_DELETE_ON_CLOSE
                IntPtr.Zero);
            if (hFile.ToInt32() != -1)
            {
                PInvoke.CloseHandle(hFile);
                DateTime end = DateTime.Now;
                if (Logging)
                {
                    Console.WriteLine($"{path}, {end - start}");
                }
            }
            else if (Logging)
            {
                Console.WriteLine($"{path}, failed with 0x{Marshal.GetLastWin32Error():X}");
            }
        }
    }

    internal static class PInvoke
    {
        [DllImport("Kernel32.dll", SetLastError = true)]
        public static extern IntPtr CreateFile(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool CloseHandle(IntPtr hObject);
    }
}
