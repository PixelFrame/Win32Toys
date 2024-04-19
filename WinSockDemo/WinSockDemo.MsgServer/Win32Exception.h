#pragma once

#include "StdAfx.h"

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