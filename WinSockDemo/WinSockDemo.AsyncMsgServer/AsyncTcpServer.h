#pragma once

#include "..\WinSockDemo.Common\Message.h"
#include "StdAfx.h"

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
    SOCKET _listenSocket = INVALID_SOCKET;
    SOCKET _allSockets[WSA_MAXIMUM_WAIT_EVENTS] = { INVALID_SOCKET };
    WSAEVENT _listenEvent = NULL;
    WSAEVENT _allEvents[WSA_MAXIMUM_WAIT_EVENTS] = { NULL };

    bool _isListening = false;
    const char* _listenPort = "27015";
    PADDRINFOA _pListenAddr = nullptr;

    void _HandleEvent();
};