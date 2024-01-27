#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <DhcpCSdk.h>
#include <iphlpapi.h>
#include <iostream>
#include <iomanip>

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment( lib, "dhcpcsvc.lib" )

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

void PrintUsage()
{
    std::wcout << L"DhcpOptRequester.exe <AdapterName> <OptId>";
}

WCHAR* CharStrToWCharStr(const char* c)
{
    const size_t cSize = strlen(c) + 1;
    size_t returnValue = 0;
    wchar_t* wc = new wchar_t[cSize];
    if (mbstowcs_s(&returnValue, wc, cSize, c, cSize) == 0)
        return wc;
    else
    {
        std::cout << "Unable to convert char string to wide char string" << std::endl;
        exit(ERROR_INVALID_PARAMETER);
    }
}

BOOL GetAdapterGuid(CHAR* pszAdapterName, OUT CHAR* pszAdapterGuid)
{
    DWORD                 dwFamily = AF_INET;
    DWORD                 dwFlags = GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_INCLUDE_ALL_COMPARTMENTS;
    IP_ADAPTER_ADDRESSES* pAdapterAddresses = NULL;
    IP_ADAPTER_ADDRESSES* pCurrAdapterAddresses = NULL;
    DWORD                 dwBufLen = WORKING_BUFFER_SIZE;
    DWORD                 dwIterations = 0;
    DWORD                 dwRetVal;
    WCHAR*                pwszAdapterName = CharStrToWCharStr(pszAdapterName);

    do
    {
        pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(dwBufLen);
        if (pAdapterAddresses == NULL)
        {
            std::cout << "Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n";
            exit(ERROR_NOT_ENOUGH_MEMORY);
        }

        dwRetVal =
            GetAdaptersAddresses(dwFamily, dwFlags, NULL, pAdapterAddresses, &dwBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            FREE(pAdapterAddresses);
            pAdapterAddresses = NULL;
        }
        else
        {
            break;
        }

        dwIterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (dwIterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR)
    {
        pCurrAdapterAddresses = pAdapterAddresses;
        while (pCurrAdapterAddresses)
        {
            if (pCurrAdapterAddresses->Dhcpv4Enabled && wcscmp(pCurrAdapterAddresses->FriendlyName, pwszAdapterName) == 0)
            {
                strcpy_s(pszAdapterGuid, 40, pCurrAdapterAddresses->AdapterName);
                FREE(pAdapterAddresses);
                return TRUE;
            }
            pCurrAdapterAddresses = pCurrAdapterAddresses->Next;
        }
    }
    return FALSE;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        PrintUsage();
        exit(ERROR_INVALID_PARAMETER);
    }

    DWORD dwError, dwSize;
    CHAR TmpBuffer[1000]{};
    CHAR* pszAdapterName = argv[1];
    CHAR pszAdapterGuid[40];
    WCHAR* pwszAdapterGuid = NULL;

    CHAR* strEnd;
    DWORD dwOptId = strtoul(argv[2], &strEnd, 10);
    if (*strEnd != '\0')
    {
        std::cout << argv[2] << " is not a valid option ID" << std::endl;
        exit(ERROR_INVALID_PARAMETER);
    }

    if (!GetAdapterGuid(pszAdapterName, pszAdapterGuid))
    {
        std::cout << "Cannot find DHCPv4 enabled adapter with name " << pszAdapterName << std::endl;
        exit(ERROR_NOT_FOUND);
    }
    pwszAdapterGuid = CharStrToWCharStr(pszAdapterGuid);

    DHCPCAPI_PARAMS DhcpApiParams = {
            0,                // Flags
            dwOptId,          // OptionId
            FALSE,            // vendor specific?
            NULL,             // data filled in on return
            0                 // nBytes
    };
    DHCPCAPI_PARAMS_ARRAY RequestParams = {
            1,  // only one option to request 
            &DhcpApiParams
    };

    DHCPCAPI_PARAMS_ARRAY SendParams = {
            0,
            NULL
    };

    dwSize = sizeof(TmpBuffer);
    dwError = DhcpRequestParams(
        DHCPCAPI_REQUEST_SYNCHRONOUS, // Flags
        NULL,                         // Reserved
        pwszAdapterGuid,              // Adapter Name
        NULL,                         // not using class id
        SendParams,                   // sent parameters
        RequestParams,                // requesting params
        (PBYTE)TmpBuffer,             // buffer
        &dwSize,                      // buffer size
        NULL                          // Request ID
    );

    if (NO_ERROR == dwError)
    {
        for (unsigned int i = 0; i < DhcpApiParams.nBytesData; ++i)
            std::cout << std::hex 
                      << std::setfill('0') 
                      << std::setw(2) 
                      << (unsigned short)DhcpApiParams.Data[i]          // We have to cast char to something else so that cout does not print the ASCII character...
                      << " ";
    }

    return dwError;
}