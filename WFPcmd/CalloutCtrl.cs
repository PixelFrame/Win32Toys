using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection.Emit;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.NetworkManagement.WindowsFilteringPlatform;

namespace WFPcmd
{
    internal static class CalloutCtrl
    {
        public static unsafe FWPM_CALLOUT0** EnumCallouts(HANDLE engineHandle, uint* numCalloutsReturned)
        {

            uint dwRet = 0;
            FWPM_CALLOUT0** callouts = null;
            HANDLE enumHandle = HANDLE.Null;

            dwRet = PInvoke.FwpmCalloutCreateEnumHandle0(engineHandle, null, &enumHandle);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmCalloutCreateEnumHandle0");
            }

            PInvoke.FwpmCalloutEnum0(engineHandle, enumHandle, 0xFFFFFFFF, &callouts, numCalloutsReturned);
            if (dwRet != 0)
            {
                PInvoke.FwpmCalloutDestroyEnumHandle0(engineHandle, enumHandle);
                throw new Win32Exception((int)dwRet, "FwpmCalloutEnum0");
            }

            PInvoke.FwpmCalloutDestroyEnumHandle0(engineHandle, enumHandle);
            return callouts;
        }

        public static unsafe void FreeCallouts(FWPM_CALLOUT0** callouts)
        {
            PInvoke.FwpmFreeMemory0((void**)&callouts);
        }

        public static unsafe void PrintCallouts(HANDLE engineHandle, bool verbose = false)
        {
            uint numCalloutsReturned = 0;
            FWPM_CALLOUT0** callouts = EnumCallouts(engineHandle, &numCalloutsReturned);
            for (int i = 0; i < numCalloutsReturned; i++)
            {
                if (verbose)
                {
                    PrintCalloutVerbose(callouts[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else
                {
                    Console.WriteLine($"{callouts[i]->calloutId,-3} {Helper.TranslateCalloutGuid(callouts[i]->calloutKey),-50} {callouts[i]->displayData.name}");
                }
            }
        }

        public static unsafe void PrintCalloutVerbose(FWPM_CALLOUT0* callout, HANDLE? engineHandle = null)
        {
            Console.WriteLine($"ID:               {callout->calloutId}");
            Console.WriteLine($"Key:              {Helper.TranslateCalloutGuid(callout->calloutKey)}");
            Console.WriteLine($"Name:             {callout->displayData.name}");
            Console.WriteLine($"Description:      {callout->displayData.description}");
            Console.WriteLine($"Applicable Layer: {Helper.TranslateLayerGuid(callout->applicableLayer)}");
            Console.WriteLine($"Flags:            0x{callout->flags:X}");
            foreach (var flag in Enum.GetValues(typeof(FWPM_CALLOUT_FLAGS)))
            {
                if ((callout->flags & (uint)flag) == (uint)flag)
                {
                    Console.WriteLine($"  {Enum.GetName(typeof(FWPM_CALLOUT_FLAGS), flag)}");
                }
            }
            if (callout->providerKey != null)
            {
                Console.WriteLine($"Provider:         {*callout->providerKey}");
                if (engineHandle.HasValue)
                {
                    FWPM_PROVIDER0* pProvider;
                    PInvoke.FwpmProviderGetByKey0(engineHandle.Value, callout->providerKey, &pProvider);
                    Console.WriteLine($"  Name:           {pProvider->displayData.name}");
                    Console.WriteLine($"  Description:    {pProvider->displayData.description}");
                    Console.WriteLine($"  Service Name:   {pProvider->serviceName}");
                }
            }
        }

        public static unsafe void QueryCalloutByKey(HANDLE engineHandle, string key)
        {
            FWPM_CALLOUT0* callout = null;
            Guid calloutKey = Helper.GetCalloutGuid(key);
            uint dwRet = PInvoke.FwpmCalloutGetByKey0(engineHandle, &calloutKey, &callout);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmCalloutGetByKey0");
            }
            PrintCalloutVerbose(callout, engineHandle);
            PInvoke.FwpmFreeMemory0((void**)&callout);
        }

        public static unsafe void QueryCalloutByName(HANDLE engineHandle, string name, bool useRegex)
        {
            uint numEntriesReturned;
            FWPM_CALLOUT0** callouts = EnumCallouts(engineHandle, &numEntriesReturned);

            for (uint i = 0; i < numEntriesReturned; i++)
            {
                if (useRegex && Regex.IsMatch(callouts[i]->displayData.name.ToString(), name))
                {
                    PrintCalloutVerbose(callouts[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else if (callouts[i]->displayData.name.ToString().Contains(name))
                {
                    PrintCalloutVerbose(callouts[i], engineHandle);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
            }

            PInvoke.FwpmFreeMemory0((void**)&callouts);
        }

        public static unsafe void RemoveCalloutByKey(HANDLE engineHandle, Guid key)
        {
            var dwRet = PInvoke.FwpmCalloutDeleteByKey0(engineHandle, &key);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmCalloutDeleteByKey0");
            }
            else
            {
                Console.WriteLine($"Callout with key {key} removed successfully");
            }
        }
    }

    [Flags]
    public enum FWPM_CALLOUT_FLAGS : uint
    {
        FWPM_CALLOUT_FLAG_PERSISTENT = 0x00010000,
        FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT = 0x00020000,
        FWPM_CALLOUT_FLAG_REGISTERED = 0x00040000,
    }
}
