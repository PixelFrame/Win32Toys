#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <string>
#include <format>

using std::string;
using std::format;      // Need C++20

class Message
{
public:
    Message(string value, string source) : _value(value), _source(source)
    {
        GetSystemTime(&_time);
    }

    string value();
    string time();
    string source();
    string to_string();

private:
    string _value = "";
    SYSTEMTIME _time = {};
    string _source = "127.0.0.1";
};