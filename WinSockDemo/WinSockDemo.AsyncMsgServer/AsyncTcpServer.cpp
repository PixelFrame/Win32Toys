#include "AsyncTcpServer.h"

AsyncTcpServer::AsyncTcpServer(unsigned short port)
{
    WSADATA wsaData = {};
    int iResult = 0;

    if (port != 0)
    {
        sprintf_s(_listenPort, 6, "%d", port);
        _debug.push_back(FormatDebugString("AsyncTcpServer::AsyncTcpServer", "Custom port set"));
    }

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw Win32Exception("WSAStartup", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::AsyncTcpServer", "Constructor completed"));
}

AsyncTcpServer::~AsyncTcpServer()
{
    if (_bIsRunning) { Stop(); }
    WSACleanup();
    _debug.push_back(FormatDebugString("AsyncTcpServer::~AsyncTcpServer", "Destructor completed"));
}

void AsyncTcpServer::Start()
{
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Enter Start method"));
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
        WSACleanup();
        throw Win32Exception("getaddrinfo", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Acquired listen address from getaddrinfo"));

    _listenSocket = socket(_pListenAddr->ai_family, _pListenAddr->ai_socktype, _pListenAddr->ai_protocol);
    if (_listenSocket == INVALID_SOCKET)
    {
        iResult = WSAGetLastError();
        freeaddrinfo(_pListenAddr);
        WSACleanup();
        throw Win32Exception("socket", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket created"));

    iResult = bind(_listenSocket, _pListenAddr->ai_addr, (int)_pListenAddr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        freeaddrinfo(_pListenAddr);
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("bind", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket bound"));

    freeaddrinfo(_pListenAddr);

    iResult = listen(_listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("listen", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket listening"));

    _listenEvent = WSACreateEvent();
    if (_listenEvent == WSA_INVALID_EVENT)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("WSACreateEvent", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket event created"));

    if (WSAEventSelect(_listenSocket, _listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("WSAEventSelect", iResult);
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket event selected"));

    _allEvents[0] = _listenEvent;
    _allSockets[0] = _listenSocket;
    _iCount = 1;

    _hEventHandleThread = CreateThread(NULL,
                                       0,
                                       _EventHandleThreadStart,
                                       this,
                                       0,
                                       &_dwEventHandleThreadId);
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Event handle thread created"));

    _bIsRunning = true;
    _debug.push_back(FormatDebugString("AsyncTcpServer::Start", "Server runnig"));
}

void AsyncTcpServer::Stop()
{
    _debug.push_back(FormatDebugString("AsyncTcpServer::Stop", "Enter Stop method"));
    if (!_bIsRunning) return;
    _bShouldStop = true;
    _debug.push_back(FormatDebugString("AsyncTcpServer::Stop", "Waiting for event handle thread exiting"));
    WaitForSingleObject(_hEventHandleThread, INFINITE);
    _debug.push_back(FormatDebugString("AsyncTcpServer::Stop", "Event handle thread exited"));
    for (int i = 0; i < _iCount; ++i)
    {
        WSACloseEvent(_allEvents[i]);
        shutdown(_allSockets[i], SD_BOTH);
        closesocket(_allSockets[i]);
    }
    _iCount = 0;
    _bIsRunning = false;
    _bShouldStop = false;
    _debug.push_back(FormatDebugString("AsyncTcpServer::Stop", "Server stopped"));
}

bool AsyncTcpServer::isRunning()
{
    return _bIsRunning;
}

int AsyncTcpServer::connectCount()
{
    return _iCount - 1;
}

string AsyncTcpServer::listenAddress()
{
    if (!_bIsRunning) return string("Server not running");
    SOCKADDR_IN serverAddr = { 0 };
    char serverAddrStr[INET_ADDRSTRLEN] = { 0 };
    int serverAddrLen = sizeof(serverAddr);
    getsockname(_listenSocket, (sockaddr*) &serverAddr, &serverAddrLen);
    inet_ntop(serverAddr.sin_family, &(serverAddr.sin_addr), serverAddrStr, INET_ADDRSTRLEN);
    unsigned short port = ntohs(serverAddr.sin_port);
    sprintf_s(serverAddrStr, INET_ADDRSTRLEN, "%s:%d", serverAddrStr, port);
    return string(serverAddrStr);
}

string AsyncTcpServer::eventHandleThreadInfo()
{
    if (!_bIsRunning) return string("Server not running");
    return format("ID {} | HANDLE {}", _dwEventHandleThreadId, _hEventHandleThread);
}

void AsyncTcpServer::printMessages(std::ostream& os)
{
    if (_messages.size() == 0)
    {
        os << "No messages received yet." << std::endl;
        return;
    }
    for (auto msg : _messages)
    {
        os << msg.to_string() << std::endl;
    }
}

void AsyncTcpServer::printDebug(std::ostream& os)
{
    for (auto dbg : _debug)
    {
        os << dbg << std::endl;
    }
}

DWORD WINAPI AsyncTcpServer::_EventHandleThreadStart(void* Param)
{
    AsyncTcpServer* instance = (AsyncTcpServer*)Param;
    instance->_HandleEvent();
    return 0;
}

void AsyncTcpServer::_HandleEvent()
{
    _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Enter _HandleEvent method"));
    while (!_bShouldStop)
    {
        int iIndex = WSAWaitForMultipleEvents(_iCount, _allEvents, false, 1000, false);
        if (iIndex == WSA_WAIT_IO_COMPLETION || iIndex == WSA_WAIT_TIMEOUT)
        {
            // We don't consider timeout as a real issue, the timeout is just used to regularly check if _bShouldStop is set, so that we can stop server
            continue;
        }

        iIndex = iIndex - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS currentEvent = {};
        SOCKET currentSocket = _allSockets[iIndex];

        WSAEnumNetworkEvents(currentSocket, _allEvents[iIndex], &currentEvent);
        if (currentEvent.lNetworkEvents & FD_ACCEPT)
        {
            _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Accept event received"));
            if (currentEvent.iErrorCode[FD_ACCEPT_BIT] == 0)
            {
                if (_iCount >= WSA_MAXIMUM_WAIT_EVENTS)
                {
                    _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Reached maximum clients"));
                    continue;
                }
                sockaddr_in addr = {};
                int len = sizeof(sockaddr_in);
                SOCKET newClientSocket = accept(_listenSocket, (sockaddr*)&addr, &len);
                if (newClientSocket != INVALID_SOCKET)
                {
                    WSAEVENT newClientEvent = WSACreateEvent();
                    WSAEventSelect(newClientSocket, newClientEvent, FD_READ | FD_CLOSE | FD_WRITE);
                    _allEvents[_iCount] = newClientEvent;
                    _allSockets[_iCount] = newClientSocket;
                    _iCount++;
                    _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Accepted new client"));
                }
            }
        }
        else if (currentEvent.lNetworkEvents & FD_READ)
        {
            _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Read event received"));
            if (currentEvent.iErrorCode[FD_READ_BIT] == 0)
            {
                char buf[_RECV_BUFF_MAX] = { 0 };
                int iRecv = recv(currentSocket, buf, _RECV_BUFF_MAX, 0);
                if (iRecv > 0)
                {
                    SOCKADDR_IN clientAddr = { 0 };
                    char clientAddrStr[INET_ADDRSTRLEN] = {0};
                    int clientAddrLen = sizeof(clientAddr);
                    getpeername(currentSocket, (sockaddr*) &clientAddr, &clientAddrLen);
                    inet_ntop(clientAddr.sin_family, &(clientAddr.sin_addr), clientAddrStr, INET_ADDRSTRLEN);
                    unsigned short port = ntohs(clientAddr.sin_port);
                    sprintf_s(clientAddrStr, INET_ADDRSTRLEN, "%s:%d", clientAddrStr, port);
                    _messages.push_back(Message(buf, clientAddrStr));
                    //send(currentSocket, _ACK_MSG, strlen(_ACK_MSG), 0);
                    _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Received message"));
                }
            }
        }
        else if (currentEvent.lNetworkEvents & FD_CLOSE)
        {
            _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Close event received"));
            WSACloseEvent(_allEvents[iIndex]);
            shutdown(_allSockets[iIndex], SD_BOTH);
            closesocket(_allSockets[iIndex]);
            _iCount--;
            for (int j = iIndex; j < _iCount; j++)
            {
                _allEvents[j] = _allEvents[j + 1];
                _allSockets[j] = _allSockets[j + 1];
            }
            _allEvents[_iCount] = NULL;
            _allSockets[_iCount] = INVALID_SOCKET;
        }
        else if (currentEvent.lNetworkEvents & FD_WRITE)
        {
            _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Write event received"));
        }
    }
    _debug.push_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "_HandleEvent thread exiting"));
}