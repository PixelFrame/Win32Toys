#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

private:
    SOCKET ListenSocket = INVALID_SOCKET;

};