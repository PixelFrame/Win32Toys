#pragma once

#include "..\WinSockDemo.Common\Message.h"
#include "StdAfx.h"

class AsyncTcpServer
{
public:
    AsyncTcpServer(unsigned short port = 0);
    ~AsyncTcpServer();

    void Start();
    void Stop();

    bool isRunning();
    int connectCount();
    string listenAddress();
    string eventHandleThreadInfo();
    void printMessages(std::ostream&);
    void printDebug(std::ostream&);

private:
    vector<Message> _messages;
    vector<string> _debug;

    SOCKET _listenSocket = INVALID_SOCKET;
    SOCKET _allSockets[WSA_MAXIMUM_WAIT_EVENTS] = { INVALID_SOCKET };
    WSAEVENT _listenEvent = NULL;
    WSAEVENT _allEvents[WSA_MAXIMUM_WAIT_EVENTS] = { NULL };
    int _iCount = 0;

    HANDLE _hEventHandleThread = NULL;
    DWORD _dwEventHandleThreadId = 0;
    BOOL _bShouldStop = false;

    bool _bIsRunning = false;
    PADDRINFOA _pListenAddr = nullptr;
    char _listenPort[6] = "27015";

    static constexpr const char* _ACK_MSG = "ACK";
    static constexpr int _RECV_BUFF_MAX = 1024;

    void _HandleEvent();
    static DWORD WINAPI _EventHandleThreadStart(void* Param);
};