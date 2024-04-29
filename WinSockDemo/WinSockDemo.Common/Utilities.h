#pragma once

#include <Windows.h>
#include <string>
#include <format>

std::string FormatSystemTimeString(SYSTEMTIME time);

std::string GetCurrentTimeString();

std::string FormatDebugString(const char* function, const char* message);