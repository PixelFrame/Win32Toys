#pragma once

#include <string>
#include <format>

using std::string;
using std::format;

class Win32Exception : public exception
{
public:
    Win32Exception(const char* function, int code) 
    {
        _what = format("Win32 API \"{}\" failed with code {}", function, code);
    }

    const char* what() const noexcept override
    {
        return _what.c_str();
    }

private:
    string _what;
};