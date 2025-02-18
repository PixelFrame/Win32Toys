using System;
using System.Collections.Generic;
using System.ComponentModel;
using Windows.Win32;
using Windows.Win32.Foundation;

namespace WFPcmd
{
    internal class Program
    {
        const string USAGE = """
            WFP command line tool.
            To check detailed help info, use help commands:

                WFPcmd.exe help filter
                WFPcmd.exe help layer
                WFPcmd.exe help guid
            """;

        const string USAGE_GUID = """
            WFPcmd.exe guid -l                      // Print all layer GUIDs
                       guid -s                      // Print all built-in sublayer GUIDs
                       guid -c                      // Print all built-in callout GUIDs
                       guid -f                      // Print all condition field GUIDs
            """;

        const string USAGE_FILTER = """
            WFPcmd.exe filter list   [-v]           // List all filters, -v for verbose
                                     -j             // JSON output
            WFPcmd.exe filter query  -i <filterId>  // Query filter by ID
                                     -k <filterKey> // Query filter by key
                                     -n <name>      // Search for filters with name containing <name>
                                     -m <regex>     // Search for filters with name matching <regex>
                                     -l <layer>     // Search for filters in layer <layer>

            WFPcmd.exe filter sdqry  -k <filterKey> // Query filter security descriptor
            WFPcmd.exe filter sdset  -k <filterKey> -v <sddl>
                                                    // Set filter security descriptor
            
            WFPcmd.exe filter remove -i <filterId>  // Remove filter by ID
                                     -k <filterKey> // Remove filter by key
            WFPcmd.exe filter add    -n <name> [-d <description>] -l <layer> -a <action> [-w <weight>] [-c <field> <match> <value>]
                                                    // layer and field can be GUID or symbolic name
                                                    // action only supports 'block' or 'permit'
                                                    // multiple conditions can be added with -c
                                                    // condition value format <TYPE>!<VALUE>
                                                    // number values support DEC and HEX (0x) format, e.g. UINT32!0x1234 or UINT64!1234
                                                    // binary values should be in BASE64 format, e.g. BYTE_BLOB:bG9yZW0=
                                                    // security descriptor values should be in SDDL format, e.g. SD!D:(A;;CCRC;;;WD)
                                                    // token information and token access information are currently not implemented
            """;

        const string USAGE_LAYER = """
            WFPcmd.exe layer list  [-v]             // List all layers
            
            WFPcmd.exe layer query -k <layerKey>    // Query layer by key
                                   -n <name>        // Search for layers with name containing <name>
                                   -m <regex>       // Search for layers with name matching <regex>
            """;

        const string USAGE_CALLOUT = """
            WFPcmd.exe callout list   [-v]            // List all callouts

            WFPcmd.exe callout query  -k <calloutKey> // Query callout by key
                                      -n <name>       // Search for callouts with name containing <name>
                                      -m <regex>      // Search for callouts with name containing <name>

            WFPcmd.exe callout remove -k <calloutKey> // Remove callout by key
            """;

        static unsafe void Main(string[] args)
        {
            Helper.EnableProcessSecurityPrivilege();
            uint dwRet;
            HANDLE engineHandle = HANDLE.Null;
            try
            {
                Helper.GenerateTables();

                dwRet = PInvoke.FwpmEngineOpen0(null, PInvoke.RPC_C_AUTHN_WINNT, null, null, &engineHandle);
                if (dwRet != 0)
                {
                    throw new Win32Exception((int)dwRet, "FwpmEngineOpen0");
                }
                ProcessArgs(args, engineHandle);
            }
            catch (Win32Exception e)
            {
                Console.WriteLine($"{e.Message} failed with error code: 0x{e.NativeErrorCode:X8}");
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
            finally
            {
                if (engineHandle != HANDLE.Null)
                {
                    // Actually unnecessary to close the session manually according to the documentation
                    PInvoke.FwpmEngineClose0(engineHandle);
                }
            }
        }

        static void ProcessArgs(string[] args, HANDLE engineHandle)
        {
            if (args.Length < 2)
            {
                Console.WriteLine(USAGE);
                return;
            }
            switch (args[0].ToLower())
            {
                case "help":
                    switch (args[1].ToLower())
                    {
                        case "guid": Console.WriteLine(USAGE_GUID); break;
                        case "layer": Console.WriteLine(USAGE_LAYER); break;
                        case "filter": Console.WriteLine(USAGE_FILTER); break;
                        case "callout": Console.WriteLine(USAGE_CALLOUT); break;
                    }
                    break;
                case "filter":
                    ProcessFilterArgs(args.AsSpan().Slice(1).ToArray(), engineHandle);
                    break;
                case "layer":
                    ProcessLayerArgs(args.AsSpan().Slice(1).ToArray(), engineHandle);
                    break;
                case "callout":
                    ProcessCalloutArgs(args.AsSpan().Slice(1).ToArray(), engineHandle);
                    break;
                case "guid":
                    if (args.Length != 2)
                    {
                        Console.WriteLine(USAGE);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-l":
                            Helper.PrintTable(1);
                            break;
                        case "-s":
                            Helper.PrintTable(2);
                            break;
                        case "-c":
                            Helper.PrintTable(3);
                            break;
                        case "-f":
                            Helper.PrintTable(0);
                            break;
                        default:
                            Console.WriteLine(USAGE);
                            break;
                    }
                    break;
                default:
                    Console.WriteLine(USAGE);
                    break;
            }
        }

        static void ProcessLayerArgs(string[] args, HANDLE engineHandle)
        {
            if (args.Length == 0)
            {
                Console.WriteLine(USAGE_LAYER);
                return;
            }
            switch (args[0].ToLower())
            {
                case "list":
                    if (args.Length == 2)
                    {
                        if (args[1] == "-v")
                        {
                            LayerCtrl.PrintLayers(engineHandle, true);
                        }
                        else
                        {
                            Console.WriteLine(USAGE_LAYER);
                        }
                    }
                    else
                    {
                        LayerCtrl.PrintLayers(engineHandle);
                    }
                    break;
                case "query":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE_LAYER);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-k":
                            LayerCtrl.QueryLayerByKey(engineHandle, args[2]);
                            break;
                        case "-n":
                            LayerCtrl.QueryLayerByName(engineHandle, args[2], false);
                            break;
                        case "-m":
                            LayerCtrl.QueryLayerByName(engineHandle, args[2], true);
                            break;
                        default:
                            Console.WriteLine(USAGE_LAYER);
                            break;
                    }
                    break;
                default:
                    Console.WriteLine(USAGE_LAYER);
                    break;
            }
        }

        static void ProcessCalloutArgs(string[] args, HANDLE engineHandle)
        {
            if (args.Length == 0)
            {
                Console.WriteLine(USAGE_CALLOUT);
                return;
            }
            switch (args[0].ToLower())
            {
                case "list":
                    if (args.Length == 2)
                    {
                        if (args[1] == "-v")
                        {
                            CalloutCtrl.PrintCallouts(engineHandle, true);
                        }
                        else
                        {
                            Console.WriteLine(USAGE_CALLOUT);
                        }
                    }
                    else
                    {
                        CalloutCtrl.PrintCallouts(engineHandle);
                    }
                    break;
                case "query":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE_CALLOUT);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-k":
                            CalloutCtrl.QueryCalloutByKey(engineHandle, args[2]);
                            break;
                        case "-n":
                            CalloutCtrl.QueryCalloutByName(engineHandle, args[2], false);
                            break;
                        case "-m":
                            CalloutCtrl.QueryCalloutByName(engineHandle, args[2], true);
                            break;
                        default:
                            Console.WriteLine(USAGE_CALLOUT);
                            break;
                    }
                    break;
                case "remove":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE_CALLOUT);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-k":
                            CalloutCtrl.RemoveCalloutByKey(engineHandle, Guid.Parse(args[2]));
                            break;
                        default:
                            Console.WriteLine(USAGE_CALLOUT);
                            break;
                    }
                    break;
                default:
                    Console.WriteLine(USAGE_CALLOUT);
                    break;
            }
        }

        static void ProcessFilterArgs(string[] args, HANDLE engineHandle)
        {
            if (args.Length == 0)
            {
                Console.WriteLine(USAGE_FILTER);
                return;
            }
            switch (args[0].ToLower())
            {
                case "list":
                    if (args.Length == 2)
                    {
                        if (args[1] == "-v")
                        {
                            FilterCtrl.PrintFilters(engineHandle, true);
                        }
                        else if (args[1] == "-j")
                        {
                            FilterCtrl.PrintFilters(engineHandle, false, true);
                        }
                        else
                        {
                            Console.WriteLine(USAGE_FILTER);
                        }
                    }
                    else
                    {
                        FilterCtrl.PrintFilters(engineHandle);
                    }
                    break;
                case "query":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE_FILTER);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-i":
                            FilterCtrl.QueryFilterById(engineHandle, ulong.Parse(args[2]));
                            break;
                        case "-k":
                            FilterCtrl.QueryFilterByKey(engineHandle, Guid.Parse(args[2]));
                            break;
                        case "-n":
                            FilterCtrl.QueryFilterByName(engineHandle, args[2], false);
                            break;
                        case "-m":
                            FilterCtrl.QueryFilterByName(engineHandle, args[2], true);
                            break;
                        case "-l":
                            FilterCtrl.QueryFilterByLayer(engineHandle, args[2]);
                            break;
                        default:
                            Console.WriteLine(USAGE_FILTER);
                            break;
                    }
                    break;
                case "sdqry":
                    if (args.Length != 3 || args[1] != "-k")
                    {
                        Console.WriteLine(USAGE_FILTER);
                        return;
                    }
                    FilterCtrl.QueryFilterSdByKey(engineHandle, Guid.Parse(args[2]));
                    break;
                case "sdset":
                    if (args.Length != 5 || args[1] != "-k" || args[3] != "-v")
                    {
                        Console.WriteLine(USAGE_FILTER);
                        return;
                    }
                    FilterCtrl.SetFilterSdByKey(engineHandle, Guid.Parse(args[2]), args[4]);
                    break;
                case "remove":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE_FILTER);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-i":
                            FilterCtrl.RemoveFilterById(engineHandle, ulong.Parse(args[2]));
                            break;
                        case "-k":
                            FilterCtrl.RemoveFilterByKey(engineHandle, Guid.Parse(args[2]));
                            break;
                        default:
                            Console.WriteLine(USAGE_FILTER);
                            break;
                    }
                    break;
                case "add":
                    ProcessFilterAddArgs(args, engineHandle);
                    break;
                default:
                    Console.WriteLine(USAGE_FILTER);
                    break;
            }
        }

        static void ProcessFilterAddArgs(string[] args, HANDLE engineHandle)
        {
            var name = "";
            var description = "";
            var layer = "";
            var action = "";
            var weight = 0ul;
            var conditions = new List<(string, string, string)>();
            for (int i = 1; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "-n":
                        name = args[++i];
                        break;
                    case "-d":
                        description = args[++i];
                        break;
                    case "-l":
                        layer = args[++i];
                        break;
                    case "-a":
                        action = args[++i];
                        break;
                    case "-w":
                        weight = ulong.Parse(args[++i]);
                        break;
                    case "-c":
                        conditions.Add((args[++i], args[++i], args[++i]));
                        break;
                    default:
                        Console.WriteLine(USAGE_FILTER);
                        return;
                }
            }
            if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(layer) || string.IsNullOrEmpty(action))
            {
                Console.WriteLine(USAGE_FILTER);
                return;
            }
            FilterCtrl.AddFilter(engineHandle, name, description, layer, action, conditions, weight);
        }
    }
}