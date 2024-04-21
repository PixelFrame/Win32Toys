#pragma once

#include "..\WinSockDemo.Common\Message.h"

class AsyncTcpServer
{
public:
    AsyncTcpServer();
    ~AsyncTcpServer();

    void Start();
    void Stop();

    bool isListening();
    int connectCount();

private:
};