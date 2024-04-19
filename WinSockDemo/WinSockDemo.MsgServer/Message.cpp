#include "Message.h"

string Message::value()
{
    return _value;
}

string Message::time()
{
    return format("{}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
        _time.wYear, _time.wMonth, _time.wDay,
        _time.wHour, _time.wMinute, _time.wSecond, _time.wMilliseconds
    );
}

string Message::source()
{
    return _source;
}
