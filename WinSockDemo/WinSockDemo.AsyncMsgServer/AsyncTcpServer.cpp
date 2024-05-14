#include "AsyncTcpServer.h"

/* AsyncTcpServer::AsyncTcpServer
 * Constructor
 * 
 * Initialze WSA
 * 
 * Param: port - TCP port that listener listens at
 */
AsyncTcpServer::AsyncTcpServer(unsigned short port)
{
    WSADATA wsaData = {};
    int iResult = 0;

    if (port != 0)
    {
        sprintf_s(_listenPort, 6, "%d", port);
        _debug.emplace_back(FormatDebugString("AsyncTcpServer::AsyncTcpServer", "Custom port set"));
    }

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw Win32Exception("WSAStartup", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::AsyncTcpServer", "Constructor completed"));
}

/* AsyncTcpServer::~AsyncTcpServer
 * Destructor
 * 
 * Stop listener if not stopped
 * Cleanup WSA
 */
AsyncTcpServer::~AsyncTcpServer()
{
    if (_bIsRunning) { Stop(); }
    WSACleanup();
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::~AsyncTcpServer", "Destructor completed"));
}

/* AsyncTcpServer::Start
 * Start TCP server
 * 
 * Create listener: socket -> bind -> listen
 * Create listener event
 * Create event handle thread
 */
void AsyncTcpServer::Start()
{
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Enter Start method"));
    
    if (_bIsRunning) return;
    
    ADDRINFOA hints = {};
    int iResult = 0;

    // Get listen address with hints
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, _listenPort, &hints, &_pListenAddr);
    if (iResult != 0)
    {
        throw Win32Exception("getaddrinfo", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Acquired listen address from getaddrinfo"));

    // Create listen socket with listen address
    _listenSocket = socket(_pListenAddr->ai_family, _pListenAddr->ai_socktype, _pListenAddr->ai_protocol);
    if (_listenSocket == INVALID_SOCKET)
    {
        iResult = WSAGetLastError();
        freeaddrinfo(_pListenAddr);
        throw Win32Exception("socket", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket created"));

    // Bind listen socket to address
    iResult = bind(_listenSocket, _pListenAddr->ai_addr, (int)_pListenAddr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        freeaddrinfo(_pListenAddr);
        closesocket(_listenSocket);
        throw Win32Exception("bind", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket bound"));

    freeaddrinfo(_pListenAddr);

    // Start listening
    iResult = listen(_listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("listen", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket listening"));

    // Create listen socket event
    _listenEvent = WSACreateEvent();
    if (_listenEvent == WSA_INVALID_EVENT)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("WSACreateEvent", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket event created"));

    // Select accept and close event of listen socket, listen socket should not issue other events
    if (WSAEventSelect(_listenSocket, _listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
    {
        iResult = WSAGetLastError();
        closesocket(_listenSocket);
        throw Win32Exception("WSAEventSelect", iResult);
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Listen socket event selected"));

    // Put listen socket event into event array
    _allEvents[0] = _listenEvent;
    _allSockets[0] = _listenSocket;
    _iCount = 1;

    // Create event handle thread, it will start running after creation
    _hEventHandleThread = CreateThread(NULL,
                                       0,
                                       _EventHandleThreadStart,
                                       this,
                                       0,
                                       &_dwEventHandleThreadId);
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Event handle thread created"));

    _bIsRunning = true;
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Start", "Server runnig"));
}

/* AsyncTcpServer::Stop
 * Stop TCP server
 *
 * Stop event handle thread
 * Close all sockets and events
 */
void AsyncTcpServer::Stop()
{
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Stop", "Enter Stop method"));
    if (!_bIsRunning) return;

    // Set _bShouldStop to true and event handle thread should stop after at max 1sec timeout
    // This is NOT thread safe as we have no lock here, but since we only have two threads in this demo, it's fine
    _bShouldStop = true;
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Stop", "Waiting for event handle thread exiting"));

    // Wait for thread exit
    WaitForSingleObject(_hEventHandleThread, INFINITE);
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Stop", "Event handle thread exited"));

    // Close all events and sockets
    for (int i = 0; i < _iCount; ++i)
    {
        WSACloseEvent(_allEvents[i]);
        shutdown(_allSockets[i], SD_BOTH);
        closesocket(_allSockets[i]);
    }
    _iCount = 0;
    _bIsRunning = false;
    _bShouldStop = false;
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::Stop", "Server stopped"));
}

/* AsyncTcpServer::ClearMessage
 * Clear received messages
 */
void AsyncTcpServer::ClearMessage()
{
    _messages.clear();
}

/* AsyncTcpServer::ClearDebug
 * Clear debug log
 */
void AsyncTcpServer::ClearDebug()
{
    _debug.clear();
}

/* AsyncTcpServer::isRunning
 * Return listener status
 */
bool AsyncTcpServer::isRunning() const
{
    return _bIsRunning;
}

/* AsyncTcpServer::connectCount
 * Return connected client count
 */
int AsyncTcpServer::connectCount() const
{
    return _iCount - 1;
}

/* AsyncTcpServer::listenAddress
 * Format listener address string
 */
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

/* AsyncTcpServer::eventHandleThreadInfo
 * Format event handle thread ID and handle
 */
string AsyncTcpServer::eventHandleThreadInfo()
{
    if (!_bIsRunning) return string("Server not running");
    return format("ID {} | HANDLE {}", _dwEventHandleThreadId, _hEventHandleThread);
}

/* AsyncTcpServer::PrintMessages
 * Write messages to an output stream
 */
void AsyncTcpServer::PrintMessages(std::ostream& os)
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

/* AsyncTcpServer::PrintDebug
 * Write debug log to an output stream
 */
void AsyncTcpServer::PrintDebug(std::ostream& os)
{
    for (auto& dbg : _debug)
    {
        os << dbg << std::endl;
    }
}

/* AsyncTcpServer::_EventHandleThreadStart
 * Static method wrapper for _HandleEvent()
 */
DWORD WINAPI AsyncTcpServer::_EventHandleThreadStart(void* Param)
{
    AsyncTcpServer* instance = (AsyncTcpServer*)Param;
    instance->_HandleEvent();
    return 0;
}

/* AsyncTcpServer::_HandleEvent
 * Event handle worker method
 * 
 * Calls WSAWaitForMultipleEvents to receive socket events
 * Use _bShouldStop to control method termination
 */
void AsyncTcpServer::_HandleEvent()
{
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Enter _HandleEvent method"));
    while (!_bShouldStop)
    {
        int iIndex = WSAWaitForMultipleEvents(_iCount, _allEvents, false, 1000, false);
        if (iIndex == WSA_WAIT_IO_COMPLETION || iIndex == WSA_WAIT_TIMEOUT)
        {
            // We don't consider timeout as a real issue, the timeout is just used to regularly check if _bShouldStop is set, so that we can stop server
            continue;
        }

        // Acquire event issuing socket and its first event
        iIndex = iIndex - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS currentEvent = {};
        SOCKET currentSocket = _allSockets[iIndex];
        WSAEnumNetworkEvents(currentSocket, _allEvents[iIndex], &currentEvent);

        // FD_ACCEPT: New client connection
        if (currentEvent.lNetworkEvents & FD_ACCEPT)
        {
            _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Accept event received"));
            if (currentEvent.iErrorCode[FD_ACCEPT_BIT] == 0)
            {
                if (_iCount >= WSA_MAXIMUM_WAIT_EVENTS)
                {
                    _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Reached maximum clients"));
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
                    _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Accepted new client"));
                }
            }
        }
        // FD_READ: Socket ready for receiving, should be issued when client sent data
        else if (currentEvent.lNetworkEvents & FD_READ)
        {
            _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Read event received"));
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
                    _messages.emplace_back(Message(buf, clientAddrStr));
                    //send(currentSocket, _ACK_MSG, strlen(_ACK_MSG), 0);
                    _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Received message"));
                }
            }
        }
        // FD_CLOSE: Client closed connection
        else if (currentEvent.lNetworkEvents & FD_CLOSE)
        {
            _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Close event received"));
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
        // FD_WRITE: Socket read for sending, should be issued once client is accepted and re-issued every sending is done
        else if (currentEvent.lNetworkEvents & FD_WRITE)
        {
            _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "Write event received"));
        }
    }
    _debug.emplace_back(FormatDebugString("AsyncTcpServer::_HandleEvent", "_HandleEvent thread exiting"));
}