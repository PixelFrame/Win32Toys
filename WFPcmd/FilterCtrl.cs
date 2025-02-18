using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Dynamic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.NetworkManagement.WindowsFilteringPlatform;
using Windows.Win32.Security;

namespace WFPcmd
{
    internal static class FilterCtrl
    {
        public static unsafe FWPM_FILTER0** EnumFilters(HANDLE engineHandle, uint* numEntriesReturned)
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

        public static unsafe void FreeFilters(FWPM_FILTER0** filters)
        {
            PInvoke.FwpmFreeMemory0((void**)&filters);
        }

        public static unsafe void PrintFilters(HANDLE engineHandle, bool verbose = false, bool json = false)
        {
            uint numEntriesReturned;
            FWPM_FILTER0** filters = EnumFilters(engineHandle, &numEntriesReturned);

            if (json)
            {
                var dFilters = new List<dynamic>();
                for (uint i = 0; i < numEntriesReturned; i++)
                {
                    dFilters.Add(FilterToDynamic(filters[i], engineHandle));
                }
                Console.WriteLine(JsonSerializer.Serialize(dFilters, new JsonSerializerOptions { WriteIndented = true }));
                FreeFilters(filters);
                return;
            }
            else
            {
                for (uint i = 0; i < numEntriesReturned; i++)
                {
                    if (verbose)
                    {
                        PrintFilterVerbose(filters[i], engineHandle);
                        Console.WriteLine(new string('-', Console.BufferWidth - 1));
                    }
                    else
                    {
                        Console.WriteLine($"{filters[i]->filterId,-8} {filters[i]->filterKey}   {filters[i]->action.type.ToString().Substring(11),-20} {filters[i]->displayData.name} ");
                    }
                }
            }
            FreeFilters(filters);
        }

        public static unsafe void PrintFilterVerbose(FWPM_FILTER0* filter, HANDLE? engineHandle = null)
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
            Console.WriteLine($"Weight:           {Helper.TranslateValue(filter->weight, true)}");
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
            Console.WriteLine($"Flags:            0x{filter->flags:X}");
            foreach (var flag in Enum.GetValues(typeof(FWPM_FILTER_FLAGS_1)))
            {
                if ((uint)flag == 0) continue; // skip none
                if (((uint)filter->flags & (uint)flag) == (uint)flag)
                {
                    Console.WriteLine($"  {Enum.GetName(typeof(FWPM_FILTER_FLAGS_1), flag)}");
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

        public static unsafe dynamic FilterToDynamic(FWPM_FILTER0* filter, HANDLE? engineHandle = null)
        {
            dynamic dFilter = new ExpandoObject();
            dFilter.ID = filter->filterId;
            dFilter.Key = filter->filterKey.ToString();
            dFilter.Name = filter->displayData.name.ToString();
            dFilter.Description = filter->displayData.description.ToString();
            dFilter.Action = filter->action.type.ToString().Substring(11);
            if (filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_TERMINATING
                || filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_UNKNOWN
                || filter->action.type == FWP_ACTION_TYPE.FWP_ACTION_CALLOUT_INSPECTION)
            {
                dFilter.Callout = new ExpandoObject();
                dFilter.Callout.Key = Helper.TranslateCalloutGuid(filter->action.Anonymous.calloutKey);
                if (engineHandle.HasValue)
                {
                    FWPM_CALLOUT0* pCallout;
                    PInvoke.FwpmCalloutGetByKey0(engineHandle.Value, &filter->action.Anonymous.calloutKey, &pCallout);
                    dFilter.Callout.ID = pCallout->calloutId;
                    dFilter.Callout.Name = pCallout->displayData.name.ToString();
                    dFilter.Callout.Description = pCallout->displayData.description.ToString();
                }
            }
            dFilter.Layer = Helper.TranslateLayerGuid(filter->layerKey);
            dFilter.SubLayer = Helper.TranslateSubLayerGuid(filter->subLayerKey);
            dFilter.Weight = Helper.TranslateValue(filter->weight, true);
            dFilter.EffectiveWeight = Helper.TranslateValue(filter->effectiveWeight);
            if (filter->providerKey != null)
            {
                dFilter.Provider = new ExpandoObject();
                dFilter.Provider.Key = (*filter->providerKey).ToString();
                if (engineHandle.HasValue)
                {
                    FWPM_PROVIDER0* pProvider;
                    PInvoke.FwpmProviderGetByKey0(engineHandle.Value, filter->providerKey, &pProvider);
                    dFilter.Provider.Name = pProvider->displayData.name.ToString();
                    dFilter.Provider.Description = pProvider->displayData.description.ToString();
                    dFilter.Provider.ServiceName = pProvider->serviceName.ToString();
                }
            }
            if (filter->numFilterConditions > 0)
            {
                dFilter.Conditions = new List<ExpandoObject>();
                for (uint j = 0; j < filter->numFilterConditions; j++)
                {
                    dynamic dCondition = new ExpandoObject();
                    dCondition.Field = Helper.TranslateConditionGuid(filter->filterCondition[j].fieldKey);
                    dCondition.Match = filter->filterCondition[j].matchType.ToString().Substring(10);
                    dCondition.Value = Helper.TranslateValue(filter->filterCondition[j].conditionValue);
                    dFilter.Conditions.Add(dCondition);
                }
            }
            return dFilter;
        }

        public static unsafe void QueryFilterById(HANDLE engineHandle, ulong id)
        {
            FWPM_FILTER0* filter = null;
            uint dwRet = PInvoke.FwpmFilterGetById0(engineHandle, id, &filter);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterGetById0");
            }

            PrintFilterVerbose(filter, engineHandle);
        }

        public static unsafe void QueryFilterByKey(HANDLE engineHandle, Guid key)
        {
            FWPM_FILTER0* filter = null;
            uint dwRet = PInvoke.FwpmFilterGetByKey0(engineHandle, &key, &filter);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterGetByKey0");
            }
            PrintFilterVerbose(filter, engineHandle);
            PInvoke.FwpmFreeMemory0((void**)&filter);
        }

        public static unsafe void QueryFilterByName(HANDLE engineHandle, string name, bool useRegex)
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

        public static unsafe void QueryFilterByLayer(HANDLE engineHandle, string layer)
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

        public static unsafe void QueryFilterSdByKey(HANDLE engineHandle, Guid key)
        {
            uint dwRet = 0;
            PSECURITY_DESCRIPTOR pSD;
            dwRet = PInvoke.FwpmFilterGetSecurityInfoByKey0(engineHandle, &key,
                (uint)(OBJECT_SECURITY_INFORMATION.DACL_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.SACL_SECURITY_INFORMATION),
                null, null, null, null, &pSD);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterGetSecurityInfoByKey0");
            }

            PWSTR strSD;
            uint strSDLen;
            PInvoke.ConvertSecurityDescriptorToStringSecurityDescriptor(pSD,
                PInvoke.SDDL_REVISION_1,
                OBJECT_SECURITY_INFORMATION.DACL_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.SACL_SECURITY_INFORMATION,
                &strSD, &strSDLen);
            Console.WriteLine(strSD.ToString());
            PInvoke.FwpmFreeMemory0((void**)&pSD);
            PInvoke.LocalFree(new HLOCAL(strSD));
        }

        public static unsafe void SetFilterSdByKey(HANDLE engineHandle, Guid key, string sddl)
        {
            uint dwRet = 0;
            uint dwSize;
            PSECURITY_DESCRIPTOR pSd;
            fixed (char* pSddl = sddl)
            {
                if (!PInvoke.ConvertStringSecurityDescriptorToSecurityDescriptor(pSddl, PInvoke.SDDL_REVISION_1, &pSd, &dwSize))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "ConvertStringSecurityDescriptorToSecurityDescriptor");
                }
            }

            PInvoke.GetSecurityDescriptorDacl(pSd, out var daclPresent, out var pDacl, out var daclDefaulted);
            PInvoke.GetSecurityDescriptorSacl(pSd, out var saclPresent, out var pSacl, out var saclDefaulted);
            PInvoke.GetSecurityDescriptorOwner(pSd, out var pOwner, out var ownerDefaulted);
            PInvoke.GetSecurityDescriptorGroup(pSd, out var pGroup, out var groupDefaulted);

            OBJECT_SECURITY_INFORMATION secInfo = 0;
            if (daclPresent) secInfo |= OBJECT_SECURITY_INFORMATION.DACL_SECURITY_INFORMATION;
            if (saclPresent) secInfo |= OBJECT_SECURITY_INFORMATION.SACL_SECURITY_INFORMATION;
            if (pOwner != null) secInfo |= OBJECT_SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION;
            if (pGroup != null) secInfo |= OBJECT_SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION;

            dwRet = PInvoke.FwpmFilterSetSecurityInfoByKey0(engineHandle, &key,
                (uint)secInfo,
                (SID*)pOwner.Value,
                (SID*)pGroup.Value,
                pDacl,
                pSacl
                );
            PInvoke.LocalFree(new HLOCAL(pSd.Value));
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmFilterSetSecurityInfoByKey0");
            }
            else
            {
                Console.WriteLine("Successfully set filter security descriptor");
            }
        }

        public static void RemoveFilterById(HANDLE engineHandle, ulong id)
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

        public static unsafe void RemoveFilterByKey(HANDLE engineHandle, Guid key)
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

        public static unsafe void AddFilter(HANDLE engineHandle, string name, string description, string layer, string action, IEnumerable<(string, string, string)> conditions, ulong weight)
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

        public static FWPM_FILTER_CONDITION0 ParseCondition(string field, string match, string value)
        {
            return new FWPM_FILTER_CONDITION0
            {
                fieldKey = Helper.GetConditionGuid(field),
                matchType = Helper.ParseMatchType(match),
                conditionValue = Helper.ParseConditionValue(value)
            };
        }
    }

    // More bits defined in SDK 10.0.26100.0
    [Flags]
    public enum FWPM_FILTER_FLAGS_1 : uint
    {
        FWPM_FILTER_FLAG_NONE                                = 0x00000000,
        FWPM_FILTER_FLAG_PERSISTENT                          = 0x00000001,
        FWPM_FILTER_FLAG_BOOTTIME                            = 0x00000002,
        FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT                = 0x00000004,
        FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT                  = 0x00000008,
        FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED      = 0x00000010,
        FWPM_FILTER_FLAG_DISABLED                            = 0x00000020,
        FWPM_FILTER_FLAG_INDEXED                             = 0x00000040,
        FWPM_FILTER_FLAG_HAS_SECURITY_REALM_PROVIDER_CONTEXT = 0x00000080,
        FWPM_FILTER_FLAG_SYSTEMOS_ONLY                       = 0x00000100,
        FWPM_FILTER_FLAG_GAMEOS_ONLY                         = 0x00000200,
        FWPM_FILTER_FLAG_SILENT_MODE                         = 0x00000400,
        FWPM_FILTER_FLAG_IPSEC_NO_ACQUIRE_INITIATE           = 0x00000800,
        FWPM_FILTER_FLAG_RESERVED0                           = 0x00001000,
        FWPM_FILTER_FLAG_RESERVED1                           = 0x00002000,
        FWPM_FILTER_FLAG_RESERVED2                           = 0x00004000,
    }
}
