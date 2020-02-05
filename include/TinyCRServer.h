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
    TinyCRServer(int port, vector<K> positive_keys, vector<V> negative_keys);
    /**
     * Starts tiny CR server. Send delta updates on changes, monitor port for new devices
     */
    bool startServer();
    /**
     * Starts socket server for taking in commands from 3rd party
     */
    bool startSocketAPIServer();

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

    void listenForNewDevices();
    void sendSummaryUpdates();
};
#endif
