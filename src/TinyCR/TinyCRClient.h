/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto 
 */

#ifndef TinyCRClient_class
#define TinyCRClient_class
#include "../platform/CRIoT.h"
#include <thread>
#include <mutex>

#define DEVICE_PORT 40000

#define SUMMARY_MAX_SIZE 1024

template<typename K, class V>
class TinyCRClient
{
public:
    /**
     * Constructor for TinyCRClient
     * @param serverIP The hostname of the server
     */
    TinyCRClient(std::string serverIP)
    {
        this->serverIP = serverIP;
    }

    /**
     * Start client, get request from server and listen to updates.
     */
    bool startClient()
    {
        requestInitialSummary();
        std::thread updatesThread (listenForUpdates, this);
        updatesThread.join();
    }
    /**
     * Query a certificate
     * @param key Key, which should be queried
     * @returns bool indicating if the key is revoked or unrevoked
     */
    bool queryCertificate(const K &key)
	{
        queryLock.lock();
        bool result = daasClient.query(key) == 1;
        queryLock.unlock();
		return result;
	}

private:
    CRIoT_Data_VO<K, V> daasClient;

    std::string serverIP;
    std::thread summaryUpdatesThread;
    std::mutex queryLock;


    /**
     * Requests initial summary from the server
     */
    void requestInitialSummary()
    {
        vector<uint8_t> msg;
        try
        {
            /*initialize connnection to server*/
            ClientSocket client_socket (serverIP, 30000);
            char* data = new char[MAXRECV + 1];
            int n_bytes = client_socket.recv(data);
            readFullSummary(client_socket);
        }
        catch ( SocketException& e )
        {
            std::cout << "Exception was caught while requesting initial summary:" << e.description() << std::endl;
        }

    }

    void readFullSummary(Socket socket)
    {
        vector<uint8_t> msg;
        while(true)
        {
            char* data = new char [MAXRECV + 1];
            int n_bytes = socket.recv(data);
            for(int i=0; i<n_bytes; i++)
            {
                uint8_t byte;
                memcpy(&byte, &data[i], 1);

                msg.push_back(byte);
            }
            if(data[n_bytes-1]=='h' && data[n_bytes-2]=='s' && data[n_bytes-3]=='i' && data[n_bytes-4]=='n' 
                && data[n_bytes-5]=='i' && data[n_bytes-6]=='f')
                break;
            delete[] data;
        }
        std::cout << "decoding" << std::endl;
        daasClient.decoding(msg);
        std::cout << "decoded" << std::endl;
        socket.send("FullDone");
        std::cout << "sent ack" << std::endl;
    }
    
    /**
     * Listens for delta updates and full updates from the server continuously
     */
    static void listenForUpdates(TinyCRClient *tinyCRClient)
    {
        std::cout << "Listening For Summary Updates at port: " << DEVICE_PORT << std::endl;
        try
        {
            // Create the socket
            ServerSocket server(DEVICE_PORT);

            while (true)
            {
                std::cout << "waiting for new connection" << std::endl;
                ServerSocket new_sock;
                server.accept(new_sock);
                tinyCRClient->queryLock.lock();

                char* data = new char[MAXRECV + 1];
                int n_bytes = new_sock.recv(data);

                /* Check if the server is sending a full update */
                if(data[0] == 'F'){
                    std::cout << "reading full" << std::endl;
                    tinyCRClient->readFullSummary(new_sock);
                    std::cout << "full read" << std::endl;
                }
                else
                {
                    bool done_receiving = false;
                    std::cout << "action: " << unsigned(static_cast<uint8_t>(data[0])) << std::endl;
                    std::cout << "size: " << n_bytes << std::endl;
                    char* msg = new char [MAXRECV + 1];
                    memcpy(&msg[0], data, n_bytes);
                    uint offset = n_bytes;
                    if(data[n_bytes-1]=='h' && data[n_bytes-2]=='s' && data[n_bytes-3]=='i' && data[n_bytes-4]=='n' 
                            && data[n_bytes-5]=='i' && data[n_bytes-6]=='f')
                    {
                        done_receiving = true;
                    }

                    delete[] data;

                    while(!done_receiving)
                    {
                        char* data = new char [MAXRECV + 1];
                        n_bytes = new_sock.recv(data);
                        memcpy(&msg[offset], data, n_bytes);
                        offset += n_bytes;
                        std::cout << n_bytes << std::endl;
                        if(data[n_bytes-1]=='h' && data[n_bytes-2]=='s' && data[n_bytes-3]=='i' && data[n_bytes-4]=='n' 
                            && data[n_bytes-5]=='i' && data[n_bytes-6]=='f')
                        {
                            done_receiving = true;
                        }
                            
                        delete[] data;
                    }
                    tinyCRClient->daasClient.decode_summary(msg);
                    new_sock << "DoneSummary";
                    std::cout << "sent ack" << std::endl;
                }

                tinyCRClient->queryLock.unlock();
            }
        }
        catch (SocketException &e)
        {
            std::cout << "Exception was caught while listening for updates:" << e.description() << std::endl;
            std::cout << "Exiting." << std::endl;
        }
    }
};
#endif
