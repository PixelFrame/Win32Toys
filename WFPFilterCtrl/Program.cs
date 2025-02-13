using Microsoft.Win32.SafeHandles;
using Microsoft.Windows.SDK.Win32Docs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.NetworkManagement.WindowsFilteringPlatform;
using Windows.Win32.Security;

namespace WFPFilterCtrl
{
    internal class Program
    {
        const string USAGE = """
            WFPFilterCtrl.exe list [-v]
            WFPFilterCtrl.exe query <filterId>
            WFPFilterCtrl.exe remove <filterId>
            WFPFilterCtrl.exe add -n <name> [-d <description>] -l <layer> -a <action> [-w <weight>] [-c <field> <match> <value>]
                              action only supports 'block' or 'permit'
                              multiple conditions can be added with -c
                              layer and field can be GUID or symbolic name
            """;

        static unsafe void Main(string[] args)
        {
            uint dwRet;
            HANDLE engineHandle;

            Helper.GenerateTables();

            dwRet = PInvoke.FwpmEngineOpen0(null, PInvoke.RPC_C_AUTHN_WINNT, null, null, &engineHandle);
            if (dwRet != 0)
            {
                Console.WriteLine($"FwpmEngineOpen0 failed with error code: 0x{dwRet:X8}");
                return;
            }

            ParseArgs(args, engineHandle);

            dwRet = PInvoke.FwpmEngineClose0(engineHandle);
            if (dwRet != 0)
            {
                Console.WriteLine($"FwpmEngineClose0 failed with error code: 0x{dwRet:X8}");
                return;
            }
        }

        static void ParseArgs(string[] args, HANDLE engineHandle)
        {
            if (args.Length == 0)
            {
                Console.WriteLine(USAGE);
                return;
            }
            switch (args[0])
            {
                case "list":
                    if (args.Length == 2 && args[1] == "-v")
                    {
                        PrintFilters(engineHandle, true);
                    }
                    else
                    {
                        PrintFilters(engineHandle);
                    }
                    break;
                case "query":
                    if (args.Length != 2)
                    {
                        Console.WriteLine(USAGE);
                        return;
                    }
                    QueryFilterById(engineHandle, ulong.Parse(args[1]));
                    break;
                case "remove":
                    if (args.Length != 2)
                    {
                        Console.WriteLine(USAGE);
                        return;
                    }
                    RemoveFilterById(engineHandle, ulong.Parse(args[1]));
                    break;
                case "add":
                    ProcessAddArgs(args, engineHandle);
                    break;
                default:
                    Console.WriteLine(USAGE);
                    break;
            }
        }

        static void ProcessAddArgs(string[] args, HANDLE engineHandle)
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
                        Console.WriteLine(USAGE);
                        return;
                }
            }
            if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(layer) || string.IsNullOrEmpty(action))
            {
                Console.WriteLine(USAGE);
                return;
            }
            AddFilter(engineHandle, name, description, layer, action, conditions, weight);
        }

        static unsafe void PrintFilters(HANDLE engineHandle, bool verbose = false)
        {
            uint dwRet;
            HANDLE enumHandle;
            FWPM_FILTER0** filters = null;
            uint numEntriesReturned;

            dwRet = PInvoke.FwpmFilterCreateEnumHandle0(engineHandle, null, &enumHandle);
            if (dwRet != 0)
            {
                Console.WriteLine($"FwpmFilterCreateEnumHandle0 failed with error code: 0x{dwRet:X8}");
                return;
            }

            dwRet = PInvoke.FwpmFilterEnum0(engineHandle, enumHandle, uint.MaxValue, &filters, &numEntriesReturned);
            if (dwRet != 0)
            {
                PInvoke.FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
                Console.WriteLine($"FwpmFilterEnum0 failed with error code: 0x{dwRet:X8}");
                return;
            }

            for (uint i = 0; i < numEntriesReturned; i++)
            {
                if (verbose)
                {
                    PrintFilterVerbose(filters[i]);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else
                {
                    Console.WriteLine($"{filters[i]->filterId,-6} {filters[i]->filterKey} {filters[i]->action.type,-30} {filters[i]->displayData.name} ");
                }
            }
            PInvoke.FwpmFreeMemory0((void**)&filters);
            PInvoke.FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
        }

        static unsafe void PrintFilterVerbose(FWPM_FILTER0* filter)
        {
            Console.WriteLine($"ID:               {filter->filterId}");
            Console.WriteLine($"Key:              {filter->filterKey}");
            Console.WriteLine($"Name:             {filter->displayData.name}");
            Console.WriteLine($"Description:      {filter->displayData.description}");
            Console.WriteLine($"Action:           {filter->action.type.ToString().Substring(11)}");
            if (filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_TERMINATING
                || filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_UNKNOWN
                || filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_INSPECTION)
            {
                Console.WriteLine($"Callout:          {Helper.TranslateCalloutGuid(filter->action.Anonymous.calloutKey)}");
            }
            Console.WriteLine($"Layer:            {Helper.TranslateLayerGuid(filter->layerKey)}");
            Console.WriteLine($"SubLayer:         {Helper.TranslateSubLayerGuid(filter->subLayerKey)}");
            Console.WriteLine($"Effective Weight: {Helper.TranslateValue(filter->effectiveWeight)}");
            if (filter->numFilterConditions > 0)
            {
                Console.WriteLine("Conditions:");
                for (uint j = 0; j < filter->numFilterConditions; j++)
                {
                    Console.WriteLine($"  {Helper.TranslateConditionGuid(filter->filterCondition[j].fieldKey),-30} {filter->filterCondition[j].matchType.ToString().Substring(10),-22} {Helper.TranslateValue(filter->filterCondition[j].conditionValue)}");
                }
            }
        }

        static unsafe void QueryFilterById(HANDLE engineHandle, ulong id)
        {
            FWPM_FILTER0* filter = null;
            uint dwRet = PInvoke.FwpmFilterGetById0(engineHandle, id, &filter);
            if (dwRet != 0)
            {
                Console.WriteLine($"FwpmFilterGetById0 failed with error code: 0x{dwRet:X8}");
                return;
            }

            PrintFilterVerbose(filter);
        }

        static void RemoveFilterById(HANDLE engineHandle, ulong id)
        {
            var dwRet = PInvoke.FwpmFilterDeleteById0(engineHandle, id);
            if (dwRet != 0)
            {
                Console.WriteLine($"FwpmFilterDeleteById0 failed with error code: 0x{dwRet:X8}");
            }
            else
            {
                Console.WriteLine($"Filter with ID {id} removed successfully");
            }
        }

        static unsafe void AddFilter(HANDLE engineHandle, string name, string description, string layer, string action, IEnumerable<(string, string, string)> conditions, ulong weight)
        {
            ulong filterId;
            var actionType = action.ToLower() switch
            {
                "block" => FWP_ACTION_TYPE.FWP_ACTION_BLOCK,
                "permit" => FWP_ACTION_TYPE.FWP_ACTION_PERMIT,
                _ => throw new NotSupportedException("Action not supported")
            };
            fixed (char* pName = name)
            {
                fixed (char* pDescription = description)
                {
                    var filter = new FWPM_FILTER0
                    {
                        displayData = new FWPM_DISPLAY_DATA0
                        {
                            name = pName,
                            description = pDescription
                        },
                        layerKey = Helper.GetLayerGuid(layer),
                        action = new FWPM_ACTION0
                        {
                            type = actionType,
                        },
                        weight = new FWP_VALUE0
                        {
                            type = FWP_DATA_TYPE.FWP_UINT64,
                            Anonymous = new FWP_VALUE0._Anonymous_e__Union
                            {
                                uint64 = &weight
                            }
                        },
                    };
                    // TODO: Parse conditions...
                    var dwRet = PInvoke.FwpmFilterAdd0(engineHandle, &filter, (PSECURITY_DESCRIPTOR)null, &filterId);
                    if (dwRet != 0)
                    {
                        Console.WriteLine($"FwpmFilterAdd0 failed with error code: 0x{dwRet:X8}");
                    }
                }
            }
            Console.WriteLine($"Filter added successfully");
            Console.WriteLine();
            QueryFilterById(engineHandle, filterId);
        }
    }
}
