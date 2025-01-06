#include <windows.h>

#include <iostream>
#include <string>

void QuerySingleDosDevice(LPCWSTR lpDevice)
{
    DWORD buffLen = 65536;
    WCHAR* buff = new WCHAR[buffLen];

    DWORD rtLen = QueryDosDeviceW(lpDevice, buff, buffLen);

    if (rtLen == 0)
    {
        std::wcout << L"QueryDosDeviceW for device " << lpDevice << L" failed with 0x" << std::hex << GetLastError() << std::endl;
    }
    else
    {
        DWORD idx = 0;
        std::wstring indent(wcslen(lpDevice) + 4, L' ');
        std::wcout << lpDevice << L" -> ";
        while (idx < rtLen)
        {
            if (buff[idx] != 0) std::wcout << buff[idx];
            else if (buff[idx + 1] != 0) std::wcout << std::endl << indent;
            else { std::wcout << std::endl; break; }
            idx++;
        }
    }
    delete[] buff;
}

void QueryAllDosDevices()
{
    DWORD buffLen = 65536;
    WCHAR* buff = new WCHAR[buffLen];

    DWORD rtLen = QueryDosDeviceW(NULL, buff, buffLen);

    if (rtLen == 0)
    {
        std::wcout << L"QueryDosDeviceW failed with 0x" << std::hex << GetLastError() << std::endl;
    }
    else
    {
        QuerySingleDosDevice(buff);
        DWORD idx = 1;
        while (idx < rtLen)
        {
            if (buff[idx] == 0)
            {
                if (buff[idx + 1] != 0) QuerySingleDosDevice(buff + idx + 1);
                else break;
            }
            idx++;
        }
    }
    delete[] buff;
}

void AddDosDevice(LPCWSTR lpDevice, LPCWSTR lpTarget, BOOL raw)
{
    DWORD dwFlags = raw ? DDD_RAW_TARGET_PATH : 0;
    if (!DefineDosDeviceW(dwFlags, lpDevice, lpTarget))
    {
        std::wcout << L"DefineDosDeviceW failed with 0x" << std::hex << GetLastError() << std::endl;
    }
    else
    {
        std::wcout << L"Device " << lpDevice << L" -> " << lpTarget << L" defined" << std::endl;
    }
}

void RemoveDosDevice(LPCWSTR lpDevice, LPCWSTR lpTarget, BOOL raw)
{
    DWORD dwFlags = DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE | (raw ? DDD_RAW_TARGET_PATH : 0);
    if (!DefineDosDeviceW(dwFlags, lpDevice, lpTarget))
    {
        std::wcout << L"DefineDosDeviceW failed with 0x" << std::hex << GetLastError() << std::endl;
    }
    else
    {
        std::wcout << L"Device " << lpDevice << L" -> " << lpTarget << L" removed" << std::endl;
    }
}

void RemoveAllDosDevices(LPCWSTR lpDevice)
{
    DWORD buffLen = 65536;
    WCHAR* buff = new WCHAR[buffLen];

    DWORD rtLen = QueryDosDeviceW(lpDevice, buff, buffLen);

    if (rtLen == 0)
    {
        std::wcout << L"QueryDosDeviceW for device " << lpDevice << L" failed with 0x" << std::hex << GetLastError() << std::endl;
    }
    else
    {
        RemoveDosDevice(lpDevice, buff, TRUE);
        DWORD idx = 1;
        while (idx < rtLen)
        {
            if (buff[idx] == 0)
            {
                if (buff[idx + 1] != 0) RemoveDosDevice(lpDevice, buff + idx + 1, TRUE);
                else break;
            }
            idx++;
        }
    }
    delete[] buff;
}

const WCHAR* USAGE = L"ddev <-a|-d> [-r] <device> <target>\n"
                     L"ddev -da <device>\n"
                     L"ddev -q [device]";

int wmain(int argc, WCHAR** argv)
{
    if (argc < 2)
    {
        goto PRINT_USAGE;
    }
    if (argc == 2)
    {
        if (wcscmp(argv[1], L"-q") == 0)
        {
            QueryAllDosDevices();
        }
        else goto PRINT_USAGE;
    }
    if (argc == 3)
    {
        if (wcscmp(argv[1], L"-q") == 0)
        {
            QuerySingleDosDevice(argv[2]);
        }
        else if (wcscmp(argv[1], L"-da") == 0)
        {
            RemoveAllDosDevices(argv[2]);
        }
        else goto PRINT_USAGE;
    }
    if (argc == 4)
    {
        if (wcscmp(argv[1], L"-a") == 0)
        {
            AddDosDevice(argv[2], argv[3], false);
        }
        else if (wcscmp(argv[1], L"-d") == 0)
        {
            RemoveDosDevice(argv[2], argv[3], false);
        }
        else goto PRINT_USAGE;
    }
    if (argc == 5)
    {
        if (wcscmp(argv[1], L"-a") == 0 && wcscmp(argv[2], L"-r") == 0)
        {
            AddDosDevice(argv[3], argv[4], true);
        }
        else if (wcscmp(argv[1], L"-d") == 0 && wcscmp(argv[2], L"-r") == 0)
        {
            RemoveDosDevice(argv[3], argv[4], true);
        }
        else goto PRINT_USAGE;
    }

    return 0;

PRINT_USAGE:
    std::wcout << USAGE << std::endl;
    return -1;
}