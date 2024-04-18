#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#include <iostream>

using std::cin;
using std::cout;
using std::endl;

const int DEFAULT_BUFLEN = 512;
const char* DEFAULT_PORT = "27015";

WSADATA wsaData              = {};
int iResult                  = 0;
SOCKET ListenSocket          = INVALID_SOCKET;
SOCKET ClientSocket          = INVALID_SOCKET;
PADDRINFOA result            = NULL;
ADDRINFOA hints              = {};
char recvbuf[DEFAULT_BUFLEN] = {};
int recvbuflen               = DEFAULT_BUFLEN;

int main()
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cout << "WSAStartup failed with error: " << iResult << endl;
        return 1;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        cout << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        cout << "socket failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        cout << "bind failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        cout << "listen failed with error: " << WSAGetLastError() << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET)
    {
        cout << "accept failed with error: " << WSAGetLastError() << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do
    {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
        {
            cout << "Client Message: " << recvbuf << endl;
            for (int i = 0; i < iResult; ++i)
            {
                recvbuf[i] = '\0';   // Clear receive buffer for new message
            }
        }
        else if (iResult == 0)
            cout << "Connection closing..." << endl;
        else
        {
            cout << "recv failed with error: " << WSAGetLastError() << endl;
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        cout << "shutdown failed with error: " << WSAGetLastError() << endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}