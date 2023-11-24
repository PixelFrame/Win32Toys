#include <iostream>
#include <windows.h>
#include <evntrace.h>

static const UINT MAX_SESSION_NAME_LEN = 1024;
static const UINT MAX_LOGFILE_PATH_LEN = 1024;
static const GUID ProviderGuid =
    { 0x22FB2CD6, 0x0E7B, 0x422B, { 0xA0, 0xC7, 0x2F, 0xAD, 0x1F, 0xD0, 0xE7, 0x16 } };

int wmain(int argc, WCHAR** argv)
{
    if (argc != 2)
    {
        std::wcout << L"Perform process rundown in an existing ETW session by adding Microsoft-Windows-Kernel-Process provider.";
        std::wcout << L"Usage: " << argv[0] << L" <SessionName>";
        return ERROR_INVALID_PARAMETER;
    }

    ULONG err = ERROR_SUCCESS;
    EVENT_TRACE_PROPERTIES* pSessionProperties = NULL;
    ULONG BufferSize = 0;
    BufferSize = sizeof(EVENT_TRACE_PROPERTIES) +
        (MAX_SESSION_NAME_LEN * sizeof(WCHAR)) +
        (MAX_LOGFILE_PATH_LEN * sizeof(WCHAR));
    pSessionProperties = (EVENT_TRACE_PROPERTIES*)malloc(BufferSize);
    if (NULL == pSessionProperties)
    {
        std::wcout << L"Unable to allocate " << BufferSize << L" bytes for properties structure.";
        free(pSessionProperties);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(pSessionProperties, BufferSize);
    pSessionProperties->Wnode.BufferSize = BufferSize;
    err = ControlTrace(NULL, argv[1], pSessionProperties, EVENT_TRACE_CONTROL_QUERY);

    if (err != ERROR_SUCCESS)
    {
        std::wcout << L"Failed to acquire trace session handle. Code " << err << L".";
        free(pSessionProperties);
        return err;
    }

    err = EnableTraceEx2(
        pSessionProperties->Wnode.HistoricalContext,
        &ProviderGuid,
        EVENT_CONTROL_CODE_CAPTURE_STATE,       // Request 'ProcessRundown' events
        TRACE_LEVEL_NONE,                       // Probably ignored for 'EVENT_CONTROL_CODE_CAPTURE_STATE'
        0,                                      // Probably ignored for 'EVENT_CONTROL_CODE_CAPTURE_STATE'
        0,                                      // Probably ignored for 'EVENT_CONTROL_CODE_CAPTURE_STATE'
        INFINITE,                               // Synchronous operation
        nullptr                                 // Probably ignored for 'EVENT_CONTROL_CODE_CAPTURE_STATE'
    );
    if (err != ERROR_SUCCESS)
    {
        std::wcout << L"Failed to enable process snapshot. Code " << err << L".";
    }
    free(pSessionProperties);
    return err;
}
