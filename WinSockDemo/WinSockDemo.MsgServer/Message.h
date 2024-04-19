#pragma once

#include "StdAfx.h"

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

private:
    string _value = "";
    SYSTEMTIME _time = {};
    string _source = "127.0.0.1";
};