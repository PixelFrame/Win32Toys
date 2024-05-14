#pragma once

#include "StdAfx.h"
#include <MSWSock.h>

const int MAX_BUFF_SIZE = 8192;
const int MAX_WORKER_THREAD = 16;

enum class IO_OPERATION 
{
    ClientIoAccept,
    ClientIoRead,
    ClientIoWrite
};

struct PER_IO_CONTEXT 
{
    WSAOVERLAPPED               Overlapped;
    char                        Buffer[MAX_BUFF_SIZE];
    WSABUF                      wsabuf;
    int                         nTotalBytes;
    int                         nSentBytes;
    IO_OPERATION                IOOperation;
    SOCKET                      SocketAccept;

    struct PER_IO_CONTEXT* pIOContextForward;
};

struct PER_SOCKET_CONTEXT {
    SOCKET                      Socket;

    LPFN_ACCEPTEX               fnAcceptEx;

    //
    //linked list for all outstanding i/o on the socket
    //
    PER_IO_CONTEXT* pIOContext;
    struct PER_SOCKET_CONTEXT* pCtxtBack;
    struct PER_SOCKET_CONTEXT* pCtxtForward;
};