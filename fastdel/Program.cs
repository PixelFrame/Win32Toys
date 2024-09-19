using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace fastdel
{
    internal class Program
    {
        const string HELP_MSG =
@"fastdel -d <value> [-v]
fastdel -f <value> [-v]
fastdel -l <value> [-v]

    -d Directory
    -f Text file containing the file list to be deleted
    -l Comma splitted file list to be deleted
    -v Verbose logging
";
        static int Mode = 0;
        static bool Logging = false;
        static void Main(string[] args)
        {
            if (args.Length < 2 || args.Length > 3) { Console.Write(HELP_MSG); Environment.Exit(87); }
            if (args[0] == "-d") Mode = 1;
            if (args[0] == "-f") Mode = 2;
            if (args[0] == "-l") Mode = 3;
            if (Mode == 0) { Console.Write(HELP_MSG); Environment.Exit(87); }
            if (args.Length == 3 && args[2] == "-v") Logging = true;

            IEnumerable<string> files = Array.Empty<string>();
            switch(Mode)
            {
                case 1:
                    files = Directory.EnumerateFiles(args[1]); break;
                case 2:
                    files = File.ReadAllLines(args[1]); break;
                case 3:
                    files = args[1].Split(','); break;
            }
            FastDeleteFiles(files);
#if DEBUG
            Console.ReadKey();
#endif
        }

        static void FastDeleteFiles(IEnumerable<string> files)
        {
            var start = DateTime.Now;

            var tasks = new List<Task>();
            foreach (var file in files)
            {
                tasks.Add(Task.Factory.StartNew(() => FastDelete(file)));   // Let ThreadPool schedule jobs
            }
            Task.WaitAll(tasks.ToArray());

            var end = DateTime.Now;
            Console.WriteLine($"Done, time spent {end - start}");
        }

        // Delete the file by setting FILE_FLAG_DELETE_ON_CLOSE at CreateFile
        static void FastDelete(string path)
        {
            var start = DateTime.Now;
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
                var end = DateTime.Now;
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
