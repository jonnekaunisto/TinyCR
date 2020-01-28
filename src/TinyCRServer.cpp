/**
 * Holds TinyCRClient implementation
 * @author Xiaofeng Shi, Jonne Kaunisto 
 */

#include "../include/TinyCRServer.h"
#include <thread>

TinyCRServer::TinyCRServer(int port, vector<uint64_t> positive_keys, vector<uint64_t> negative_keys)
{
    this->port = port;
    this->positive_keys = positive_keys;
    this->negative_keys = negative_keys;
    this->daasServer = CRIoT_Control_VO<uint64_t, uint32_t>(positive_keys, negative_keys);
}

bool TinyCRServer::startServer()
{
    std::thread connectionListenerThread(listenForNewDevices);
    std::thread summaryUpdatesThread(sendSummaryUpdates);

    summaryUpdatesThread.join();
    connectionListenerThread.join();
    return 0;
}

bool TinyCRServer::startSocketAPIServer(){
    return false;
}

void TinyCRServer::listenForNewDevices()
{
    try
    {
        ServerSocket server(port);

        while (true)
        {

            ServerSocket new_sock;
            server.accept(new_sock);
            int msg_size = 0;

            try
            {
                while (true)
                {
                    /*send the CRC packets*/
                    vector<vector<uint8_t>> v = dassServer.encoding(dassServer.vo_data);
                    for (int i = 0; i < v.size(); i++)
                    {
                        char *msg;
                        msg = new char[v[i].size()];
                        for (int j = 0; j < v[i].size(); j++)
                        {
                            memcpy(&msg[j], &v[i][j], 1);
                        }
                        new_sock.send(msg, v[i].size());

                        msg_size += v[i].size();

                        delete[] msg;
                    }

                    cout << "size: " << msg_size << endl;
                    /*if finished, close*/
                    new_sock << "finish";
                    break;
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

// TODO: add something to trigger a change and change the code to actually send a summary
void TinyCRServer::sendSummaryUpdates()
{
    while (true)
    {
        // add something to trigger a change
        if (true)
        { 
            std::cout << "Sending Delta Summary...\n";
            try
            {
                char device[1][10] = {"localhost"};
                for (int i = 0; i < 1; i++)
                {
                    ClientSocket client_socket(device[i], 40000);

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
            }
            catch (SocketException &e)
            {
                std::cout << "Exception was caught:" << e.description() << "\n";
            }
        }
    }
}