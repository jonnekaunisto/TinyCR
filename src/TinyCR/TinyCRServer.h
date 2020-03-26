/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto
 */
#ifndef TinyCRServer_class
#define TinyCRServer_class
#include "../Socket/ClientSocket.h"
#include "../Socket/ServerSocket.h"
#include "../Socket/SocketException.h"
#include "../platform/CRIoT.h"
#include "../utils/perf_tool.h"
#include <thread>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <regex>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <utility>

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

        commandsMap["add"] = &addCommand;
        commandsMap["rem"] = &removeCommand;
        commandsMap["unr"] = &unrevokeCommand;
        commandsMap["rev"] = &revokeCommand;
        commandsMap["exi"] = &exitCommand;
        commandsMap["ping"] = &pingCommand;
        commandsMap["get"] = &statisticsCommand;

        statistics.addStatistic("calc_latency");
        statistics.addStatistic("full_encoding_latency");
        statistics.addStatistic("delta_encoding_latency");
        statistics.addStatistic("overall_RTT");
    }

    /**
     * Starts tiny CR server. Send delta updates on changes, monitor port for new devices
     * and start a socket server for communication from the Certificate Authority.
     */
    void startServer()
    {
        running = true;
        std::thread connectionListenerThread (listenForNewDevices, this);
        std::thread commandListenerThread (listenForCACommands, this);

        //connectionListenerThread.join();
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
        StopWatch stopWatch = StopWatch();
        bool status = daasServer.insert(kv);
        statistics.addLatency("calc_latency", stopWatch.stop());
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
        StopWatch stopWatch = StopWatch();
        daasServer.erase(k);
        statistics.addLatency("calc_latency", stopWatch.stop());
        return sendSummaryUpdate(pair<K, V>(k, 0), uint8_t(1));
    }

    /**
     * Unrevoke an existing certificate
     * @param k Key that should be unrevoked.
     * @return Status of unrevoking the certificate.
     */
    bool unrevokeCertificate(K k)
    {
        pair<K, V> kv(k, 1);
        StopWatch stopWatch = StopWatch();
        bool status = daasServer.setValue(kv);
        statistics.addLatency("calc_latency", stopWatch.stop());
        if (status) 
        {
            return sendSummaryUpdate(kv, uint8_t(2));
        }
        else
        {
            return sendFullUpdates();
        }
    }

    /**
     * Revoke and existing certificate
     * @param k Key that should be revoked.
     * @return Status of revoking the certificate.
     */
    bool revokeCertificate(K k)
    {
        pair<K, V> kv(k, 0);
        StopWatch stopWatch = StopWatch();
        daasServer.setValue(kv);
        statistics.addLatency("calc_latency", stopWatch.stop());
        return sendSummaryUpdate(kv, uint8_t(3));
    }

    /**
     * Average latnecy of a successful DASS update.
     * @param statistic The name of the statistic
     * @return The value of the statistic
     */
    double getLatencyStatistic(std::string statistic)
    {
        statistics.getAverageLatency(statistic);
    }

private:
    typedef std::string (*CommandFunction)(std::string data, TinyCRServer<K, V>  *tinyCRServer);

    int port;
    bool running;
    std::chrono::duration<double> lastRTT;
    std::list<std::string> connectedDevices; //doubly linked list of connected  devices
    vector<K> positive_keys;
    vector<K> negative_keys;
    CRIoT_Control_VO<K, V> daasServer;
    std::thread summaryUpdatesThread;
    std::thread connectionListenerThread;
    std::mutex updateLock;

    std::unordered_map<std::string, CommandFunction> commandsMap;

    LatencyStatistics statistics;

    double delta_ack_latency = 0;
    int delta_ack_count = 0;

    double full_ack_latency = 0;
    int full_ack_count = 0;

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

    static std::string addCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        std::regex addRgx("([a-z]{3}) ([0-9]+) ([0-9]+)");
        std::smatch matches;
        if(!std::regex_search(data, matches, addRgx)) 
        {
            return "Inputs are badly formed";
        }
        pair<K, V> kv(static_cast<K>(std::stoul(matches[2])), static_cast<V>(std::stoul(matches[3])));
        StopWatch stopWatch = StopWatch();
        tinyCRServer->addCertificate(kv);
        tinyCRServer->statistics.addLatency("overall_RTT", stopWatch.stop());
        std::string time = std::to_string(tinyCRServer->lastRTT.count());
        return "Add Duration: " + time;
    }

    static std::string removeCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        //TODO fix
        std::regex rgx("([a-z]{3}) ([0-9]+)");
        std::smatch matches;
        if(!std::regex_search(data, matches, rgx)) 
        {
            return "Inputs are badly formed";        
        }
        StopWatch stopWatch = StopWatch();
        tinyCRServer->removeCertificate(static_cast<K>(std::stoul(matches[2])));
        tinyCRServer->statistics.addLatency("overall_RTT", stopWatch.stop());
        std::string time = std::to_string(tinyCRServer->lastRTT.count());
        std::string response = "Unr Duration: " + time;
        return response;
    }

    static std::string unrevokeCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        std::regex rgx("([a-z]{3}) ([0-9]+)");
        std::smatch matches;
        if(!std::regex_search(data, matches, rgx)) 
        {
            return "Inputs are badly formed";        
        }
        StopWatch stopWatch = StopWatch();
        tinyCRServer->unrevokeCertificate(static_cast<K>(std::stoul(matches[2])));
        tinyCRServer->statistics.addLatency("overall_RTT", stopWatch.stop());
        std::string time = std::to_string(tinyCRServer->lastRTT.count());
        std::string response = "Unr Duration: " + time;
        return response;
    }

    static std::string revokeCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        std::regex rgx("([a-z]{3}) ([0-9]+)");
        std::smatch matches;
        if(!std::regex_search(data, matches, rgx)) 
        {
            return "Inputs are badly formed";
        }
        StopWatch stopWatch = StopWatch();
        tinyCRServer->revokeCertificate(static_cast<K>(std::stoul(matches[2])));
        tinyCRServer->statistics.addLatency("overall_RTT", stopWatch.stop());
        std::string time = std::to_string(tinyCRServer->lastRTT.count());
        std::string response = "Rev Duration: " + time;
        return response;
    }

    static std::string exitCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        tinyCRServer->running = false;
        return "exiting";
    }

    static std::string pingCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        return "pong";
    }

    static std::string statisticsCommand(std::string data, TinyCRServer *tinyCRServer)
    {
        std::regex rgx("^\\w+ (\\w+)");
        std::smatch matches;
        if(!std::regex_search(data, matches, rgx)) 
        {
            return "Inputs are badly formed";
        }
        return std::to_string(tinyCRServer->statistics.getAverageLatency(matches[1]));
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
        std::regex commandRgx("(^\\w+)");
        ServerSocket server(COMMAND_PORT);
        std::cout << "Listening For Commands on Port: " << COMMAND_PORT << std::endl;

        while (tinyCRServer->running)
        {
            try
            {
                ServerSocket new_sock;
                server.accept(new_sock);
                tinyCRServer->updateLock.lock();
                std::string data;
                new_sock >> data;
                std::smatch matches;
                if(!std::regex_search(data, matches, commandRgx)) 
                {
                    new_sock << "Command not recognized";
                    continue;
                }
                std::cout << "Received command: " << data << std::endl;
                std::string command = matches[1];
                if(tinyCRServer->commandsMap.find(command) != tinyCRServer->commandsMap.end())
                {
                    std::string response = tinyCRServer->commandsMap[command](data, tinyCRServer);
                    new_sock << response;
                }   
                else
                {
                    new_sock << "Bad input";
                }
            }
            catch (SocketException &e)
            {
                std::cout << "Exception was caught when receiving commands:" << e.description() << std::endl;
            }
            tinyCRServer->updateLock.unlock();
        }
        exit(1);
    }

    void sendFullUpdate(Socket socket)
    {
        StopWatch stopWatchFullRTT = StopWatch();
        socket << "F";

        int msg_size = 0;
        /*send the CRC packets*/
        StopWatch stopWatchEncode = StopWatch();
        vector<vector<uint8_t>> v = daasServer.encode_full();
        double time = stopWatchEncode.stop();
        std::cout << "Full encode latency: " << time << std::endl;
        statistics.addLatency("full_encoding_latency", time);
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

        /*if finished, close*/
        socket.send("finish");
        std::string response;
        socket >> response;
        std::cout << "received: " << response << std::endl;

        full_ack_latency = stopWatchFullRTT.stop();
        full_ack_count++;

        if(response.compare("FullDone") != 0){
            std::cout << "Sending Full Update Failed" << std::endl;
        }
    }

    /**
     * Listens for new devices and sends the DASS
     */
    static void listenForNewDevices(TinyCRServer *tinyCRServer)
    {
        ServerSocket server(tinyCRServer->port);
        std::cout << "Listening For New Devices: " << tinyCRServer->port << std::endl;

        while (tinyCRServer->running)
        {
            try
            {
                ServerSocket new_sock;
                server.accept(new_sock);
                tinyCRServer->updateLock.lock();
                std::string device_ip_str = tinyCRServer->convertSockaddrToStr(server.get_client());
                tinyCRServer->connectedDevices.push_back(device_ip_str);
                std::cout << "added new device at " << device_ip_str << std::endl;
                
                tinyCRServer->sendFullUpdate(new_sock);
            } 

            catch (SocketException &e)
            {
                std::cout << "Exception was caught while listening for new devices:" << e.description() << std::endl;
            } 
            tinyCRServer->updateLock.unlock(); 
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

        std::cout << "Sending Delta Summary..." << std::endl;
        StopWatch stopWatch = StopWatch();
        vector<uint8_t> v = daasServer.encode_summary(kv, action);
        statistics.addLatency("delta_encoding_latency", stopWatch.stop());
            
        for (std::string host : connectedDevices)            
        {
            for(int i = 0; i < 10; i++)
            {
                try
                {
                    ClientSocket client_socket(host, DEVICE_PORT);
                    std::cout << "Sending Summary To " << host << std::endl;
                    std::string reply;
                    
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    for (int i = 0; i < v.size(); i++)
                    {
                        char *msg;
                        msg = new char[sizeof(v[i])];
                        memcpy(msg, &v[i], sizeof(v[i]));
                        client_socket.send(msg, 1);
                        delete[] msg;
                    }
                    client_socket << "finish";
                    std::cout << "Sent: " << sizeof(v) << std::endl;

                    client_socket >> reply;
                    auto finish = std::chrono::high_resolution_clock::now();
                    lastRTT = finish - start;
                    std::cout << "We received this response from the client: \"" << reply << "\"" << std::endl;
                    break;
                }
                catch (SocketException &e)
                {
                    std::cout << "Exception was caught while sending a summary update:" << e.description() << std::endl;
                    std::cout << "Trying again #" << i << std::endl;
                    exit(1);
                }
            }
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
            StopWatch stopWatch = StopWatch();
            vector<vector<uint8_t>> v = daasServer.encode_full();
            statistics.addLatency("full_encoding_latency", stopWatch.stop());
            for (std::string host : connectedDevices)            
            {
                ClientSocket client_socket(host, DEVICE_PORT);
                std::string response;
                std::cout << "Sending Full Update to " << host << std::endl;

                sendFullUpdate(client_socket); 
            }
        }
        catch (SocketException &e)
        {
            std::cout << "Exception was caught while sending a full update:" << e.description() << std::endl;
            return false;
        }
        return true;
    }

};
#endif
