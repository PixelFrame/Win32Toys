#pragma once

#include "..\WinSockDemo.Common\Message.h"
#include "StdAfx.h"
#include <mutex>

class AsyncTcpServer
{
public:
    AsyncTcpServer(unsigned short port = 0);
    ~AsyncTcpServer();

    // Public control interfaces
    void Start();
    void Stop();
    void ClearMessage();
    void ClearDebug();

    // Public output interfaces
    void PrintMessages(std::ostream&);
    void PrintDebug(std::ostream&);

    // Public properties
    bool isRunning() const;
    int connectCount() const;
    string listenAddress();
    string eventHandleThreadInfo();

private:
    // Received message and debug log
    vector<Message> _messages;
    vector<string> _debug;

    // Sockets and events
    SOCKET _listenSocket = INVALID_SOCKET;
    SOCKET _allSockets[WSA_MAXIMUM_WAIT_EVENTS] = { INVALID_SOCKET };
    WSAEVENT _listenEvent = NULL;
    WSAEVENT _allEvents[WSA_MAXIMUM_WAIT_EVENTS] = { NULL };
    int _iCount = 0;

    // Event handle thread
    HANDLE _hEventHandleThread = NULL;
    DWORD _dwEventHandleThreadId = 0;
    BOOL _bShouldStop = false;

    // Listener properties
    bool _bIsRunning = false;
    PADDRINFOA _pListenAddr = nullptr;
    char _listenPort[6] = "27015";

    // Static consts
    static constexpr const char* _ACK_MSG = "ACK";
    static constexpr int _RECV_BUFF_MAX = 1024;

    // Event handle routines
    void _HandleEvent();
    static DWORD WINAPI _EventHandleThreadStart(void* Param);
};