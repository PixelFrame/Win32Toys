#pragma once

#include "StdAfx.h"
#include "..\WinSockDemo.Common\Message.h"

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    
    void Start();
    void AcceptClient();
    void ReadMessage(vector<Message*>&);
    void Disconnect();
    void Stop();

    bool isListening() const;
    bool isConnected() const;

private:
    SOCKET _listenSocket = INVALID_SOCKET;
    SOCKET _clientSocket = INVALID_SOCKET;
    bool _isListening = false;
    bool _isConnected = false;
    char _clientAddr[INET_ADDRSTRLEN] = "0.0.0.0:0";

    const char* _listenPort = "27015";
    PADDRINFOA _pListenAddr = nullptr;

    const char* _STOP_MSG = "Bye";
    const static int _RECV_BUF_LEN = 1024;
    char _recvBuf[_RECV_BUF_LEN] = {};
};