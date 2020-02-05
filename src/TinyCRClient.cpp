#include "../include/TinyCRClient.h"
#include <thread>

TinyCRClient::TinyCRClient(int serverIP)
{
    this->serverIP = serverIP;
    CRIoT_Data_VO<uint64_t, uint32_t>daasClient();
}

bool TinyCRClient::startClient()
{
    requestInitialSummary();
}

void TinyCRClient::requestInitialSummary()
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
    daasClient.decoding(msg);
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

