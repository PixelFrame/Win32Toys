#define SECURITY_WIN32

#include <windows.h>
#include <windns.h>
#include <stdio.h>
#include <rpc.h>
#include <rpcdce.h>
#include <security.h>
#include <winsock.h>
#include <strsafe.h>
#include <ip2string.h>
#include <in6addr.h>
#include <ntstatus.h>

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ntdll.lib")

static const WCHAR* Usage =
L"DnsRegistrator.exe [-u <UserName> -p <Password> [-d <Domain>]] [-t (A|AAAA|CNAME)] -n <RecordFQDN> -v <RecordValue> [-l <TTL>] [-s <ServerAddr>] -r\n"
L"DnsRegistrator.exe -h\n";

static const WCHAR* UsageVerbose =
L"Initiate DNS update requests via Windows DNS API\n"
L"DnsRegistrator.exe [-u <UserName> -p <Password> [-d <Domain>]] [-t (A|AAAA|CNAME)] -n <RecordFQDN> -v <RecordValue> [-l <TTL>] [-s <ServerAddr>] -r\n"
L"DnsRegistrator.exe -h\n"
L"  -u      User name to perform the registration\n"
L"  -p      Password of the user\n"
L"  -d      Domain of the user\n"
L"  -t      DNS type to be registered\n"
L"  -n      DNS name to be registered\n"
L"  -v      Value of the record\n"
L"  -l      TTL of the record\n"
L"  -s      DNS server address that SOA query will be sent to\n"
L"  -r      Remove other existing records of the same name\n"
L"  -h      Print this message\n"
;

void PrintUsage(int verbosity = 0)
{
    if (verbosity == 0)
        wprintf(Usage);
    else
        wprintf(UsageVerbose);
}

void CleanUp(PDNS_RECORD pAddDnsRecord, PDNS_RECORD pDeleteDnsRecord, PSEC_WINNT_AUTH_IDENTITY_W pCredentials, PIP4_ARRAY pSrvList)
{
    if (NULL != pAddDnsRecord) LocalFree(pAddDnsRecord);
    if (NULL != pDeleteDnsRecord) LocalFree(pDeleteDnsRecord);
    if (NULL != pSrvList) LocalFree(pSrvList);
    if (NULL != pCredentials) LocalFree(pCredentials);
}

int __cdecl wmain(int argc, wchar_t** argv)
{
    WCHAR* wDomain = NULL, * wName = NULL, * wPassword = NULL, * wFqdn = NULL, * wValue = NULL;
    DWORD                             dwTtl = 3600;
    WORD                              wRecordType = DNS_TYPE_A;
    PSEC_WINNT_AUTH_IDENTITY_W        pCredentials = NULL;
    HANDLE                            secHandle = NULL;
    PDNS_RECORD                       pAddDnsRecord = (PDNS_RECORD)LocalAlloc(LPTR, sizeof(DNS_RECORD));
    PDNS_RECORD                       pDeleteDnsRecord = NULL;
    PIP4_ARRAY                        pSrvList = NULL;
    DNS_STATUS                        status;
    LPCWSTR                           terminator;

    if (NULL == pAddDnsRecord)
    {
        wprintf(L"Couldn't allocate memory!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    int i = 1;
    while (i < argc)
    {
        if (_wcsicmp(argv[i], L"-h") == 0)
        {
            PrintUsage(1);
            CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
            return ERROR_SUCCESS;
        }
        if (_wcsicmp(argv[i], L"-u") == 0)
        {
            wName = argv[i + 1];
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-p") == 0)
        {
            wPassword = argv[i + 1];
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-d") == 0)
        {
            wDomain = argv[i + 1];
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-t") == 0)
        {
            if (_wcsicmp(argv[i + 1], L"A") == 0)
            {
                wRecordType = DNS_TYPE_A;
            }
            else if (_wcsicmp(argv[i + 1], L"AAAA") == 0)
            {
                wRecordType = DNS_TYPE_AAAA;
            }
            else if (_wcsicmp(argv[i + 1], L"CNAME") == 0)
            {
                wRecordType = DNS_TYPE_CNAME;
            }
            else
            {
                wprintf(L"Bad DNS Type: %s\n\n", argv[i + 1]);
                PrintUsage();
                CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
                return ERROR_INVALID_PARAMETER;
            }
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-n") == 0)
        {
            wFqdn = argv[i + 1];
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-v") == 0)
        {
            wValue = argv[i + 1];
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-l") == 0)
        {
            WCHAR* end;
            dwTtl = wcstol(argv[i + 1], &end, 10);
            if (*end != NULL)
            {
                wprintf(L"Bad TTL value: %s\n\n", argv[i + 1]);
                PrintUsage();
                CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
                return ERROR_INVALID_PARAMETER;
            }
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-s") == 0)
        {
            in_addr addr;
            pSrvList = (PIP4_ARRAY)LocalAlloc(LPTR, sizeof(IP4_ARRAY));
            if (NULL == pSrvList)
            {
                wprintf(L"Could not allocate memory!\n");
                CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            pSrvList->AddrCount = 1;
            if (RtlIpv4StringToAddressW(argv[i + 1], FALSE, &terminator, &addr) != STATUS_SUCCESS)
            {
                wprintf(L"Invalid IPv4 address\n\n");
                PrintUsage();
                CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
                return ERROR_INVALID_PARAMETER;
            }
            pSrvList->AddrArray[0] = addr.S_un.S_addr;
            i += 2;
        }
        else if (_wcsicmp(argv[i], L"-r") == 0)
        {
            pDeleteDnsRecord = (PDNS_RECORD)LocalAlloc(LPTR, sizeof(DNS_RECORD));
            if (NULL == pDeleteDnsRecord)
            {
                wprintf(L"Couldn't allocate memory!\n");
                CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            i += 1;
        }
        else
        {
            wprintf(L"Bad Argument: %s\n\n", argv[i]);
            PrintUsage();
            CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (NULL == wFqdn || wValue == NULL)
    {
        wprintf(L"DNS name or value not provided\n\n");
        PrintUsage();
        CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
        return ERROR_INVALID_PARAMETER;
    }

    switch (wRecordType)
    {
    case DNS_TYPE_AAAA:
        pAddDnsRecord->wDataLength = sizeof(DNS_AAAA_DATA);
        in6_addr in6addr;
        if (RtlIpv6StringToAddressW(wValue, &terminator, &in6addr) != STATUS_SUCCESS)
        {
            wprintf(L"Invalid IPv6 address\n\n");
            PrintUsage();
            CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
            return ERROR_INVALID_PARAMETER;
        }
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[0] = in6addr.u.Word[0];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[1] = in6addr.u.Word[1];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[2] = in6addr.u.Word[2];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[3] = in6addr.u.Word[3];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[4] = in6addr.u.Word[4];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[5] = in6addr.u.Word[5];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[6] = in6addr.u.Word[6];
        pAddDnsRecord->Data.AAAA.Ip6Address.IP6Word[7] = in6addr.u.Word[7];
        break;
    case DNS_TYPE_CNAME:
        pAddDnsRecord->wDataLength = sizeof(DNS_PTR_DATA);
        pAddDnsRecord->Data.CNAME.pNameHost = wValue;
        break;
    default:
        in_addr inaddr;
        if (RtlIpv4StringToAddressW(wValue, FALSE, &terminator, &inaddr) != STATUS_SUCCESS)
        {
            wprintf(L"Invalid IPv4 address\n\n");
            PrintUsage();
            CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
            return ERROR_INVALID_PARAMETER;
        }
        pAddDnsRecord->wDataLength = sizeof(DNS_A_DATA);
        pAddDnsRecord->Data.A.IpAddress = inaddr.S_un.S_addr;
    }

    pAddDnsRecord->pName = wFqdn;
    pAddDnsRecord->wType = wRecordType;
    pAddDnsRecord->dwTtl = dwTtl;

    if (NULL != pDeleteDnsRecord)
    {
        pDeleteDnsRecord->pName = wFqdn;
        pDeleteDnsRecord->wType = DNS_TYPE_ANY;
        pDeleteDnsRecord->dwTtl = 0;
    }

    if (NULL != wName)
    {
        pCredentials = (PSEC_WINNT_AUTH_IDENTITY_W)LocalAlloc(LPTR, sizeof(SEC_WINNT_AUTH_IDENTITY_W));
        pCredentials->User = (unsigned short*)wName;
        pCredentials->UserLength = wcslen(wName);
        if (NULL != wPassword)
        {
            pCredentials->Password = (unsigned short*)wPassword;
            pCredentials->PasswordLength = wcslen(wPassword);
        }
        if (NULL != wDomain)
        {
            pCredentials->Domain = (unsigned short*)wDomain;
            pCredentials->DomainLength = wcslen(wDomain);
        }
        pCredentials->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        status = DnsAcquireContextHandle_W(
            0,
            pCredentials,
            &secHandle);
        if (status)
        {
            wprintf(L"Could not aquire credentials %d \n",
                status);
            secHandle = NULL;
        }
        else if (!secHandle) 
        {
            wprintf(L"DnsAcquireContextHandle returned success but handle is NULL\n");
        }
        else
        {
            wprintf(L"DnsAcquireContextHandle returned successful \n");
        }
    }

    wprintf(L"Start DNS registration\n");
    status = DnsModifyRecordsInSet_W(pAddDnsRecord,
        pDeleteDnsRecord,
        DNS_UPDATE_SECURITY_USE_DEFAULT,
        secHandle,
        pSrvList,
        NULL);

    if (status)
    {
        wprintf(L"Couldn't add records %d \n", status);
    }

    else
    {
        wprintf(L"Added records successfully \n");
    }

    CleanUp(pAddDnsRecord, pDeleteDnsRecord, pCredentials, pSrvList);
    return 0;
}