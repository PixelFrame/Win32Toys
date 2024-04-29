#include "Utilities.h"

std::string FormatSystemTimeString(SYSTEMTIME time)
{
    return std::format("{}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

std::string GetCurrentTimeString()

{
    SYSTEMTIME now;
    GetSystemTime(&now);
    return FormatSystemTimeString(now);
}

std::string FormatDebugString(const char* function, const char* message)
{
    return std::format("[{}] [{}] {}", GetCurrentTimeString(), function, message);
}
