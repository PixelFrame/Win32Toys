#include "AsyncTcpServer.h"

AsyncTcpServer::AsyncTcpServer()
{
    WSADATA wsaData = {};
    ADDRINFOA hints = {};
    int iResult = 0;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw Win32Exception("WSAStartup", iResult);
    }

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

    _listenSocket = socket(_pListenAddr->ai_family, _pListenAddr->ai_socktype, _pListenAddr->ai_protocol);
    if (_listenSocket == INVALID_SOCKET)
    {
        freeaddrinfo(_pListenAddr);
        WSACleanup();
        throw Win32Exception("socket", WSAGetLastError());
    }
}

AsyncTcpServer::~AsyncTcpServer()
{
    if (_isListening)
    {
        Stop();
    }
}

void AsyncTcpServer::Start()
{
    if (_isListening) return;
    int iResult = 0;
    iResult = bind(_listenSocket, _pListenAddr->ai_addr, (int)_pListenAddr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        freeaddrinfo(_pListenAddr);
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("bind", WSAGetLastError());
    }

    freeaddrinfo(_pListenAddr);

    iResult = listen(_listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("listen", WSAGetLastError());
    }

    _listenEvent = WSACreateEvent();
    if (_listenEvent == WSA_INVALID_EVENT)
    {
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("WSACreateEvent", WSAGetLastError());
    }

    if (WSAEventSelect(_listenSocket, _listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
    {
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("WSAEventSelect", WSAGetLastError());
    }

    _allEvents[0] = _listenEvent;
    _allSockets[0] = _listenSocket;

    _isListening = true;
}

void AsyncTcpServer::_HandleEvent()
{
    int nEvent = 1;
    int nIndex = WSAWaitForMultipleEvents(nEvent, _allEvents, false, WSA_INFINITE, false);
    if (nIndex == WSA_WAIT_IO_COMPLETION || nIndex == WSA_WAIT_TIMEOUT) 
    {
        Stop();
        throw Win32Exception("WSAWaitForMultipleEvents", WSAGetLastError());
    }

    nIndex = nIndex - WSA_WAIT_EVENT_0;
    WSANETWORKEVENTS currentEvent = {};
    SOCKET currentSocket = _allSockets[nIndex];

    WSAEnumNetworkEvents(currentSocket, _allEvents[nIndex], &currentEvent);
    if (currentEvent.lNetworkEvents & FD_ACCEPT)
    {
        if (currentEvent.iErrorCode[FD_ACCEPT_BIT] == 0)
        {
            if (nEvent >= WSA_MAXIMUM_WAIT_EVENTS) 
            {
                return;
            }
            sockaddr_in addr = {};
            int len = sizeof(sockaddr_in);
            SOCKET newClientSocket = accept(_listenSocket, (sockaddr*)&addr, &len);
            if (newClientSocket != INVALID_SOCKET)
            {
                WSAEVENT newClientEvent = WSACreateEvent();
                WSAEventSelect(newClientSocket, newClientEvent, FD_READ | FD_CLOSE | FD_WRITE);
                _allEvents[nEvent] = newClientEvent;
                _allSockets[nEvent] = newClientSocket;
                nEvent++;
            }
        }
    }
    else if (currentEvent.lNetworkEvents & FD_READ) 
    {
        if (currentEvent.iErrorCode[FD_READ_BIT] == 0) 
        {
            char buf[2500];
            ZeroMemory(buf, 2500);
            int nRecv = ::recv(currentSocket, buf, 2500, 0);
            if (nRecv > 0) 
            {
                char strSend[] = "I recvived your message.";
                send(currentSocket, strSend, strlen(strSend), 0);
            }
        }
    }
    else if (currentEvent.lNetworkEvents & FD_CLOSE) 
    {
        WSACloseEvent(_allEvents[nIndex]);
        closesocket(_allSockets[nIndex]);
        for (int j = nIndex; j < nEvent - 1; j++) 
        {
            _allEvents[j] = _allEvents[j + 1];
            _allSockets[j] = _allSockets[j + 1];
        }
        nEvent--;
    }
    else if (currentEvent.lNetworkEvents & FD_WRITE)
    {

    }
}