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
    std::cout << "DhcpOptRequester.exe < -a:AdapterName > < -i:OptId > [ -u:UserClass ] [ -v ]\n";
    std::cout << "    -a Adapter to perform the DHCP request\n";
    std::cout << "    -i Option ID in dec\n";
    std::cout << "    -u User class\n";
    std::cout << "    -v Request vendor specific option. For Windows, the vendor class will always be \"MSFT 5.0\".\n";
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
    WCHAR* pwszAdapterName = CharStrToWCharStr(pszAdapterName);

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
    if (argc < 3 || argc > 5)
    {
        PrintUsage();
        exit(ERROR_INVALID_PARAMETER);
    }

    DWORD              dwError, dwSize;
    CHAR               TmpBuffer[1000]{};
    CHAR               pszAdapterGuid[40]{};
    DWORD              dwOptId = 0;
    BOOL               bVendorSpecific = FALSE;
    DHCPCAPI_CLASSID   ClassId;
    LPDHCPCAPI_CLASSID pClassId = NULL;
    CHAR* pszAdapterName = NULL;
    WCHAR* pwszAdapterGuid = NULL;

    BYTE paramMap = 0;   // 0000 aiuv

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-'
            || strlen(argv[i]) < 2
            || (argv[i][1] != 'v' && strlen(argv[i]) < 4)
            || (argv[i][1] != 'v' && argv[i][2] != ':'))
        {
            std::cout << "Invalid argument: " << argv[i] << "\n\n";
            PrintUsage();
            exit(ERROR_INVALID_PARAMETER);
        }
        switch (argv[i][1])
        {
        case 'a':
            if ((paramMap & 0x08) != 0)
            {
                std::cout << "Duplicate argument: " << argv[i] << "\n\n";
                PrintUsage();
                exit(ERROR_INVALID_PARAMETER);
            }
            pszAdapterName = argv[i] + 3;
            paramMap |= 0x08;
            break;
        case 'i':
            if ((paramMap & 0x04) != 0)
            {
                std::cout << "Duplicate argument: " << argv[i] << "\n\n";
                PrintUsage();
                exit(ERROR_INVALID_PARAMETER);
            }
            CHAR * strEnd;
            dwOptId = strtoul(argv[i] + 3, &strEnd, 10);
            if (*strEnd != '\0')
            {
                std::cout << argv[2] << " is not a valid option ID" << std::endl;
                exit(ERROR_INVALID_PARAMETER);
            }
            paramMap |= 0x04;
            break;
        case 'u':
            if ((paramMap & 0x02) != 0)
            {
                std::cout << "Duplicate argument: " << argv[i] << "\n\n";
                PrintUsage();
                exit(ERROR_INVALID_PARAMETER);
            }
            ClassId = {
                   0,
                   (BYTE*)(argv[i] + 3),
                   (ULONG)strlen(argv[i] + 3)
            };
            pClassId = &ClassId;
            paramMap |= 0x02;
            break;
        case 'v':
            if ((paramMap & 0x01) != 0)
            {
                std::cout << "Duplicate argument: " << argv[i] << "\n\n";
                PrintUsage();
                exit(ERROR_INVALID_PARAMETER);
            }
            bVendorSpecific = TRUE;
            paramMap |= 0x01;
            break;
        default:
            std::cout << "Invalid argument: " << argv[i] << "\n\n";
            PrintUsage();
            exit(ERROR_INVALID_PARAMETER);
            break;
        }
    }
    if ((paramMap & 0x0c) != 0x0c)
    {
        std::cout << "Required argument not provided" << "\n\n";
        PrintUsage();
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
            bVendorSpecific,  // vendor specific?
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
        pClassId,                     // not using class id
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