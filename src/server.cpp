#include "../include/ClientSocket.h"
#include "../include/ServerSocket.h"
#include "../include/SocketException.h"
#include <string>
#include <iostream>
#include <thread>

bool change = false;

void connectionListener(){
    try
    {
        // Create the socket
        ServerSocket server(30000);

        while (true)
        {

            ServerSocket new_sock;
            server.accept(new_sock);

            try
            {
                while (true)
                {
                    std::string data;
                    new_sock >> data;
                    if (data == "Request Summary")
                    {
                        new_sock << "Summary";
                        std::cout << "Sent Summary\n";
                    }
                    else
                    {
                        new_sock << "Some error";
                    }
                }
            }
            catch (SocketException &)
            {
            }
        }
    }
    catch (SocketException &e)
    {
        std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
    }
}

void sendSummaryUpdates(){
    while(true){
        if (change){
            std::cout << "Sending Delta Summary...\n";
            try
            {

                ClientSocket client_socket("localhost", 40000);

                std::string reply;

                try
                {
                    client_socket << "Delta Summary";
                    client_socket >> reply;
                }
                catch (SocketException &)
                {
                }

                std::cout << "We received this response from the client:\n\"" << reply << "\"\n";
            }
            catch (SocketException &e)
            {
                std::cout << "Exception was caught:" << e.description() << "\n";
            }
            change = false;
        }
    }
}

void someRandomWork(){
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(20000));
        std::cout << "Something Changed\n";
        change = true;
    }
}


int main(int argc, char **argv)
{
    std::cout << "running....\n";
    std::thread connectionListenerThread (connectionListener);
    std::thread randomWorkThread (someRandomWork);
    std::thread sendSummaryUpdatesThread (sendSummaryUpdates);

    sendSummaryUpdatesThread.join();
    randomWorkThread.join();
    connectionListenerThread.join();
    return 0;
}