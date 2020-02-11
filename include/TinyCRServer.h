/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto
 */
#ifndef TinyCRServer_class
#define TinyCRServer_class
#include "../include/CRIoT.h"

template<typename K, class V>
class TinyCRServer
{
public:
    TinyCRServer(int port, vector<K> positive_keys, vector<V> negative_keys)
    {
        this->port = port;
        this->positive_keys = positive_keys;
        this->negative_keys = negative_keys;
        CRIoT_Control_VO<uint64_t, uint32_t>daasServer(positive_keys, negative_keys);
    }

    /**
     * Starts tiny CR server. Send delta updates on changes, monitor port for new devices
     */
    bool startServer()
    {
        std::thread connectionListenerThread(listenForNewDevices);
        std::thread summaryUpdatesThread(sendSummaryUpdates);

        summaryUpdatesThread.join();
        connectionListenerThread.join();
        return 0;
    }
    /**
     * Starts socket server for taking in commands from 3rd party
     */
    bool startSocketAPIServer(){
        return false;
    }

private:
    int port;
    vector<uint64_t> positive_keys;
    vector<uint64_t> negative_keys;
    CRIoT_Control_VO<K, V> daasServer;
    /**
     * Registers a device that is authorized and send the delta summary to all devices
     */
    int addDeviceToGroup(int device);
    /**
     * Registers a device that is unauthorized and send the delta summary to all devices
     */
    int removeDeviceFromGroup(int device);
    /**
     * Deregisters a device, no updates
     */
    int removeDevice(int device);

    void listenForNewDevices()
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
                        vector<vector<uint8_t>> v = daasServer.encoding(daasServer.vo_data);
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


    void sendSummaryUpdates()
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
};
#endif
