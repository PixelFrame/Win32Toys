using Microsoft.Win32.SafeHandles;
using Microsoft.Windows.SDK.Win32Docs;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
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
            WFPFilterCtrl.exe list   [-v]           // List all filters, -v for verbose

            WFPFilterCtrl.exe guid   -l             // Print all layer GUIDs
                              guid   -s             // Print all built-in sublayer GUIDs
                              guid   -c             // Print all built-in callout GUIDs
                              guid   -f             // Print all condition field GUIDs

            WFPFilterCtrl.exe query  -i <filterId>  // Query filter by ID
                                     -k <filterKey> // Query filter by key
                                     -n <name>      // Search for filters with name containing <name>
                                     -m <regex>     // Search for filters with name matching <regex>
                                     -l <layer>     // Search for filters in layer <layer>

            WFPFilterCtrl.exe remove -i <filterId>  // Remove filter by ID
                                     -k <filterKey> // Remove filter by key

            WFPFilterCtrl.exe add    -n <name> [-d <description>] -l <layer> -a <action> [-w <weight>] [-c <field> <match> <value>]
                                                    // layer and field can be GUID or symbolic name
                                                    // action only supports 'block' or 'permit'
                                                    // multiple conditions can be added with -c
                                                    // condition value format <TYPE>!<VALUE>
                                                    // number values support DEC and HEX (0x) format, e.g. UINT32!0x1234 or UINT64!1234
                                                    // binary values should be in BASE64 format, e.g. BYTE_BLOB:bG9yZW0=
                                                    // security descriptor values should be in SDDL format, e.g. SD!D:(A;;CCRC;;;WD)
                                                    // token information and token access information are currently not implemented
            """;

        static unsafe void Main(string[] args)
        {
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
                ParseArgs(args, engineHandle);
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
                case "query":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-i":
                            QueryFilterById(engineHandle, ulong.Parse(args[2]));
                            break;
                        case "-k":
                            QueryFilterByKey(engineHandle, Guid.Parse(args[2]));
                            break;
                        case "-n":
                            QueryFilterByName(engineHandle, args[2], false);
                            break;
                        case "-m":
                            QueryFilterByName(engineHandle, args[2], true);
                            break;
                        case "-l":
                            QueryFilterByLayer(engineHandle, args[2]);
                            break;
                        default:
                            Console.WriteLine(USAGE);
                            break;
                    }
                    break;
                case "remove":
                    if (args.Length != 3)
                    {
                        Console.WriteLine(USAGE);
                        return;
                    }
                    switch (args[1])
                    {
                        case "-i":
                            RemoveFilterById(engineHandle, ulong.Parse(args[2]));
                            break;
                        case "-k":
                            RemoveFilterByKey(engineHandle, Guid.Parse(args[2]));
                            break;
                        default:
                            Console.WriteLine(USAGE);
                            break;
                    }
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

        static unsafe FWPM_FILTER0** EnumFilters(HANDLE engineHandle, uint* numEntriesReturned)
        {
            uint dwRet;
            HANDLE enumHandle;
            FWPM_FILTER0** filters = null;

            dwRet = PInvoke.FwpmFilterCreateEnumHandle0(engineHandle, null, &enumHandle);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterCreateEnumHandle0");
            }

            dwRet = PInvoke.FwpmFilterEnum0(engineHandle, enumHandle, uint.MaxValue, &filters, numEntriesReturned);
            if (dwRet != 0)
            {
                PInvoke.FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
                throw new Win32Exception((int)dwRet, "FwpmFilterEnum0");
            }

            PInvoke.FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
            return filters;
        }

        static unsafe void FreeFilters(FWPM_FILTER0** filters)
        {
            PInvoke.FwpmFreeMemory0((void**)&filters);
        }

        static unsafe void PrintFilters(HANDLE engineHandle, bool verbose = false)
        {
            uint numEntriesReturned;
            FWPM_FILTER0** filters = EnumFilters(engineHandle, &numEntriesReturned);

            for (uint i = 0; i < numEntriesReturned; i++)
            {
                if (verbose)
                {
                    PrintFilterVerbose(filters[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else
                {
                    Console.WriteLine($"{filters[i]->filterId,-6} {filters[i]->filterKey} {filters[i]->action.type,-30} {filters[i]->displayData.name} ");
                }
            }

            FreeFilters(filters);
        }

        static unsafe void PrintFilterVerbose(FWPM_FILTER0* filter, HANDLE? engineHandle = null)
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
                if (engineHandle.HasValue)
                {
                    FWPM_CALLOUT0* pCallout;
                    PInvoke.FwpmCalloutGetByKey0(engineHandle.Value, &filter->action.Anonymous.calloutKey, &pCallout);
                    Console.WriteLine($"  ID:             {pCallout->calloutId}");
                    Console.WriteLine($"  Name:           {pCallout->displayData.name}");
                    Console.WriteLine($"  Description:    {pCallout->displayData.description}");
                }
            }
            Console.WriteLine($"Layer:            {Helper.TranslateLayerGuid(filter->layerKey)}");
            Console.WriteLine($"SubLayer:         {Helper.TranslateSubLayerGuid(filter->subLayerKey)}");
            Console.WriteLine($"Effective Weight: {Helper.TranslateValue(filter->effectiveWeight)}");
            if (filter->providerKey != null)
            {
                Console.WriteLine($"Provider:         {*filter->providerKey}");
                if (engineHandle.HasValue)
                {
                    FWPM_PROVIDER0* pProvider;
                    PInvoke.FwpmProviderGetByKey0(engineHandle.Value, filter->providerKey, &pProvider);
                    Console.WriteLine($"  Name:           {pProvider->displayData.name}");
                    Console.WriteLine($"  Description:    {pProvider->displayData.description}");
                    Console.WriteLine($"  Service Name:   {pProvider->serviceName}");
                }
            }
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
                throw new Win32Exception((int)dwRet, "FwpmFilterGetById0");
            }

            PrintFilterVerbose(filter, engineHandle);
        }

        static unsafe void QueryFilterByKey(HANDLE engineHandle, Guid key)
        {
            FWPM_FILTER0* filter = null;
            uint dwRet = PInvoke.FwpmFilterGetByKey0(engineHandle, &key, &filter);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterGetByKey0");
            }
            PrintFilterVerbose(filter, engineHandle);
        }

        static unsafe void QueryFilterByName(HANDLE engineHandle, string name, bool useRegex)
        {
            uint numEntriesReturned;
            FWPM_FILTER0** filters = EnumFilters(engineHandle, &numEntriesReturned);

            for (uint i = 0; i < numEntriesReturned; i++)
            {
                if (useRegex && Regex.IsMatch(filters[i]->displayData.name.ToString(), name))
                {
                    PrintFilterVerbose(filters[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else if (filters[i]->displayData.name.ToString().Contains(name))
                {
                    PrintFilterVerbose(filters[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
            }

            PInvoke.FwpmFreeMemory0((void**)&filters);
        }

        static unsafe void QueryFilterByLayer(HANDLE engineHandle, string layer)
        {
            uint numEntriesReturned;
            FWPM_FILTER0** filters = EnumFilters(engineHandle, &numEntriesReturned);
            for (uint i = 0; i < numEntriesReturned; i++)
            {
                if (filters[i]->layerKey == Helper.GetLayerGuid(layer))
                {
                    PrintFilterVerbose(filters[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
            }
            PInvoke.FwpmFreeMemory0((void**)&filters);
        }

        static void RemoveFilterById(HANDLE engineHandle, ulong id)
        {
            var dwRet = PInvoke.FwpmFilterDeleteById0(engineHandle, id);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterDeleteById0");
            }
            else
            {
                Console.WriteLine($"Filter with ID {id} removed successfully");
            }
        }

        static unsafe void RemoveFilterByKey(HANDLE engineHandle, Guid key)
        {
            var dwRet = PInvoke.FwpmFilterDeleteByKey0(engineHandle, &key);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterDeleteById0");
            }
            else
            {
                Console.WriteLine($"Filter with key {key} removed successfully");
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
                    filter.numFilterConditions = (uint)conditions.Count();
                    filter.filterCondition = (FWPM_FILTER_CONDITION0*)Marshal.AllocHGlobal((int)(filter.numFilterConditions * sizeof(FWPM_FILTER_CONDITION0))).ToPointer();

                    for (int i = 0; i < filter.numFilterConditions; i++)
                    {
                        filter.filterCondition[i] = ParseCondition(conditions.ElementAt(i).Item1, conditions.ElementAt(i).Item2, conditions.ElementAt(i).Item3);
                    }

                    var dwRet = PInvoke.FwpmFilterAdd0(engineHandle, &filter, (PSECURITY_DESCRIPTOR)null, &filterId);

                    // Free condition itself and their values
                    for (int i = 0; i < filter.numFilterConditions; i++)
                    {
                        Helper.FreeValue(filter.filterCondition[i].conditionValue);
                    }
                    Marshal.FreeHGlobal((IntPtr)filter.filterCondition);

                    if (dwRet != 0)
                    {
                        throw new Win32Exception((int)dwRet, "FwpmFilterAdd0");
                    }
                }
            }
            Console.WriteLine("Filter added successfully");
            Console.WriteLine();
            QueryFilterById(engineHandle, filterId);
        }

        static FWPM_FILTER_CONDITION0 ParseCondition(string field, string match, string value)
        {
            return new FWPM_FILTER_CONDITION0
            {
                fieldKey = Helper.GetConditionGuid(field),
                matchType = Helper.ParseMatchType(match),
                conditionValue = Helper.ParseConditionValue(value)
            };
        }
    }
}