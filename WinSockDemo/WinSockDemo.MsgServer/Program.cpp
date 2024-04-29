#include "..\WinSockDemo.Common\Message.h"
#include "TcpServer.h"
#include <iostream>

using std::cout;
using std::cin;
using std::endl;

int main()
{
    vector<Message*> msgs = vector<Message*>();
    TcpServer srv = TcpServer();
    string userInputNext = "y";
    try
    {
        srv.Start();
        cout << "Server started, waiting for connection..." << endl;
        while (userInputNext == "y")
        {
            userInputNext = "X";
            srv.AcceptClient();
            cout << "Client connected, receiving messages..." << endl;
            while (srv.isConnected())
            {
                srv.ReadMessage(msgs);
            }
            cout << "Received completed. Messages: " << endl;
            cout << "------------------------------------------------------------------------------" << endl;
            for (auto msg : msgs)
            {
                cout << msg->to_string() << endl;
                delete msg;
            }
            cout << "------------------------------------------------------------------------------" << endl;
            msgs.clear();
            while (userInputNext != "y" && userInputNext != "N")
            {
                cout << "Waiting for next client? y/N ";
                cin >> userInputNext;
                cin.ignore(INT_MAX, '\n');
            }
        }
        srv.Stop();
        cout << "Server stopped" << endl;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
        return -1;
    }
}