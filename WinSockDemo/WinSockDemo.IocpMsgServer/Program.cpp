#include "StdAfx.h"
#include "IocpTcpServer.h"
#include <iostream>

using std::cin;
using std::cout;
using std::endl;

static void printOperations()
{
    cout << "Operations:" << endl
        << "1. Print status" << endl
        << "2. Print messages" << endl
        << "3. Clear messages" << endl
        << "4. Print debug log" << endl
        << "5. Clear debug log" << endl
        << "6. Stop server" << endl
        << "7. Restart server" << endl
        << "8. Exit program" << endl;
}

int main()
{
    IocpTcpServer* pServer = nullptr;
    int userInput = 0;

    try
    {
        pServer = new IocpTcpServer();
        pServer->Start();
    }
    catch (exception& e)
    {
        cout << "Error happened during server start: " << e.what() << endl;
        if (pServer) delete pServer;
        return -1;
    }
    int userChoice = 0xFF;
    bool stopping = false;

    printOperations();
    cout << "-------------------------------------------" << endl;
    while (!stopping)
    {
        cout << "Opeartion: ";
        cin >> userChoice;

        switch (userChoice)
        {
        case 1:
            if (pServer->isRunning())
            {
                cout << "Server is listening at: " << pServer->listenAddress() << endl
                    << pServer->threadsInfo() << endl
                    << "Current connection: " << pServer->connectionCount() << endl;
            }
            else
            {
                cout << "Server is not running..." << endl;
            }
            break;
        case 2:
            pServer->PrintMessages(cout);
            break;
        case 3:
            pServer->ClearMessages();
            break;
        case 4:
            pServer->PrintDebug(cout);
            break;
        case 5:
            pServer->ClearDebug();
            break;
        case 6:
            if (pServer->isRunning())
            {
                pServer->Stop();
                cout << "Server stopped" << endl;
            }
            else { cout << "Server not running!" << endl; }
            break;
        case 7:
            if (pServer->isRunning())
            {
                pServer->Stop();
            }
            pServer->Start();
            cout << "Server restarted" << endl;
            break;
        case 8:
            if (pServer->isRunning())
            {
                pServer->Stop();
                cout << "Server stopped" << endl;
            }
            stopping = true;
            break;
        default:
            cout << "Bad operation!" << endl;
            break;
        }
        cout << "-------------------------------------------" << endl;
        cin.clear();
        cin.ignore(INT_MAX, '\n');
    }

    pServer->Stop();
    delete pServer;
}