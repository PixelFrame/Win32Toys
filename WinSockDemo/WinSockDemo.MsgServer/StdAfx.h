#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")

#include <string>
#include <format>
#include <vector>
#include <stdexcept>

using std::string;
using std::format;
using std::vector;
using std::exception;
using std::runtime_error;

#include "Win32Exception.h"
#include "NetworkException.h"