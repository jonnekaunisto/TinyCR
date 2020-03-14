/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto 
 */

#ifndef TinyCRClient_class
#define TinyCRClient_class
#include "../include/CRIoT.h"
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
        std::thread summaryUpdatesThread (listenForSummaryUpdates, this);
        summaryUpdatesThread.join();
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
		return resutl;
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
            ClientSocket client_socket (serverIP, 30000 );
            
            try
            {
                /*recieve the CRC packets*/
                while(true)
                {
                    char* data = new char [MAXRECV + 1];
                    int n_bytes = client_socket.recv(data);
                    // for(int k=7; k>=0; k--)
                    // 	cout<<((data[0]>>k)&(uint8_t(1)))<<" ";
                    // cout<<endl;
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
                
            }
            catch ( SocketException& ) {}


        }
        catch ( SocketException& e )
        {
            std::cout << "Exception was caught:" << e.description() << "\n";
        }

        /*decoding*/
        this->daasClient.decoding(msg);
    }
    
    /**
     * Listens for delta updates from the server continuously
     */
    static void listenForSummaryUpdates(TinyCRClient *tinyCRClient)
    {
        std::cout << "Listening For Summary Updates at port: " << DEVICE_PORT << "\n";
        try
        {
            // Create the socket
            ServerSocket server(DEVICE_PORT);

            while (true)
            {
                ServerSocket new_sock;
                server.accept(new_sock);
                queryLock.lock();
                try
                {
                    char* data = new char[MAXRECV + 1];
                    int n_bytes = new_sock.recv(data);
                    std::cout << "received: " << n_bytes << "\n";
                    tinyCRClient->daasClient.decode_summary(data);
                    delete[] data;
                    new_sock << "Done";
                }
                catch (SocketException &)
                {
                }
                queryLock.unlock();
            }
        }
        catch (SocketException &e)
        {
            std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
        }
    }
};
#endif
