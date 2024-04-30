#include "Message.h"
#include "Utilities.h"

string Message::value()
{
    return _value;
}

string Message::time()
{
    return FormatSystemTimeString(_time);
}

string Message::source()
{
    return _source;
}

string Message::to_string()
{
    return format("[{}] [{}] {}", time(), source(), value());
}