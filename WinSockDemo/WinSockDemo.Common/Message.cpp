#include "Message.h"
#include "Utilities.h"

string Message::value() const
{
    return _value;
}

string Message::time() const
{
    return FormatSystemTimeString(_time);
}

string Message::source() const
{
    return _source;
}

string Message::to_string() const
{
    return format("[{}] [{}] {}", time(), source(), value());
}