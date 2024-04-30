#include "StdAfx.h"
#include "AsyncTcpServer.h"

#include <iostream>

using std::cout;
using std::cin;
using std::endl;

void printOperations()
{
    cout << "Operations:" << endl
        << "1. Print status" << endl
        << "2. Print messages" << endl
        << "3. Print debug log" << endl
        << "4. Stop server" << endl
        << "5. Restart server" << endl
        << "6. Exit program" << endl;
}

int main()
{
    AsyncTcpServer* pServer = nullptr; 
    try
    {
        pServer = new AsyncTcpServer();
        pServer->Start();
    }
    catch(exception& e)
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
                    << "Event handling thread: " << pServer->eventHandleThreadInfo() << endl
                    << "Current connection: " << pServer->connectCount() << endl;
            }
            else
            {
                cout << "Server is not running..." << endl;
            }
            break;
        case 2:
            pServer->printMessages(cout);
            break;
        case 3:
            pServer->printDebug(cout);
            break;
        case 4:
            if (pServer->isRunning())
            {
                pServer->Stop();
                cout << "Server stopped" << endl;
            }
            else { cout << "Server not running!" << endl; }
            break;
        case 5:
            pServer->Stop();
            pServer->Start();
            cout << "Server restarted" << endl;
            break;
        case 6:
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
    delete pServer;
    cout << "Program exiting" << endl;
}