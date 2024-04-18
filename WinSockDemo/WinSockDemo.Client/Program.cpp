// Win32 Lib
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

// Standard Lib
#include <iostream>
#include <string>
#include <stdexcept>

using std::cout;
using std::cin;
using std::getline;
using std::endl;
using std::string;
using std::runtime_error;

// Global Variables
const int DEFAULT_BUFLEN = 512;
const char* DEFAULT_PORT = "27015";

WSADATA wsaData              = WSADATA();
SOCKET ConnectSocket         = INVALID_SOCKET;
PADDRINFOA result            = nullptr;
PADDRINFOA ptr               = nullptr;
ADDRINFOA  hints             = ADDRINFOA();
LPCSTR initMsg               = "Hello";
LPCSTR lastMsg               = "Bye";
LPCSTR server                = "localhost";
string userMsg               = string();
char recvbuf[DEFAULT_BUFLEN] = {};
int iResult                  = 0;
int recvbuflen               = DEFAULT_BUFLEN;


void SendMsg(const char* msg)
{
    iResult = send(ConnectSocket, msg, (int)strlen(msg), 0);
    if (iResult == SOCKET_ERROR)
    {
        string err = "send failed with error: " + WSAGetLastError() ;
        closesocket(ConnectSocket);
        WSACleanup();
        throw runtime_error(err);
    }
    cout << "Sent " << iResult << " bytes" << endl;
}

int main()
{
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) 
    {
        cout << "WSAStartup failed with error: " << iResult << endl;
        return 1;
    }

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(server, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) 
    {
        cout << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return 1;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) 
        {
            cout << "socket failed with error: " << WSAGetLastError() << endl;
            WSACleanup();
            return 1;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) 
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) 
    {
        cout << "Unable to connect to server!" << endl;
        WSACleanup();
        return 1;
    }

    try
    {
        userMsg = initMsg;
        while (userMsg != lastMsg)
        {
            SendMsg(userMsg.c_str());
            cout << "Message: ";
            getline(cin, userMsg);
        }
        SendMsg(lastMsg);
    }
    catch(std::exception e)
    {
        cout << e.what() << endl;
        return 1;
    }

    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) 
    {
        cout <<"shutdown failed with error: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    do 
    {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            cout <<"Bytes received: " << iResult << endl;
        else if (iResult == 0)
            cout <<"Connection closed" << endl;
        else
            cout <<"recv failed with error: " << WSAGetLastError() << endl;
    } while (iResult > 0);

    closesocket(ConnectSocket);
    WSACleanup();
}

