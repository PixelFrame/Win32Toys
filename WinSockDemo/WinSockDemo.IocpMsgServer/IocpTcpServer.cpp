#include "IocpTcpServer.h"

IocpTcpServer::IocpTcpServer(unsigned short port)
{
    WSADATA wsaData = {};
    int iResult = 0;

    if (port != 0)
    {
        sprintf_s(_listenPort, 6, "%d", port);
        _debug.emplace_back(FormatDebugString("IocpTcpServer::IocpTcpServer", "Custom port set"));
    }

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw Win32Exception("WSAStartup", iResult);
    }

    InitializeCriticalSection(&_csLock);

    _debug.emplace_back(FormatDebugString("IocpTcpServer::IocpTcpServer", "Constructor completed"));

}

IocpTcpServer::~IocpTcpServer()
{
    if (_bIsRunning) { Stop(); }
    DeleteCriticalSection(&_csLock);
    WSACleanup();
    _debug.emplace_back(FormatDebugString("IocpTcpServer::~IocpTcpServer", "Destructor completed"));
}

void IocpTcpServer::Start()
{
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Enter Start method"));

    if (_bIsRunning) return;

    ADDRINFOA hints = {};
    int iResult = 0;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, _listenPort, &hints, &_pListenAddr);
    if (iResult != 0)
    {
        throw Win32Exception("getaddrinfo", iResult);
    }
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Acquired listen address from getaddrinfo"));

    _listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (_listenSocket == INVALID_SOCKET) {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("WSASocket", iResult);
    }

    DWORD dwMode = 1;
    iResult = ioctlsocket(_listenSocket, FIONBIO, &dwMode);
    if (iResult == SOCKET_ERROR) {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("ioctlsocket", iResult);
    }
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Listen socket mode set to FIONBIO"));

    iResult = bind(_listenSocket, _pListenAddr->ai_addr, (int)_pListenAddr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        freeaddrinfo(_pListenAddr);
        closesocket(_listenSocket);
        throw Win32Exception("bind", iResult);
    }
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Listen socket bound"));

    freeaddrinfo(_pListenAddr);

    iResult = listen(_listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("listen", iResult);
    }
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Listen socket listening"));

    _hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _NUM_THREAD);
    if (_hIocp == NULL)
    {
        iResult = GetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("CreateIoCompletionPort", iResult);
    }

    for (int i = 0; i < _NUM_THREAD; ++i)
    {
        _hWorkerThreads[i] = CreateThread(NULL,
            0,
            _WorkerThreadStart,
            this,
            0,
            &_dwWorkerThreadIds[i]);
    }
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Worker threads started"));

    _hAcceptThread = CreateThread(NULL,
        0,
        _AcceptThreadStart,
        this,
        0,
        &_dwAcceptThreadId);
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Accept thread started"));
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Start", "Server started"));

    _bIsRunning = true;
}

void IocpTcpServer::Stop()
{
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Stop", "Enter Stop method"));
    _bStopServer = true;

    //
    // Cause accept thread exit
    //
    if (_listenSocket != INVALID_SOCKET)
    {
        closesocket(_listenSocket);
        _listenSocket = INVALID_SOCKET;
    }

    if (WAIT_OBJECT_0 != WaitForSingleObject(_hAcceptThread, 1000))
    {
        throw Win32Exception("WaitForSingleObject", GetLastError());
    }
    else
    {
        _hAcceptThread = INVALID_HANDLE_VALUE;
        _debug.emplace_back(FormatDebugString("IocpTcpServer::Stop", "Accept thread stopped"));
    }

    //
    // Cause worker threads to exit
    //
    if (_hIocp)
    {
        for (DWORD i = 0; i < _NUM_THREAD; i++)
        {
            PostQueuedCompletionStatus(_hIocp, 0, 0, NULL);
        }
    }

    //
    //Make sure worker threads exits.
    //
    if (WAIT_OBJECT_0 != WaitForMultipleObjects(_NUM_THREAD, _hWorkerThreads, TRUE, 1000))
    {
        throw Win32Exception("WaitForMultipleObjects", GetLastError());
    }
    else
    {
        for (DWORD i = 0; i < _NUM_THREAD; i++)
        {
            if (_hWorkerThreads[i] != INVALID_HANDLE_VALUE) CloseHandle(_hWorkerThreads[i]);
            _hWorkerThreads[i] = INVALID_HANDLE_VALUE;
        }
        _debug.emplace_back(FormatDebugString("IocpTcpServer::Stop", "Worker threads stopped"));
    }

    FreeContextList();

    if (_hIocp) 
    {
        CloseHandle(_hIocp);
        _hIocp = NULL;
        _debug.emplace_back(FormatDebugString("IocpTcpServer::Stop", "IOCP handle closed"));
    }

    _bIsRunning = false;
    _bStopServer = false;
    _debug.emplace_back(FormatDebugString("IocpTcpServer::Stop", "Server stopped"));
}

void IocpTcpServer::PrintMessages(std::ostream& os)
{
    if (_messages.size() == 0)
    {
        os << "No messages received yet." << std::endl;
        return;
    }
    for (auto& msg : _messages)
    {
        os << msg.to_string() << std::endl;
    }
}

void IocpTcpServer::PrintDebug(std::ostream& os)
{
    for (auto& dbg : _debug)
    {
        os << dbg << std::endl;
    }
}

void IocpTcpServer::ClearMessages()
{
    _messages.clear();
}

void IocpTcpServer::ClearDebug()
{
    _debug.clear();
}

bool IocpTcpServer::isRunning() const
{
    return _bIsRunning;
}

string IocpTcpServer::listenAddress() const
{
    if (!_bIsRunning) return string("Server not running");

    SOCKADDR_IN serverAddr = { 0 };
    char serverAddrStr[INET_ADDRSTRLEN] = { 0 };
    int serverAddrLen = sizeof(serverAddr);
    getsockname(_listenSocket, (sockaddr*)&serverAddr, &serverAddrLen);
    inet_ntop(serverAddr.sin_family, &(serverAddr.sin_addr), serverAddrStr, INET_ADDRSTRLEN);
    unsigned short port = ntohs(serverAddr.sin_port);
    sprintf_s(serverAddrStr, INET_ADDRSTRLEN, "%s:%d", serverAddrStr, port);
    return string(serverAddrStr);
}

string IocpTcpServer::threadsInfo() const
{
    if (!_bIsRunning) return "Server not running";

    string res = format("Accept thread: ID {} | Handle {}\n", _dwAcceptThreadId, _hAcceptThread);
    res += "Worker threads:\n";
    for (int i = 0; i < _NUM_THREAD; ++i)
    {
        res += format("    ID {}\t| Handle {}\n", _dwWorkerThreadIds[i], _hWorkerThreads[i]);
    }
    return res;
}

int IocpTcpServer::connectionCount() const
{
    int count = 0;
    PER_SOCKET_CONTEXT* it = _contextList;
    while (it != nullptr)
    {
        count++;
        it = it->pCtxtBack;
    }
    return count;
}

void IocpTcpServer::_AcceptThread()
{
    _debug.emplace_back(FormatDebugString("IocpTcpServer::_AcceptThread", "Accept thread start"));
    while (!_bStopServer)
    {
        DWORD dwRecvBytes = 0;
        DWORD dwFlags = 0;

        SOCKET clientSocket = WSAAccept(_listenSocket, nullptr, nullptr, nullptr, 0);
        if (INVALID_SOCKET == clientSocket) continue;

        PER_SOCKET_CONTEXT* pSocketContext = UpdateCompletionPort(clientSocket, IO_OPERATION::ClientIoRead, TRUE);
        _debug.emplace_back(FormatDebugString("IocpTcpServer::_AcceptThread", "Updated IOCP for new client"));
        if (pSocketContext == nullptr)
        {
            shutdown(clientSocket, SD_BOTH);
            closesocket(clientSocket);
            continue;
        }

        int nRet = WSARecv(clientSocket, &pSocketContext->pIOContext->wsabuf, 1, &dwRecvBytes, &dwFlags, &pSocketContext->pIOContext->Overlapped, nullptr);
        if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
        {
            CloseClient(pSocketContext, FALSE);
        }
    }
}
DWORD WINAPI IocpTcpServer::_AcceptThreadStart(void* Param)
{
    IocpTcpServer* instance = (IocpTcpServer*)Param;
    instance->_AcceptThread();
    return 0;
}

void IocpTcpServer::_WorkerThread()
{
    BOOL bSuccess = FALSE;
    int nRet = 0;
    WSAOVERLAPPED* lpOverlapped = nullptr;
    PER_SOCKET_CONTEXT* lpPerSocketContext = nullptr;
    PER_IO_CONTEXT* lpIOContext = nullptr;
    WSABUF buffRecv{};
    WSABUF buffSend{};
    DWORD dwRecvNumBytes = 0;
    DWORD dwSendNumBytes = 0;
    DWORD dwFlags = 0;
    DWORD dwIoSize = 0;

    SOCKADDR_IN clientAddr{};
    char clientAddrStr[INET_ADDRSTRLEN]{};
    int clientAddrLen = sizeof(clientAddr);
    unsigned short clientPort = 0;

    while (TRUE) 
    {
        //
        // continually loop to service io completion packets
        //
        bSuccess = GetQueuedCompletionStatus(_hIocp, &dwIoSize,
            (PDWORD_PTR)&lpPerSocketContext,
            (LPOVERLAPPED*)&lpOverlapped,
            INFINITE);
        if (!bSuccess)
        {
        }

        if (lpPerSocketContext == NULL)
        {
            //
            // CTRL-C handler used PostQueuedCompletionStatus to post an I/O packet with
            // a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
            //
            return;
        }

        if (_bStopServer)
        {
            //
            // main thread will do all cleanup needed - see finally block
            //
            return;
        }

        if (!bSuccess || (bSuccess && (dwIoSize == 0)))
        {
            //
            // client connection dropped, continue to service remaining (and possibly 
            // new) client connections
            //
            CloseClient(lpPerSocketContext, FALSE);
            continue;
        }

        //
        // determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
        // associated with this socket.  This will determine what action to take.
        //
        lpIOContext = (PER_IO_CONTEXT*)lpOverlapped;
        switch (lpIOContext->IOOperation)
        {
        case IO_OPERATION::ClientIoRead:
            //
            // a read operation has completed, post a write operation to echo the
            // data back to the client using the same data buffer.
            //

            getpeername(lpPerSocketContext->Socket, (sockaddr*)&clientAddr, &clientAddrLen);
            inet_ntop(clientAddr.sin_family, &(clientAddr.sin_addr), clientAddrStr, INET_ADDRSTRLEN);
            clientPort = ntohs(clientAddr.sin_port);
            sprintf_s(clientAddrStr, INET_ADDRSTRLEN, "%s:%d", clientAddrStr, clientPort);
            _messages.emplace_back(Message(string(lpIOContext->Buffer, dwIoSize), clientAddrStr));

            lpIOContext->IOOperation = IO_OPERATION::ClientIoWrite;
            lpIOContext->nTotalBytes = 3;
            lpIOContext->nSentBytes = 0;
            lpIOContext->wsabuf.buf[0] = 'A';
            lpIOContext->wsabuf.buf[1] = 'C';
            lpIOContext->wsabuf.buf[2] = 'K';
            lpIOContext->wsabuf.len = 3;
            dwFlags = 0;
            nRet = WSASend(lpPerSocketContext->Socket, &lpIOContext->wsabuf, 1,
                &dwSendNumBytes, dwFlags, &(lpIOContext->Overlapped), nullptr);
            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
            {
                CloseClient(lpPerSocketContext, FALSE);
            }
            break;
        case IO_OPERATION::ClientIoWrite:
            //
            // a write operation has completed, determine if all the data intended to be
            // sent actually was sent.
            //
            lpIOContext->IOOperation = IO_OPERATION::ClientIoWrite;
            lpIOContext->nSentBytes += dwIoSize;
            dwFlags = 0;
            if (lpIOContext->nSentBytes < lpIOContext->nTotalBytes)
            {
                //
                // the previous write operation didn't send all the data,
                // post another send to complete the operation
                //
                buffSend.buf = lpIOContext->Buffer + lpIOContext->nSentBytes;
                buffSend.len = lpIOContext->nTotalBytes - lpIOContext->nSentBytes;
                nRet = WSASend(lpPerSocketContext->Socket, &buffSend, 1,
                    &dwSendNumBytes, dwFlags, &(lpIOContext->Overlapped), nullptr);
                if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                    CloseClient(lpPerSocketContext, FALSE);
                }
            }
            else
            {
                //
                // previous write operation completed for this socket, post another recv
                //
                lpIOContext->IOOperation = IO_OPERATION::ClientIoRead;
                dwRecvNumBytes = 0;
                dwFlags = 0;
                buffRecv.buf = lpIOContext->Buffer,
                    buffRecv.len = MAX_BUFF_SIZE;
                nRet = WSARecv(lpPerSocketContext->Socket, &buffRecv, 1,
                    &dwRecvNumBytes, &dwFlags, &lpIOContext->Overlapped, nullptr);
                if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
                {
                    CloseClient(lpPerSocketContext, FALSE);
                }
            }
            break;
        }
    }
}
DWORD WINAPI IocpTcpServer::_WorkerThreadStart(void* Param)
{
    IocpTcpServer* instance = (IocpTcpServer*)Param;
    instance->_WorkerThread();
    return 0;
}

PER_SOCKET_CONTEXT* IocpTcpServer::UpdateCompletionPort(SOCKET sd, IO_OPERATION ClientIo, BOOL bAddToList)
{
    PER_SOCKET_CONTEXT* pPerSocketContext;

    pPerSocketContext = ContextAllocate(sd, ClientIo);
    if (pPerSocketContext == NULL)
        return nullptr;

    _hIocp = CreateIoCompletionPort((HANDLE)sd, _hIocp, (DWORD_PTR)pPerSocketContext, 0);
    if (_hIocp == NULL)
    {
        if (pPerSocketContext->pIOContext)
            delete pPerSocketContext->pIOContext;
        delete pPerSocketContext;
        return nullptr;
    }

    //
    //The listening socket context (bAddToList is FALSE) is not added to the list.
    //All other socket contexts are added to the list.
    //
    if (bAddToList) AddToContextList(pPerSocketContext);

    return pPerSocketContext;
}

PER_SOCKET_CONTEXT* IocpTcpServer::ContextAllocate(SOCKET sd, IO_OPERATION ClientIo)
{
    EnterCriticalSection(&_csLock);

    PER_SOCKET_CONTEXT* pPerSocketContext = new PER_SOCKET_CONTEXT();
    pPerSocketContext->pIOContext = new PER_IO_CONTEXT();
    if (pPerSocketContext->pIOContext)
    {
        pPerSocketContext->Socket = sd;
        pPerSocketContext->pCtxtBack = NULL;
        pPerSocketContext->pCtxtForward = NULL;

        pPerSocketContext->pIOContext->Overlapped.Internal = 0;
        pPerSocketContext->pIOContext->Overlapped.InternalHigh = 0;
        pPerSocketContext->pIOContext->Overlapped.Offset = 0;
        pPerSocketContext->pIOContext->Overlapped.OffsetHigh = 0;
        pPerSocketContext->pIOContext->Overlapped.hEvent = NULL;
        pPerSocketContext->pIOContext->IOOperation = ClientIo;
        pPerSocketContext->pIOContext->pIOContextForward = NULL;
        pPerSocketContext->pIOContext->nTotalBytes = 0;
        pPerSocketContext->pIOContext->nSentBytes = 0;
        pPerSocketContext->pIOContext->wsabuf.buf = pPerSocketContext->pIOContext->Buffer;
        pPerSocketContext->pIOContext->wsabuf.len = sizeof(pPerSocketContext->pIOContext->Buffer);

        ZeroMemory(pPerSocketContext->pIOContext->wsabuf.buf, pPerSocketContext->pIOContext->wsabuf.len);
    }
    else {
        delete pPerSocketContext;
    }

    LeaveCriticalSection(&_csLock);

    return(pPerSocketContext);
}

void IocpTcpServer::CloseClient(PER_SOCKET_CONTEXT* pSocketContext, BOOL bGraceful)
{
    EnterCriticalSection(&_csLock);
    if (pSocketContext)
    {
        if (!bGraceful)
        {

            //
            // force the subsequent closesocket to be abortative.
            //
            LINGER lingerStruct{};
            lingerStruct.l_onoff = 1;
            lingerStruct.l_linger = 0;
            setsockopt(pSocketContext->Socket, SOL_SOCKET, SO_LINGER,
                (char*)&lingerStruct, sizeof(lingerStruct));
        }
        closesocket(pSocketContext->Socket);
        pSocketContext->Socket = INVALID_SOCKET;
        RemoveFromContextList(pSocketContext);
        pSocketContext = nullptr;
    }

    LeaveCriticalSection(&_csLock);
}

void IocpTcpServer::AddToContextList(PER_SOCKET_CONTEXT* pSocketContext)
{
    PER_SOCKET_CONTEXT* pTemp;

    EnterCriticalSection(&_csLock);

    if (_contextList == NULL)
    {
        //
        // add the first node to the linked list
        //
        pSocketContext->pCtxtBack = NULL;
        pSocketContext->pCtxtForward = NULL;
        _contextList = pSocketContext;
    }
    else
    {
        //
        // add node to head of list
        //
        pTemp = _contextList;

        _contextList = pSocketContext;
        pSocketContext->pCtxtBack = pTemp;
        pSocketContext->pCtxtForward = NULL;

        pTemp->pCtxtForward = pSocketContext;
    }

    LeaveCriticalSection(&_csLock);
}

void IocpTcpServer::RemoveFromContextList(PER_SOCKET_CONTEXT* pSocketContext)
{
    PER_SOCKET_CONTEXT* pBack = nullptr;
    PER_SOCKET_CONTEXT* pForward = nullptr;
    PER_IO_CONTEXT* pNextIO = nullptr;
    PER_IO_CONTEXT* pTempIO = nullptr;

    EnterCriticalSection(&_csLock);

    if (pSocketContext) {
        pBack = pSocketContext->pCtxtBack;
        pForward = pSocketContext->pCtxtForward;


        if ((pBack == NULL) && (pForward == NULL))
        {
            //
            // This is the only node in the list to delete
            //
            _contextList = NULL;
        }
        else if ((pBack == NULL) && (pForward != NULL))
        {
            //
            // This is the start node in the list to delete
            //
            pForward->pCtxtBack = NULL;
            _contextList = pForward;
        }
        else if ((pBack != NULL) && (pForward == NULL))
        {
            //
            // This is the end node in the list to delete
            //
            pBack->pCtxtForward = NULL;
        }
        else if (pBack && pForward)
        {
            //
            // Neither start node nor end node in the list
            //
            pBack->pCtxtForward = pForward;
            pForward->pCtxtBack = pBack;
        }

        //
        // Free all i/o context structures per socket
        //
        pTempIO = pSocketContext->pIOContext;
        do {
            pNextIO = pTempIO->pIOContextForward;
            if (pTempIO)
            {
                //
                //The overlapped structure is safe to free when only the posted i/o has
                //completed. Here we only need to test those posted but not yet received 
                //by PQCS in the shutdown process.
                //
                if (_bStopServer)
                    while (!HasOverlappedIoCompleted((LPOVERLAPPED)pTempIO)) Sleep(0);
                delete pTempIO;
                pTempIO = NULL;
            }
            pTempIO = pNextIO;
        } while (pNextIO);
        delete pSocketContext;
        pSocketContext = NULL;
    }

    LeaveCriticalSection(&_csLock);
}

void IocpTcpServer::FreeContextList()
{
    PER_SOCKET_CONTEXT* pTemp1, * pTemp2;

    EnterCriticalSection(&_csLock);

    pTemp1 = _contextList;
    while (pTemp1)
    {
        pTemp2 = pTemp1->pCtxtBack;
        CloseClient(pTemp1, FALSE);
        pTemp1 = pTemp2;
    }

    LeaveCriticalSection(&_csLock);
}