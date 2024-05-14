#include "StdAfx.h"
#include "IocpTcpServer.h"
#include <iostream>

using std::cin;
using std::cout;
using std::endl;

int main()
{
    IocpTcpServer* pServer = nullptr;
    int userInput = 0;

    try 
    {
        pServer = new IocpTcpServer();
        pServer->Start();
        cin >> userInput;
        pServer->Stop();
    }
    catch(exception& e)
    {

    }
}