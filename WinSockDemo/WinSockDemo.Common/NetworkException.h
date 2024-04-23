#pragma once

#include "StdAfx.h"

#include <string>
#include <format>

using std::string;
using std::format;

class NetworkException : public exception
{
public:
    NetworkException(const char* error)
    {
        _what = format("Network error occurred: ", error);
    }

    const char* what() const noexcept override
    {
        return _what.c_str();
    }

private:
    string _what = "";
};