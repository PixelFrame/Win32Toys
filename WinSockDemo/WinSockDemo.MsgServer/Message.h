#pragma once

#include <Windows.h>
#include <string>

using std::string;

class Message
{
public:
    Message() = default;
    Message(string value, FILETIME time) : _value(value), _time(time) {}

    string value();
    FILETIME time();
    string source();

private:
    string _value = "";
    FILETIME _time = { 0, 0 };
    string _source = "127.0.0.1";
};