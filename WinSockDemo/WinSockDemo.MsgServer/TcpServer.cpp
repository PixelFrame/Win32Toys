#include "TcpServer.h"

TcpServer::TcpServer()
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

TcpServer::~TcpServer()
{
    if (isConnected())
    {
        Disconnect();
    }
    if (isListening())
    {
        Stop();
    }
}

bool TcpServer::isListening() const
{
    return _isListening;
}

bool TcpServer::isConnected() const
{
    return _isConnected;
}

void TcpServer::Start()
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
    _isListening = true;
}

void TcpServer::AcceptClient()
{
    if (_isConnected) return;
    SOCKADDR_IN clientAddr = {};
    int addrSize = sizeof(SOCKADDR_IN);
    _clientSocket = accept(_listenSocket, (PSOCKADDR)&clientAddr, &addrSize);
    if (_clientSocket == INVALID_SOCKET)
    {
        closesocket(_listenSocket);
        WSACleanup();
        throw Win32Exception("accept", WSAGetLastError());
    }
    inet_ntop(AF_INET, &(clientAddr.sin_addr), _clientAddr, INET_ADDRSTRLEN);
    _isConnected = true;
}

void TcpServer::ReadMessage(vector<Message*>& msgs)
{
    if (!_isConnected) return;
    int iResult = 0;

    iResult = recv(_clientSocket, _recvBuf, _RECV_BUF_LEN, 0);
    if (iResult > 0)
    {
        Message* newMsg = new Message(_recvBuf, _clientAddr);
        msgs.push_back(newMsg);

        if (strcmp(_recvBuf, _STOP_MSG) == 0)
        {
            Disconnect();
        }
        else
        {
            for (int i = 0; i < iResult; ++i)
            {
                _recvBuf[i] = '\0';   // Clear receive buffer for new message
            }
        }
    }
    else if (iResult == 0)
    {
        closesocket(_clientSocket);
        WSACleanup();
        throw NetworkException("Connection closed unexpectedly");
    }
    else
    {
        closesocket(_clientSocket);
        WSACleanup();
        throw Win32Exception("recv", WSAGetLastError());
    }
}

void TcpServer::Disconnect()
{
    if (!_isConnected) return;
    int iResult = 0;
    iResult = shutdown(_clientSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR)
    {
        closesocket(_clientSocket);
        WSACleanup();
        throw Win32Exception("shutdown", WSAGetLastError());
    }
    closesocket(_clientSocket);
    _isConnected = false;
}

void TcpServer::Stop()
{
    if (!_isListening) return;
    if (_isConnected) Disconnect();

    closesocket(_listenSocket);
    WSACleanup();
}