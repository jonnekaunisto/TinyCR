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
#include <regex>

#define DEVICE_PORT 40000
#define COMMAND_PORT 50000


template<typename K, class V>
class TinyCRServer
{
public:
    /**
     * Constructor for TinyCRServer
     * @param port The port that the server should run on
     * @param positive_keys The list of unrevoked keys.
     * @param negative_keys The list of revoked keys.
     */
    TinyCRServer(int port, vector<K> positive_keys, vector<K> negative_keys)
    {
        this->port = port;
        this->positive_keys = positive_keys;
        this->negative_keys = negative_keys;
        daasServer.init(positive_keys, negative_keys);
    }

    /**
     * Starts tiny CR server. Send delta updates on changes, monitor port for new devices
     * and start a socket server for communication from the Certificate Authority.
     */
    void startServer()
    {
        std::thread connectionListenerThread (listenForNewDevices, this);
        std::thread commandListenerThread (listenForCACommands, this);

        connectionListenerThread.join();
        commandListenerThread.join();
    }

    /**
     * Adds a new certificate, value 1 corresponds to valid certificate and
     * 0 to invalid
     * @param kv Key value pair of the key that should be added and the value it should hold
     * @return Status of adding the certificate.
     */
    bool addCertificate(pair<K,V> kv)
    {
        bool status = daasServer.insert(kv);
        if (status) 
        {
            return sendSummaryUpdate(kv, uint8_t(0));
        }
        else
        {
            return sendFullUpdates();
        } 
    }

    /**
     * Removes and existing certificate
     * NOT IMPLEMENTED
     * @param k The key that should be removed.
     * @return Status of removing the certificate.
     */
    bool removeCertificate(K k)
    {
        pair<K, V> kv(k, 0);
        K& kref = k;
        daasServer.erase(kref);
        return sendSummaryUpdate(kv, uint8_t(1));
    }

    /**
     * Unrevoke an existing certificate
     * @param k Key that should be unrevoked.
     * @return Status of unrevoking the certificate.
     */
    bool unrevokeCertificate(K k)
    {
        pair<K, V> kv(k, 1);
        pair<K, V>& kvref = kv;
        daasServer.valueFlip(kvref);
        return sendSummaryUpdate(kv, uint8_t(2));
    }

    /**
     * Revoke and existing certificate
     * @param k Key that should be revoked.
     * @return Status of revoking the certificate.
     */
    bool revokeCertificate(K k)
    {
        pair<K, V> kv(k, 0);
        daasServer.valueFlip(std::ref(kv));
        return sendSummaryUpdate(kv, uint8_t(3));
    }

private:
    int port;
    std::list<std::string> connectedDevices; //doubly linked list of connected  devices
    vector<K> positive_keys;
    vector<K> negative_keys;
    CRIoT_Control_VO<K, V> daasServer;
    std::thread summaryUpdatesThread;
    std::thread connectionListenerThread;

    /**
     * Converts a sockaddr_in to a string.
     * @param in The sockaddr that should be converted
     * @return The string form of sockaddr
     */
    std::string convertSockaddrToStr(sockaddr_in in)
    {
        in_addr ip_address = ((sockaddr_in)in).sin_addr;
        std::string str(inet_ntoa(ip_address));
        return str;
    }

    /* Listens for commands from CA
     * valid commands are:
     * "add {k} {v} "
     * "rem {k} "
     * "unr {k} "
     * "rev {k} "
     * "exi"
     */
    static void listenForCACommands(TinyCRServer *tinyCRServer)
    {
        std::regex rgx("([a-z]{3}) ([0-9]+)(?: ([0-9]+))?");
        ServerSocket server(COMMAND_PORT);
        std::cout << "Listening For Commands on Port: " << COMMAND_PORT << "\n";

        while (true)
        {
            try
            {
                ServerSocket new_sock;
                server.accept(new_sock);

                std::string data;
                new_sock >> data;
                std::smatch matches;
                if(!std::regex_search(data, matches, rgx)) 
                {
                    new_sock << "Command not recognized";
                    continue;
                
                }
                std::cout << "Received command: " << data << "\n";
                std::string command = matches[1];
                if(command == "add" && matches[3].length() > 0)
                {
                    pair<K, V> kv(static_cast<K>(std::stoul(matches[2])), static_cast<V>(std::stoul(matches[3])));
                    tinyCRServer->addCertificate(kv);
                    new_sock << "added";
                }
                else if(command == "rem")
                {
                    //TODO fix
                    //tinyCRServer->removeCertificate(tinyCRServer->parseCommandNum(data));
                    new_sock << "Not implemented";
                }
                else if(command == "unr")
                {
                    tinyCRServer->unrevokeCertificate(static_cast<K>(std::stoul(matches[2])));
                    new_sock << "unrevoked";

                }
                else if(command == "rev")
                {
                    tinyCRServer->revokeCertificate(static_cast<K>(std::stoul(matches[2])));
                    new_sock << "revoked";

                }
                else
                {
                    new_sock << "Command not recognized";
                }
            }
            catch (SocketException &e)
            {
                std::cout << "Exception was caught:" << e.description() << "\n";
            }  
        }
    }

    void sendFullUpdate(ServerSocket socket)
    {
        int msg_size = 0;
        /*send the CRC packets*/
        vector<vector<uint8_t>> v = daasServer.encode();
        for (int i = 0; i < v.size(); i++)
        {
            char *msg;
            msg = new char[v[i].size()];
            for (int j = 0; j < v[i].size(); j++)
            {
                memcpy(&msg[j], &v[i][j], 1);
            }
            socket.send(msg, v[i].size());

            msg_size += v[i].size();

            delete[] msg;
        }

        cout << "size: " << msg_size << endl;
        /*if finished, close*/
        socket << "finish";
    }

    void sendFullUpdate(Socket socket)
    {
        int msg_size = 0;
        /*send the CRC packets*/
        vector<vector<uint8_t>> v = daasServer.encode();
        for (int i = 0; i < v.size(); i++)
        {
            char *msg;
            msg = new char[v[i].size()];
            for (int j = 0; j < v[i].size(); j++)
            {
                memcpy(&msg[j], &v[i][j], 1);
            }
            socket.send(msg, v[i].size());

            msg_size += v[i].size();

            delete[] msg;
        }

        cout << "size: " << msg_size << endl;
        /*if finished, close*/
        socket.send("finish");
    }

    /**
     * Listens for new devices and sends the DASS
     */
    static void listenForNewDevices(TinyCRServer *tinyCRServer)
    {
        ServerSocket server(tinyCRServer->port);
        std::cout << "Listening For New Devices: " << tinyCRServer->port << "\n";

        while (true)
        {
            try
            {
                ServerSocket new_sock;
                server.accept(new_sock);
                std::string device_ip_str = tinyCRServer->convertSockaddrToStr(server.get_client());
                tinyCRServer->connectedDevices.push_back(device_ip_str);
                std::cout << "added new device at " << device_ip_str << "\n";
                
                tinyCRServer->sendFullUpdate(new_sock);
            } 

            catch (SocketException &e)
            {
                std::cout << "Exception was caught:" << e.description() << "\n";
            }  
        }
    }

    /**
     * Sends a summary update based on the action.
     * @param kv Key value pair that is mutated.
     * @param action Action that is being done to the key value pair.
     * @return Status of sending the update.
     */
    bool sendSummaryUpdate(pair<K,V> kv, uint8_t action)
    {
        if(connectedDevices.empty()){
            return true;
        }

        std::cout << "Sending Delta Summary...\n";
        try
        {
            vector<uint8_t> v = daasServer.encode_summary(kv, action);
            
            for (std::string host : connectedDevices)            
            {
                ClientSocket client_socket(host, DEVICE_PORT);
                std::cout << "Sending Summary To " << host << "\n";
                std::string reply;

                char *msg;
                msg = new char[v.size()];
                memcpy(msg, &v[0], v.size());
                std::cout << v.size() << " sent\n";
                client_socket.send(msg, v.size());
                delete[] msg;

                client_socket >> reply;

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

    bool sendFullUpdates()
    {
        if(connectedDevices.empty()){
            return true;
        }

        try
        {
            vector<vector<uint8_t>> v = daasServer.encode();

            for (std::string host : connectedDevices)            
            {
                ClientSocket client_socket(host, DEVICE_PORT);
                std::cout << "Sending Full Update to " << host << "\n";
                int msg_size = 0;

                client_socket << "F";
                /*send the CRC packets*/
                vector<vector<uint8_t>> v = daasServer.encode();
                for (int i = 0; i < v.size(); i++)
                {
                    char *msg;
                    msg = new char[v[i].size()];
                    for (int j = 0; j < v[i].size(); j++)
                    {
                        memcpy(&msg[j], &v[i][j], 1);
                    }
                    client_socket.send(msg, v[i].size());

                    msg_size += v[i].size();

                    delete[] msg;
                }

                cout << "size: " << msg_size << endl;
                /*if finished, close*/
                client_socket << "finish";
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
