#include "../include/ClientSocket.h"
#include "../include/SocketException.h"
#include "../include/ServerSocket.h"
#include <iostream>
#include <thread>
#include <string>

void requestSummary()
{
    std::cout << "Requesting Summary...\n";
    try
    {

        ClientSocket client_socket("localhost", 30000);

        std::string reply;

        try
        {
            client_socket << "Request Summary";
            client_socket >> reply;
        }
        catch (SocketException &)
        {
        }

        std::cout << "We received this response from the server:\n\"" << reply << "\"\n";
        ;
    }
    catch (SocketException &e)
    {
        std::cout << "Exception was caught:" << e.description() << "\n";
    }
}

void listenForSummaryUpdates()
{
    std::cout << "Listening For Summary Updates...\n";
    try
    {
        // Create the socket
        ServerSocket server(40000);

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
                    if (data == "Delta Summary")
                    {
                        new_sock << "Received Summary";
                        std::cout << "Received Delta Summary\n";
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

int main(int argc, char **argv)
{
    requestSummary();
    std::thread summaryListenerThread (listenForSummaryUpdates);
    summaryListenerThread.join();
    return 0;
}
