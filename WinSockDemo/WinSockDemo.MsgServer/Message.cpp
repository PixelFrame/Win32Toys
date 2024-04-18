#include "Message.h"

string Message::value()
{
    return _value;
}

FILETIME Message::time()
{
    return _time;
}

string Message::source()
{
    return _source;
}
