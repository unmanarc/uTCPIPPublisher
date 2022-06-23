#ifndef UPLINK_SERVER_H
#define UPLINK_SERVER_H

#include "service_server.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <map>
#include <mdz_net_sockets/socket_tcp.h>
//#include <mdz_net_chains/chainsockets.h>

struct sUplinkServiceConnection
{
    sUplinkServiceConnection(uint64_t uplinkId=0,Mantids::Network::Streams::StreamSocket * uplinkSocket = nullptr)
    {
        this->upCntId = uplinkId;
        this->uplinkSocket = uplinkSocket;
    }
    //Mantids::Network::Chains::ChainSockets * uplinkSocket;
    Mantids::Network::Streams::StreamSocket * uplinkSocket;
    uint64_t upCntId;
};

struct sServiceUplinkConnectionPool
{
    sUplinkServiceConnection getConnectionUplink()
    {
        sUplinkServiceConnection r;
        std::unique_lock<std::mutex> lk(mt);
        while (pool.empty())
        {
            // TODO: change ms...
            if (cond_notEmpty.wait_for(lk, std::chrono::milliseconds(10000)) == std::cv_status::timeout)
                return r;
        }
        r = pool.front();
        pool.pop();
        return r;
    }
    void addConnectionUplink(sUplinkServiceConnection connection)
    {
        std::unique_lock<std::mutex> lk(mt);
        pool.push(connection);
    }

    std::string serviceKey;
    std::queue<sUplinkServiceConnection> pool;
    std::mutex mt;
    std::condition_variable cond_notEmpty;
    Service_Server server;
};

class Uplink_Server
{
public:
    Uplink_Server();
    // Uplink:
    static void uplinkServerStart();
    static bool uplinkClientHdlr(void * obj, Mantids::Network::Streams::StreamSocket * baseClientSocket, const char * remotePair, bool secure);

    // Published services:
    static sServiceUplinkConnectionPool * getPublishedServiceConnectionPool(const std::string & poolName);
    static bool addPublishedServiceConnectionPool(const std::string &poolName, sServiceUplinkConnectionPool * pool);
    static void startPublishedServices();

    static std::atomic<uint64_t> serviceConnectionId;

private:
    static std::map<std::string,sServiceUplinkConnectionPool *> publishedServicesConnectionUplinkPool;
    static std::atomic<uint64_t> uplinkId;
};

#endif // UPLINK_SERVER_H
