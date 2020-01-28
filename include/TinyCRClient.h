/**
 * Holds TinyCRServer Class
 * @author Xiaofeng Shi, Jonne Kaunisto 
 */
#include "../include/CRIoT.h"

#ifndef TinyCRClient_class
#define TinyCRClient_class

class TinyCRClient
{
public:
    TinyCRClient(int serverIP);

    /**
     * Start client, get request from server and listen to updates
     */
    int startClient();

    /**
     * Query a peer for certificate, returns bool
     */
    int queryCertificate(long long key);

private:
    int serverIP;
    CRIoT_Data_VO<uint64_t, uint32_t> dassClient;
};

#endif
