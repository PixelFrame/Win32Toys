using BidirectionalDict;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.NetworkManagement.WindowsFilteringPlatform;

namespace WFPcmd
{
    internal static class LayerCtrl
    {
        public static unsafe FWPM_LAYER0** EnumLayers(HANDLE engineHandle, uint* numLayersReturned)
        {
            uint dwRet = 0;
            FWPM_LAYER0** layers = null;
            HANDLE enumHandle = HANDLE.Null;

            dwRet = PInvoke.FwpmLayerCreateEnumHandle0(engineHandle, null, &enumHandle);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmLayerCreateEnumHandle0");
            }

            PInvoke.FwpmLayerEnum0(engineHandle, enumHandle, 0xFFFFFFFF, &layers, numLayersReturned);
            if (dwRet != 0)
            {
                PInvoke.FwpmLayerDestroyEnumHandle0(engineHandle, enumHandle);
                throw new Win32Exception((int)dwRet, "FwpmLayerEnum0");
            }

            PInvoke.FwpmLayerDestroyEnumHandle0(engineHandle, enumHandle);
            return layers;
        }

        public static unsafe void FreeLayers(FWPM_LAYER0** layers)
        {
            PInvoke.FwpmFreeMemory0((void**)&layers);
        }

        public static unsafe void QueryLayerByKey(HANDLE engineHandle, string key)
        {
            FWPM_LAYER0* layer = null;
            Guid layerKey = Helper.GetLayerGuid(key);
            uint dwRet = PInvoke.FwpmLayerGetByKey0(engineHandle, &layerKey, &layer);
            if (dwRet != 0)
            {
                throw new Win32Exception((int)dwRet, "FwpmLayerGetByKey0");
            }
            PrintLayerVerbose(layer);
            PInvoke.FwpmFreeMemory0((void**)&layer);
        }

        public static unsafe void QueryLayerByName(HANDLE engineHandle, string name, bool useRegex)
        {
            uint numLayersReturned = 0;
            FWPM_LAYER0** layers = EnumLayers(engineHandle, &numLayersReturned);
            for (int i = 0; i < numLayersReturned; i++)
            {
                if (useRegex && Regex.IsMatch(layers[i]->displayData.name.ToString(), name))
                {
                    PrintLayerVerbose(layers[i]);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else if (layers[i]->displayData.name.ToString().Contains(name))
                {
                    PrintLayerVerbose(layers[i]);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
            }
        }

        public static unsafe void PrintLayers(HANDLE engineHandle, bool verbose = false)
        {
            uint numLayersReturned = 0;
            FWPM_LAYER0** layers = EnumLayers(engineHandle, &numLayersReturned);
            for (int i = 0; i < numLayersReturned; i++)
            {
                if (verbose)
                {
                    PrintLayerVerbose(layers[i]);
                    Console.WriteLine(new string('-', Console.BufferWidth - 1));
                }
                else
                {
                    Console.WriteLine($"{layers[i]->layerId,-3} {Helper.TranslateLayerGuid(layers[i]->layerKey),-50} {layers[i]->displayData.name}");
                }
            }
        }

        public unsafe static void PrintLayerVerbose(FWPM_LAYER0* layer)
        {
            Console.WriteLine($"Key:   {Helper.TranslateLayerGuid(layer->layerKey)}");
            Console.WriteLine($"ID:    {layer->layerId}");
            Console.WriteLine($"Name:  {layer->displayData.name}");
            Console.WriteLine($"Flags: 0x{layer->flags:X}");
            foreach (var flag in Enum.GetValues(typeof(FWPM_LAYER_FLAGS)))
            {
                if ((layer->flags & (uint)flag) == (uint)flag)
                {
                    Console.WriteLine($"  {Enum.GetName(typeof(FWPM_LAYER_FLAGS), flag)}");
                }
            }
            Console.WriteLine($"Fields:");
            {
                for (int i = 0; i < layer->numFields; i++)
                {
                    Console.WriteLine($"  {Helper.TranslateConditionGuid(*layer->field[i].fieldKey),-35} {layer->field[i].type.ToString().Substring(11),-15} {layer->field[i].dataType.ToString().Substring(4)}");
                }
            }
        }
    }

    [Flags]
    public enum FWPM_LAYER_FLAGS : uint
    {
        FWPM_LAYER_FLAG_KERNEL = 0x00000001,
        FWPM_LAYER_FLAG_BUILTIN = 0x00000002,
        FWPM_LAYER_FLAG_CLASSIFY_MOSTLY = 0x00000004,
        FWPM_LAYER_FLAG_BUFFERED = 0x00000008,
    }
}
