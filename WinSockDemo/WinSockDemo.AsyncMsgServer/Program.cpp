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
    auto server = AsyncTcpServer();
    server.Start();
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
            cout << "Is server running: " << server.isRunning() << endl
                << "Current connection: " << server.connectCount() << endl;
            break;
        case 2:
            server.printMessages(cout);
            break;
        case 3:
            server.printDebug(cout);
            break;
        case 4:
            if (server.isRunning())
            {
                server.Stop();
                cout << "Server stopped" << endl;
            }
            else { cout << "Server not running!" << endl; }
            break;
        case 5:
            server.Stop();
            server.Start();
            cout << "Server restarted" << endl;
            break;
        case 6:
            if (server.isRunning()) 
            {
                server.Stop();
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
    cout << "Program exiting" << endl;
}