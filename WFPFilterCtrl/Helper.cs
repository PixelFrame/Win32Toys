using BidirectionalDict;
using System;
using System.Buffers.Binary;
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

        public static void PrintTable(int table)
        {
            switch (table)
            {
                case 0:
                    Console.WriteLine("ConditionGuidTable:");
                    foreach (var item in ConditionGuidTable)
                    {
                        Console.WriteLine($"{item.Key} : {item.Value}");
                    }
                    break;
                case 1:
                    Console.WriteLine("LayerGuidTable:");
                    foreach (var item in LayerGuidTable)
                    {
                        Console.WriteLine($"{item.Key} : {item.Value}");
                    }
                    break;
                case 2:
                    Console.WriteLine("SubLayerGuidTable:");
                    foreach (var item in SubLayerGuidTable)
                    {
                        Console.WriteLine($"{item.Key} : {item.Value}");
                    }
                    break;
                case 3:
                    Console.WriteLine("CalloutGuidTable:");
                    foreach (var item in CalloutGuidTable)
                    {
                        Console.WriteLine($"{item.Key} : {item.Value}");
                    }
                    break;
                default:
                    break;
            }
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
            ConditionGuidTable.AddOrUpdate(Guid.Parse("81BC78FB-F28D-4886-A604-6ACC261F261B"), "FWPM_CONDITION_ALE_PACKAGE_FAMILY_NAME");
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
            var res = TranslateValue(*(FWP_CONDITION_VALUE0*)&value);
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

        internal static FWP_MATCH_TYPE ParseMatchType(string match)
        {
            
            if (Enum.TryParse<FWP_MATCH_TYPE>(match, true, out var parsed))
            {
                return parsed;
            }
            if (Enum.TryParse<FWP_MATCH_TYPE>("FWP_MATCH_" + match, true, out parsed))
            {
                return parsed;
            }
            throw new ArgumentException($"Invalid match type {match}");
        }

        internal unsafe static FWP_VALUE0 ParseValue(string value)
        {
            var res = new FWP_VALUE0();
            var type = value.Substring(0, value.IndexOf('!')).ToUpper();
            var val = value.Substring(value.IndexOf('!') + 1);

            switch (type)
            {
                case "EMPTY":
                case "FWP_EMPTY":
                    res.type = FWP_DATA_TYPE.FWP_EMPTY;
                    break;
                case "UINT8":
                case "FWP_UINT8":
                    res.type = FWP_DATA_TYPE.FWP_UINT8;
                    res.Anonymous.uint8 = byte.Parse(val);
                    break;
                case "UINT16":
                case "FWP_UINT16":
                    res.type = FWP_DATA_TYPE.FWP_UINT16;
                    res.Anonymous.uint16 = ushort.Parse(val);
                    break;
                case "UINT32":
                case "FWP_UINT32":
                    res.type = FWP_DATA_TYPE.FWP_UINT32;
                    res.Anonymous.uint32 = uint.Parse(val);
                    break;
                case "UINT64":
                case "FWP_UINT64":
                    res.type = FWP_DATA_TYPE.FWP_UINT64;
                    var pVal = Marshal.AllocHGlobal(sizeof(ulong));
                    Marshal.WriteInt64(pVal, long.Parse(val));
                    res.Anonymous.uint64 = (ulong*)pVal;
                    break;
                case "INT8":
                case "FWP_INT8":
                    res.type = FWP_DATA_TYPE.FWP_INT8;
                    res.Anonymous.int8 = sbyte.Parse(val);
                    break;
                case "INT16":
                case "FWP_INT16":
                    res.type = FWP_DATA_TYPE.FWP_INT16;
                    res.Anonymous.int16 = short.Parse(val);
                    break;
                case "INT32":
                case "FWP_INT32":
                    res.type = FWP_DATA_TYPE.FWP_INT32;
                    res.Anonymous.int32 = int.Parse(val);
                    break;
                case "INT64":
                case "FWP_INT64":
                    res.type = FWP_DATA_TYPE.FWP_INT64;
                    var pVal2 = Marshal.AllocHGlobal(sizeof(long));
                    Marshal.WriteInt64(pVal2, long.Parse(val));
                    res.Anonymous.int64 = (long*)pVal2;
                    break;
                case "FLOAT":
                case "FWP_FLOAT":
                    res.type = FWP_DATA_TYPE.FWP_FLOAT;
                    res.Anonymous.float32 = float.Parse(val);
                    break;
                case "DOUBLE":
                case "FWP_DOUBLE":
                    res.type = FWP_DATA_TYPE.FWP_DOUBLE;
                    var pVal3 = Marshal.AllocHGlobal(sizeof(double));
                    Marshal.WriteInt64(pVal3, BitConverter.DoubleToInt64Bits(double.Parse(val)));
                    res.Anonymous.double64 = (double*)pVal3;
                    break;
                case "BYTE_ARRAY16":
                case "FWP_BYTE_ARRAY16_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_BYTE_ARRAY16_TYPE;
                    var arr16 = Convert.FromBase64String(val);
                    var pVal4 = Marshal.AllocHGlobal(16);
                    Marshal.Copy(arr16, 0, pVal4, 16);
                    res.Anonymous.byteArray16 = (FWP_BYTE_ARRAY16*)pVal4;
                    break;
                case "BYTE_BLOB":
                case "FWP_BYTE_BLOB_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_BYTE_BLOB_TYPE;
                    var blob = new FWP_BYTE_BLOB();
                    var data = Convert.FromBase64String(val);
                    blob.size = (uint)data.Length;
                    blob.data = (byte*)Marshal.AllocHGlobal(data.Length);
                    Marshal.Copy(data, 0, (IntPtr)blob.data, data.Length);
                    var pVal5 = Marshal.AllocHGlobal(Marshal.SizeOf(blob));
                    Marshal.StructureToPtr(blob, pVal5, false);
                    res.Anonymous.byteBlob = (FWP_BYTE_BLOB*)pVal5;
                    break;
                case "SID":
                case "FWP_SID":
                    res.type = FWP_DATA_TYPE.FWP_SID;
                    PInvoke.ConvertStringSidToSid(val, out var pSid);
                    res.Anonymous.sid = (SID*)pSid.Value;
                    break;
                case "SECURITY_DESCRIPTOR":
                case "FWP_SECURITY_DESCRIPTOR_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_SECURITY_DESCRIPTOR_TYPE;
                    var sd = new FWP_BYTE_BLOB();
                    uint sdSize;
                    PInvoke.ConvertStringSecurityDescriptorToSecurityDescriptor(val, PInvoke.SDDL_REVISION_1, out var pSd, &sdSize);
                    sd.size = sdSize;
                    sd.data = (byte*)pSd.Value;
                    var pVal6 = Marshal.AllocHGlobal(Marshal.SizeOf(sd));
                    Marshal.StructureToPtr(sd, pVal6, false);
                    res.Anonymous.sd = (FWP_BYTE_BLOB*)pVal6;
                    break;
                case "TOKEN_INFORMATION":
                case "FWP_TOKEN_INFORMATION_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_TOKEN_INFORMATION_TYPE;
                    throw new NotImplementedException("Not implemented: FWP_TOKEN_INFORMATION_TYPE");
                case "TOKEN_ACCESS_INFORMATION":
                case "FWP_TOKEN_ACCESS_INFORMATION_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_TOKEN_ACCESS_INFORMATION_TYPE;
                    throw new NotImplementedException("Not implemented: FWP_TOKEN_ACCESS_INFORMATION_TYPE");
                case "UNICODE_STRING":
                case "FWP_UNICODE_STRING_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_UNICODE_STRING_TYPE;
                    var pVal7 = Marshal.StringToHGlobalUni(val);
                    res.Anonymous.unicodeString = (char*)pVal7;
                    break;
                case "BYTE_ARRAY6":
                case "FWP_BYTE_ARRAY6_TYPE":
                    res.type = FWP_DATA_TYPE.FWP_BYTE_ARRAY6_TYPE;
                    var arr6 = Convert.FromBase64String(val);
                    var pVal8 = Marshal.AllocHGlobal(6);
                    Marshal.Copy(arr6, 0, pVal8, 6);
                    res.Anonymous.byteArray6 = (FWP_BYTE_ARRAY6*)pVal8;
                    break;
                default:
                    throw new ArgumentException($"Invalid value type {type}");
            }
            return res;
        }

        internal unsafe static FWP_CONDITION_VALUE0 ParseConditionValue(string value)
        {
            var res = new FWP_CONDITION_VALUE0();
            if (value.Contains("-"))
            {
                res.type = FWP_DATA_TYPE.FWP_RANGE_TYPE;
                var parts = value.Split('-');
                var low = ParseValue(parts[0]);
                var high = ParseValue(parts[1]);
                var range = new FWP_RANGE0
                {
                    valueLow = low,
                    valueHigh = high
                };
                var pRange = Marshal.AllocHGlobal(Marshal.SizeOf(range));
                Marshal.StructureToPtr(range, pRange, false);
                res.Anonymous.uint64 = (ulong*)pRange;
            }
            else
            {
                if (!value.Contains("!"))
                throw new ArgumentException($"Invalid value {value}");
                var type = value.Substring(0, value.IndexOf('!')).ToUpper();
                var val = value.Substring(value.IndexOf('!') + 1);
                switch (type)
                {
                    case "EMPTY":
                    case "FWP_EMPTY":
                        res.type = FWP_DATA_TYPE.FWP_EMPTY;
                        break;
                    case "UINT8":
                    case "FWP_UINT8":
                        res.type = FWP_DATA_TYPE.FWP_UINT8;
                        res.Anonymous.uint8 = byte.Parse(val);
                        break;
                    case "UINT16":
                    case "FWP_UINT16":
                        res.type = FWP_DATA_TYPE.FWP_UINT16;
                        res.Anonymous.uint16 = ushort.Parse(val);
                        break;
                    case "UINT32":
                    case "FWP_UINT32":
                        res.type = FWP_DATA_TYPE.FWP_UINT32;
                        res.Anonymous.uint32 = uint.Parse(val);
                        break;
                    case "UINT64":
                    case "FWP_UINT64":
                        res.type = FWP_DATA_TYPE.FWP_UINT64;
                        var pVal = Marshal.AllocHGlobal(sizeof(ulong));
                        Marshal.WriteInt64(pVal, long.Parse(val));
                        res.Anonymous.uint64 = (ulong*)pVal;
                        break;
                    case "INT8":
                    case "FWP_INT8":
                        res.type = FWP_DATA_TYPE.FWP_INT8;
                        res.Anonymous.int8 = sbyte.Parse(val);
                        break;
                    case "INT16":
                    case "FWP_INT16":
                        res.type = FWP_DATA_TYPE.FWP_INT16;
                        res.Anonymous.int16 = short.Parse(val);
                        break;
                    case "INT32":
                    case "FWP_INT32":
                        res.type = FWP_DATA_TYPE.FWP_INT32;
                        res.Anonymous.int32 = int.Parse(val);
                        break;
                    case "INT64":
                    case "FWP_INT64":
                        res.type = FWP_DATA_TYPE.FWP_INT64;
                        var pVal2 = Marshal.AllocHGlobal(sizeof(long));
                        Marshal.WriteInt64(pVal2, long.Parse(val));
                        res.Anonymous.int64 = (long*)pVal2;
                        break;
                    case "FLOAT":
                    case "FWP_FLOAT":
                        res.type = FWP_DATA_TYPE.FWP_FLOAT;
                        res.Anonymous.float32 = float.Parse(val);
                        break;
                    case "DOUBLE":
                    case "FWP_DOUBLE":
                        res.type = FWP_DATA_TYPE.FWP_DOUBLE;
                        var pVal3 = Marshal.AllocHGlobal(sizeof(double));
                        Marshal.WriteInt64(pVal3, BitConverter.DoubleToInt64Bits(double.Parse(val)));
                        res.Anonymous.double64 = (double*)pVal3;
                        break;
                    case "BYTE_ARRAY16":
                    case "FWP_BYTE_ARRAY16_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_BYTE_ARRAY16_TYPE;
                        var arr16 = Convert.FromBase64String(val);
                        var pVal4 = Marshal.AllocHGlobal(16);
                        Marshal.Copy(arr16, 0, pVal4, 16);
                        res.Anonymous.byteArray16 = (FWP_BYTE_ARRAY16*)pVal4;
                        break;
                    case "BYTE_BLOB":
                    case "FWP_BYTE_BLOB_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_BYTE_BLOB_TYPE;
                        var blob = new FWP_BYTE_BLOB();
                        var data = Convert.FromBase64String(val);
                        blob.size = (uint)data.Length;
                        blob.data = (byte*)Marshal.AllocHGlobal(data.Length);
                        Marshal.Copy(data, 0, (IntPtr)blob.data, data.Length);
                        var pVal5 = Marshal.AllocHGlobal(Marshal.SizeOf(blob));
                        Marshal.StructureToPtr(blob, pVal5, false);
                        res.Anonymous.byteBlob = (FWP_BYTE_BLOB*)pVal5;
                        break;
                    case "SID":
                    case "FWP_SID":
                        res.type = FWP_DATA_TYPE.FWP_SID;
                        PInvoke.ConvertStringSidToSid(val, out var pSid);
                        res.Anonymous.sid = (SID*)pSid.Value;
                        break;
                    case "SECURITY_DESCRIPTOR":
                    case "FWP_SECURITY_DESCRIPTOR_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_SECURITY_DESCRIPTOR_TYPE;
                        var sd = new FWP_BYTE_BLOB();
                        uint sdSize;
                        PInvoke.ConvertStringSecurityDescriptorToSecurityDescriptor(val, PInvoke.SDDL_REVISION_1, out var pSd, &sdSize);
                        sd.size = sdSize;
                        sd.data = (byte*)pSd.Value;
                        var pVal6 = Marshal.AllocHGlobal(Marshal.SizeOf(sd));
                        Marshal.StructureToPtr(sd, pVal6, false);
                        res.Anonymous.sd = (FWP_BYTE_BLOB*)pVal6;
                        break;
                    case "TOKEN_INFORMATION":
                    case "FWP_TOKEN_INFORMATION_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_TOKEN_INFORMATION_TYPE;
                        throw new NotImplementedException("Not implemented: FWP_TOKEN_INFORMATION_TYPE");
                    case "TOKEN_ACCESS_INFORMATION":
                    case "FWP_TOKEN_ACCESS_INFORMATION_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_TOKEN_ACCESS_INFORMATION_TYPE;
                        throw new NotImplementedException("Not implemented: FWP_TOKEN_ACCESS_INFORMATION_TYPE");
                    case "UNICODE_STRING":
                    case "FWP_UNICODE_STRING_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_UNICODE_STRING_TYPE;
                        var pVal7 = Marshal.StringToHGlobalUni(val);
                        res.Anonymous.unicodeString = (char*)pVal7;
                        break;
                    case "BYTE_ARRAY6":
                    case "FWP_BYTE_ARRAY6_TYPE":
                        res.type = FWP_DATA_TYPE.FWP_BYTE_ARRAY6_TYPE;
                        var arr6 = Convert.FromBase64String(val);
                        var pVal8 = Marshal.AllocHGlobal(6);
                        Marshal.Copy(arr6, 0, pVal8, 6);
                        res.Anonymous.byteArray6 = (FWP_BYTE_ARRAY6*)pVal8;
                        break;
                    case "V4_ADDR_MASK":
                    case "FWP_V4_ADDR_MASK":
                        res.type = FWP_DATA_TYPE.FWP_V4_ADDR_MASK;
                        var parts = val.Split('/');
                        var addr = IPAddress.Parse(parts[0]);
                        var mask = IPAddress.Parse(parts[1]);
                        var pVal9 = Marshal.AllocHGlobal(sizeof(FWP_V4_ADDR_AND_MASK));
                        var pV4AddrMask = (FWP_V4_ADDR_AND_MASK*)pVal9;
                        pV4AddrMask->addr = BitConverter.ToUInt32(addr.GetAddressBytes(), 0);
                        pV4AddrMask->mask = BitConverter.ToUInt32(mask.GetAddressBytes(), 0);
                        res.Anonymous.uint64 = (ulong*)pVal9;
                        break;
                    case "V6_ADDR_MASK":
                    case "FWP_V6_ADDR_MASK":
                        res.type = FWP_DATA_TYPE.FWP_V6_ADDR_MASK;
                        var parts2 = val.Split('/');
                        var addr2 = IPAddress.Parse(parts2[0]);
                        var maskLen = byte.Parse(parts2[1]);
                        var pVal10 = Marshal.AllocHGlobal(sizeof(FWP_V6_ADDR_AND_MASK));
                        var pV6AddrMask = (FWP_V6_ADDR_AND_MASK*)pVal10;
                        var addrBytes = addr2.GetAddressBytes();
                        for (int i = 0; i < 16; i++)
                        {
                            pV6AddrMask->addr.Value[i] = addrBytes[i];
                        }
                        pV6AddrMask->prefixLength = maskLen;
                        res.Anonymous.uint64 = (ulong*)pVal10;
                        break;
                    default:
                        throw new ArgumentException($"Invalid value type {type}");
                }
            }
            return res;
        }

        internal unsafe static void FreeValue(FWP_VALUE0 value)
        {
            FreeValue(*(FWP_CONDITION_VALUE0*)&value);
        }

        internal unsafe static void FreeValue(FWP_CONDITION_VALUE0 conditionValue)
        {
            switch (conditionValue.type)
            {
                case FWP_DATA_TYPE.FWP_UINT64:
                case FWP_DATA_TYPE.FWP_INT64:
                case FWP_DATA_TYPE.FWP_DOUBLE:
                case FWP_DATA_TYPE.FWP_UNICODE_STRING_TYPE:
                case FWP_DATA_TYPE.FWP_BYTE_ARRAY16_TYPE:
                case FWP_DATA_TYPE.FWP_BYTE_ARRAY6_TYPE:
                case FWP_DATA_TYPE.FWP_V4_ADDR_MASK:
                case FWP_DATA_TYPE.FWP_V6_ADDR_MASK:
                    Marshal.FreeHGlobal((IntPtr)conditionValue.Anonymous.uint64);
                    break;
                case FWP_DATA_TYPE.FWP_BYTE_BLOB_TYPE:
                    var blob = conditionValue.Anonymous.byteBlob;
                    Marshal.FreeHGlobal((IntPtr)blob->data);
                    Marshal.FreeHGlobal((IntPtr)blob);
                    break;
                case FWP_DATA_TYPE.FWP_SID:
                    PInvoke.LocalFree(new HLOCAL(conditionValue.Anonymous.sid));
                    break;
                case FWP_DATA_TYPE.FWP_SECURITY_DESCRIPTOR_TYPE:
                    var sd = conditionValue.Anonymous.sd;
                    PInvoke.LocalFree(new HLOCAL(sd->data));
                    Marshal.FreeHGlobal((IntPtr)sd);
                    break;
                case FWP_DATA_TYPE.FWP_RANGE_TYPE:
                    var range = (FWP_RANGE0*)conditionValue.Anonymous.uint64;
                    FreeValue(range->valueLow);
                    FreeValue(range->valueHigh);
                    Marshal.FreeHGlobal((IntPtr)range);
                    break;
                default: break;
            }
        }
    }
}
