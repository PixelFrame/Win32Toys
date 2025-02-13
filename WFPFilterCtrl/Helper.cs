using BidirectionalDict;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.NetworkManagement.WindowsFilteringPlatform;
using Windows.Win32.Security;

namespace WFPFilterCtrl
{
    internal class Helper
    {
        public static BiDictionary<Guid, string> ConditionGuidTable = [];
        public static BiDictionary<Guid, string> LayerGuidTable = [];
        public static BiDictionary<Guid, string> SubLayerGuidTable = [];
        public static BiDictionary<Guid, string> CalloutGuidTable = [];

        public static void GenerateTables()
        {
            GenerateConditionGuidTable();
            GenerateLayerGuidTable();
            GenerateSubLayerGuidTable();
            GenerateCalloutGuidTable();
        }

        private static void GenerateConditionGuidTable()
        {
            typeof(Windows.Win32.PInvoke).GetFields(System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static).ToList().ForEach(f =>
            {
                if (f.Name.StartsWith("FWPM_CONDITION_") && f.FieldType == typeof(Guid))
                {
                    ConditionGuidTable.AddOrUpdate((Guid)f.GetValue(null), f.Name);
                }
            });

            // Added since Cu
            ConditionGuidTable.AddOrUpdate(Guid.Parse("81BC78FB-F28D-4886-A604-6ACC261F261B") , "FWPM_CONDITION_ALE_PACKAGE_FAMILY_NAME");
        }

        public static string TranslateConditionGuid(Guid guid)
        {
            return ConditionGuidTable.Contains(guid) ? ConditionGuidTable[guid].Substring(15) : guid.ToString();
        }

        public static Guid GetConditionGuid(string name)
        {
            return ConditionGuidTable.Contains(name) ? ConditionGuidTable[name] : Guid.Parse(name);
        }

        private static void GenerateLayerGuidTable()
        {
            typeof(Windows.Win32.PInvoke).GetFields(System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static).ToList().ForEach(f =>
            {
                if (f.Name.StartsWith("FWPM_LAYER_") && f.FieldType == typeof(Guid))
                {
                    LayerGuidTable.AddOrUpdate((Guid)f.GetValue(null), f.Name);
                }
            });
        }

        public static string TranslateLayerGuid(Guid guid)
        {
            return LayerGuidTable.Contains(guid) ? LayerGuidTable[guid] : guid.ToString();
        }

        public static Guid GetLayerGuid(string name)
        {
            return LayerGuidTable.Contains(name) ? LayerGuidTable[name] : Guid.Parse(name);
        }

        private static void GenerateSubLayerGuidTable()
        {
            typeof(Windows.Win32.PInvoke).GetFields(System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static).ToList().ForEach(f =>
            {
                if (f.Name.StartsWith("FWPM_SUBLAYER_") && f.FieldType == typeof(Guid))
                {
                    SubLayerGuidTable.AddOrUpdate((Guid)f.GetValue(null), f.Name);
                }
            });
        }

        public static string TranslateSubLayerGuid(Guid guid)
        {
            return SubLayerGuidTable.Contains(guid) ? SubLayerGuidTable[guid] : guid.ToString();
        }

        private static void GenerateCalloutGuidTable()
        {
            typeof(Windows.Win32.PInvoke).GetFields(System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static).ToList().ForEach(f =>
            {
                if (f.Name.StartsWith("FWPM_CALLOUT_") && f.FieldType == typeof(Guid))
                {
                    CalloutGuidTable.AddOrUpdate((Guid)f.GetValue(null), f.Name);
                }
            });
        }

        public static string TranslateCalloutGuid(Guid guid)
        {
            return CalloutGuidTable.Contains(guid) ? CalloutGuidTable[guid] : guid.ToString();
        }

        public unsafe static string TranslateValue(FWP_VALUE0 value)
        {
            GCHandle handle = GCHandle.Alloc(value, GCHandleType.Pinned);
            FWP_CONDITION_VALUE0* pValue = (FWP_CONDITION_VALUE0*)handle.AddrOfPinnedObject();
            var res = TranslateValue(*pValue);
            handle.Free();
            return res;
        }

        public unsafe static string TranslateValue(FWP_CONDITION_VALUE0 value)
        {
            switch (value.type)
            {
                case FWP_DATA_TYPE.FWP_EMPTY:
                    return "Empty";
                case FWP_DATA_TYPE.FWP_UINT8:
                    return (value.Anonymous.uint8).ToString();
                case FWP_DATA_TYPE.FWP_UINT16:
                    return (value.Anonymous.uint16).ToString();
                case FWP_DATA_TYPE.FWP_UINT32:
                    return (value.Anonymous.uint32).ToString();
                case FWP_DATA_TYPE.FWP_UINT64:
                    return (*value.Anonymous.uint64).ToString();
                case FWP_DATA_TYPE.FWP_INT8:
                    return (value.Anonymous.int8).ToString();
                case FWP_DATA_TYPE.FWP_INT16:
                    return (value.Anonymous.int16).ToString();
                case FWP_DATA_TYPE.FWP_INT32:
                    return (value.Anonymous.int32).ToString();
                case FWP_DATA_TYPE.FWP_INT64:
                    return (*value.Anonymous.int64).ToString();
                case FWP_DATA_TYPE.FWP_FLOAT:
                    return (value.Anonymous.float32).ToString();
                case FWP_DATA_TYPE.FWP_DOUBLE:
                    return (*value.Anonymous.double64).ToString();
                case FWP_DATA_TYPE.FWP_BYTE_ARRAY16_TYPE:
                    {
                        var arr = (*value.Anonymous.byteArray16).byteArray16;
                        return $"{arr[0]:X2}{arr[1]:X2}{arr[2]:X2}{arr[3]:X2}{arr[4]:X2}{arr[5]:X2}{arr[6]:X2}{arr[7]:X2}{arr[8]:X2}{arr[9]:X2}{arr[10]:X2}{arr[11]:X2}{arr[12]:X2}{arr[13]:X2}{arr[14]:X2}{arr[15]:X2}";
                    }
                case FWP_DATA_TYPE.FWP_BYTE_BLOB_TYPE:
                    {
                        var blob = *value.Anonymous.byteBlob;
                        return Encoding.Unicode.GetString(blob.data, (int)blob.size);
                    }
                case FWP_DATA_TYPE.FWP_SID:
                    {
                        PWSTR strSid;
                        PSID pSID = new(value.Anonymous.sid);
                        PInvoke.ConvertSidToStringSid(pSID, &strSid);
                        var res = strSid.ToString();
                        PInvoke.LocalFree(new HLOCAL(strSid));
                        return res;
                    }
                case FWP_DATA_TYPE.FWP_SECURITY_DESCRIPTOR_TYPE:
                    {
                        var sd = (*value.Anonymous.sd).data;
                        PSECURITY_DESCRIPTOR pSD = new(sd);
                        PWSTR strSD;
                        uint strSDLen;
                        PInvoke.ConvertSecurityDescriptorToStringSecurityDescriptor(pSD,
                            PInvoke.SDDL_REVISION_1,
                            OBJECT_SECURITY_INFORMATION.DACL_SECURITY_INFORMATION /*| OBJECT_SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION | OBJECT_SECURITY_INFORMATION.SACL_SECURITY_INFORMATION*/, // Only DACL matters
                            &strSD, &strSDLen);
                        var res = strSD.ToString();
                        PInvoke.LocalFree(new HLOCAL(strSD));
                        return res;
                    }
                case FWP_DATA_TYPE.FWP_TOKEN_INFORMATION_TYPE:
                    {
                        var tokenInfo = *value.Anonymous.tokenInformation;
                        return "Not implemented: FWP_TOKEN_INFORMATION_TYPE";
                    }
                case FWP_DATA_TYPE.FWP_TOKEN_ACCESS_INFORMATION_TYPE:
                    {
                        var tokenAccessInfo = *value.Anonymous.tokenAccessInformation;
                        return "Not implemented: FWP_TOKEN_ACCESS_INFORMATION_TYPE";
                    }
                case FWP_DATA_TYPE.FWP_UNICODE_STRING_TYPE:
                    return value.Anonymous.unicodeString.ToString();
                case FWP_DATA_TYPE.FWP_BYTE_ARRAY6_TYPE:
                    {
                        var arr = (*value.Anonymous.byteArray6).byteArray6;
                        return $"{arr[0]:X2}{arr[1]:X2}{arr[2]:X2}{arr[3]:X2}{arr[4]:X2}{arr[5]:X2}";
                    }
                case FWP_DATA_TYPE.FWP_V4_ADDR_MASK:
                    {
                        var pValue = (FWP_V4_ADDR_AND_MASK*)value.Anonymous.uint64;
                        var addr = new IPAddress(pValue->addr);
                        var mask = new IPAddress(pValue->mask);
                        return $"{addr}/{mask}";
                    }
                case FWP_DATA_TYPE.FWP_V6_ADDR_MASK:
                    {
                        var pValue = (FWP_V6_ADDR_AND_MASK*)value.Anonymous.uint64;
                        var addrbytes = (new Span<byte>(pValue->addr.Value, 16)).ToArray();
                        var addr = new IPAddress(addrbytes);
                        var maskLen = pValue->prefixLength;
                        return $"{addr}/{maskLen}";
                    }
                case FWP_DATA_TYPE.FWP_RANGE_TYPE:
                    {
                        var pValue = (FWP_RANGE0*)value.Anonymous.uint64;
                        return $"{TranslateValue(pValue->valueLow)}-{TranslateValue(pValue->valueHigh)}";
                    }
                default:
                    return "Unknown value";
            }
        }

    }
}
