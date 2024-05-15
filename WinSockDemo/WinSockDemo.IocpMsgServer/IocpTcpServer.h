#pragma once

#include "StdAfx.h"
#include "Context.h"
#include "../WinSockDemo.Common/Message.h"

class IocpTcpServer
{
public:
    IocpTcpServer(unsigned short port = 0);
    ~IocpTcpServer();

    void Start();
    void Stop();

    void PrintMessages(std::ostream& os);
    void ClearMessages();
    void PrintDebug(std::ostream& os);
    void ClearDebug();

    bool isRunning() const;
    string listenAddress() const;
    string threadsInfo() const;
    int connectionCount() const;

private:
    CRITICAL_SECTION _csLock;
    const static int _NUM_THREAD = 8;

    vector<Message> _messages;
    vector<string> _debug;

    HANDLE _hIocp = NULL;
    HANDLE _hWorkerThreads[_NUM_THREAD] = { NULL };
    DWORD _dwWorkerThreadIds[_NUM_THREAD] = { 0 };
    HANDLE _hAcceptThread = NULL;
    DWORD _dwAcceptThreadId = 0;
    SOCKET _listenSocket = INVALID_SOCKET;
    PER_SOCKET_CONTEXT* _contextList = NULL;

    bool _bIsRunning = false;
    bool _bStopServer = false;
    PADDRINFOA _pListenAddr = nullptr;
    char _listenPort[6] = "27015";

    void _AcceptThread();
    static DWORD WINAPI _AcceptThreadStart(void* Param);

    void _WorkerThread();
    static DWORD WINAPI _WorkerThreadStart(void* Param);

    void CloseClient(PER_SOCKET_CONTEXT* pSocketContext, BOOL bGraceful);
    void AddToContextList(PER_SOCKET_CONTEXT* pSocketContext);
    void RemoveFromContextList(PER_SOCKET_CONTEXT* pSocketContext);
    void FreeContextList();
    PER_SOCKET_CONTEXT* UpdateCompletionPort(SOCKET sd, IO_OPERATION ClientIo, BOOL bAddToList);
    PER_SOCKET_CONTEXT* ContextAllocate(SOCKET sd, IO_OPERATION ClientIo);
};