/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto
 */
#ifndef TinyCRServer_class
#define TinyCRServer_class
#include "../include/CRIoT.h"
#include <thread>
#include <netinet/ip.h>

template<typename K, class V>
class TinyCRServer
{
public:
    TinyCRServer(int port, vector<K> positive_keys, vector<K> negative_keys)
    {
        this->port = port;
        this->positive_keys = positive_keys;
        this->negative_keys = negative_keys;
        daasServer.init(positive_keys, negative_keys);
    }

    /**
     * Starts tiny CR server. Send delta updates on changes, monitor port for new devices
     */
    bool startServer()
    {
        std::thread connectionListenerThread (listenForNewDevices, this);

        connectionListenerThread.join();
        return 0;
    }
    /**
     * Starts socket server for taking in commands from 3rd party
     */
    bool startSocketAPIServer(){
        return false;
    }

    /**
     * Adds a new certificate, value 1 corresponds to valid certificate and
     * 0 to invalid
     */
    bool addCertificate(pair<K,V> kv)
    {
        daasServer.insert(kv);
        return sendSummaryUpdate();
    }

    /**
     * Removes and existing certificate
     */
    bool removeCertificate(K k)
    {
        daasServer.erase(&k);
        return sendSummaryUpdate();
    }

    /**
     * Unrevoke an existing certificate
     */
    bool unrevokeCertificate(K k)
    {
        daasServer.valueFlip(&std::make_pair(k, 1));
        return sendSummaryUpdate();
    }

    /**
     * Revoke and existing certificate
     */
    bool revokeCertificate(K k)
    {
        V rev = 0;
        daasServer.valueFlip(&std::make_pair(k, rev));
        return sendSummaryUpdate();
    }
    

private:
    int port;
    //doubly linked list of connected  devices
    std::list<sockaddr_in> connectedDevices;
    vector<K> positive_keys;
    vector<K> negative_keys;
    CRIoT_Control_VO<K, V> daasServer;
    std::thread summaryUpdatesThread;
    std::thread connectionListenerThread;

    struct
    {
        K key;
        V value;
        //send flipped indexes
    } p;

    /**
     * Deregisters a device, no updates
     */
    int removeDevice(int device);

    static void listenForNewDevices(TinyCRServer *tinyCRServer)
    {
        try
        {
            ServerSocket server(tinyCRServer->port);
            std::cout << "Listening For New Devices: " << tinyCRServer->port << "\n";

            while (true)
            {
                ServerSocket new_sock;
                server.accept(new_sock);
                tinyCRServer->connectedDevices.push_back(server.get_client());
                int msg_size = 0;

                while (true)
                {
                    /*send the CRC packets*/
                    vector<vector<uint8_t>> v = tinyCRServer->daasServer.encoding(tinyCRServer->daasServer.vo_data);
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
        }
        catch (SocketException &e)
        {
            std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
        }
    }


    bool sendSummaryUpdate()
    {
        std::cout << "Sending Delta Summary...\n";
        try
        {
            for (sockaddr_in host : connectedDevices)            
            {
                ClientSocket client_socket(host, 40000);

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
            return false;
        }
        return true;
    }

};
#endif
