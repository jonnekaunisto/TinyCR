#include "../include/TinyCRClient.h"
#include <thread>


TinyCRClient::TinyCRClient(int serverIP)
{
    this->serverIP = serverIP;
    this->daasClient = CRIoT_Data_VO<uint64_t, uint32_t>();

}