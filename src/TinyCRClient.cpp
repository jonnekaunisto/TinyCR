#include "../include/TinyCRClient.h"
#include <thread>

TinyCRClient::TinyCRClient(int serverIP)
{
    this->serverIP = serverIP;
    this->daasClient = CRIoT_Data_VO<uint64_t, uint32_t>();
}

bool TinyCRClient::startClient()
{
    requestInitialSummary();
}

void TinyCRClient::requestInitialSummary()
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

void TinyCRClient::listenForSummaryUpdates()
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
