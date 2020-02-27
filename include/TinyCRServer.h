/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto
 */
#ifndef TinyCRServer_class
#define TinyCRServer_class
#include "../include/CRIoT.h"
#include <thread>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define DEVICE_PORT 40000


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
        std::cout << "joined connections\n"; 
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
        return sendSummaryUpdate(kv);
    }

    /**
     * Removes and existing certificate
     */
    bool removeCertificate(K k)
    {
        pair<K, V> kv(k, 0);
        daasServer.erase(&k);
        return sendSummaryUpdate(kv);
    }

    /**
     * Unrevoke an existing certificate
     */
    bool unrevokeCertificate(K k)
    {
        pair<K, V> kv(k, 1);
        daasServer.valueFlip(&std::make_pair(k, 1));
        return sendSummaryUpdate(kv);
    }

    /**
     * Revoke and existing certificate
     */
    bool revokeCertificate(K k)
    {
        pair<K, V> kv(k, 0);
        daasServer.valueFlip(std::ref(kv));
        return sendSummaryUpdate(kv);
    }
    

private:
    int port;
    //doubly linked list of connected  devices
    std::list<std::string> connectedDevices;
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

    void printSockAddr(sockaddr_in device)
    {
        in_addr ip_address = ((sockaddr_in)device).sin_addr;
        char *addr = inet_ntoa(ip_address);
        std::cout << "IP: " << addr << "\n";
        std::cout << "Port: " << ntohs(device.sin_port) << "\n";
    }

    std::string convert_sockaddr_to_str(sockaddr_in in)
    {
        in_addr ip_address = ((sockaddr_in)in).sin_addr;
        std::string str(inet_ntoa(ip_address));
        return str;
    }

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
                std::string device_ip_str = tinyCRServer->convert_sockaddr_to_str(server.get_client());
                tinyCRServer->connectedDevices.push_back(device_ip_str);
                std::cout << "added new device at " << device_ip_str << "\n";
                int msg_size = 0;

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
            }
        }
        catch (SocketException &e)
        {
            std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
        }
    }


    bool sendSummaryUpdate(pair<K,V> kv)
    {
        if(connectedDevices.empty()){
            return true;
        }

        std::cout << "Sending Delta Summary...\n";
        try
        {
            vector<uint8_t> v = daasServer.encode_summary(kv);
            

            for (std::string host : connectedDevices)            
            {
                ClientSocket client_socket(host, DEVICE_PORT);
                std::cout << "Sending Summary To " << host << "\n";
                std::string reply;
                try
                {
                    char *msg;
                    msg = new char[v.size()];
                    memcpy(msg, &v[0], v.size());
                    std::cout << v.size() << " sent\n";
                    client_socket.send(msg, v.size());
                    delete[] msg;

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
