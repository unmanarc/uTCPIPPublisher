#ifndef UPLINK_SERVER_H
#define UPLINK_SERVER_H

#include "service_server.h"
#include "globals.h"

#include <inttypes.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <map>
#include <mdz_net_sockets/socket_tcp.h>

struct sUplinkServiceConnection
{
    sUplinkServiceConnection(uint64_t uplinkId=0,Mantids::Network::Sockets::Socket_StreamBase * uplinkSocket = nullptr)
    {
        this->upCntId = uplinkId;
        this->uplinkSocket = uplinkSocket;
    }

    bool ping()
    {
        if (!this->uplinkSocket)
            return false;

        if (!this->uplinkSocket->writeU<uint8_t>(7) || this->uplinkSocket->readU<uint8_t>()!=7)
        {
            LOG_APP->log0(__func__,Mantids::Application::Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Uplink connection is not responding to ping, aborting...");

            // Bad ping, close the socket and remove this object...
            delete this->uplinkSocket;
            this->uplinkSocket = nullptr;
            return false;
        }
        return true;
    }

    Mantids::Network::Sockets::Socket_StreamBase * uplinkSocket;
    uint64_t upCntId;
};

struct sServiceUplinkConnectionPool
{
    sUplinkServiceConnection getConnectionUplink()
    {
        sUplinkServiceConnection r;
        auto now = std::chrono::system_clock::now();
        while (1)
        {
            std::unique_lock<std::mutex> lk(mt);
            while (pool.empty())
            {
                if (cond_notEmpty.wait_until(lk, now + std::chrono::milliseconds( Globals::getLC_ServerNoConnectionInPoolTimeoutMS() )) == std::cv_status::timeout)
                    return r;
            }

            r = pool.front();
            pool.pop();

            // If the connection pings, return the connection, if not, lock again until the time for leave...
            if (r.ping())
                return r;
        }
    }

    void pingerGC()
    {
        std::unique_lock<std::mutex> lk(mt);
        std::queue<sUplinkServiceConnection> scrubbedPool;
        while (!pool.empty())
        {
            sUplinkServiceConnection r = pool.front();
            pool.pop();
            if (r.ping())
                scrubbedPool.push(r);
        }
        pool = scrubbedPool;
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
    static bool uplinkClientHdlr(void * obj, Mantids::Network::Sockets::Socket_StreamBase * baseClientSocket, const char * remotePair, bool secure);

    // Published services:
    static sServiceUplinkConnectionPool * getPublishedServiceConnectionPool(const std::string & poolName);
    static bool addPublishedServiceConnectionPool(const std::string &poolName, sServiceUplinkConnectionPool * pool);
    static std::atomic<uint64_t> serviceConnectionId;

    static void poolMonitorGCThread();

private:
    static void startPoolMonitorGC();
    static void startPublishedServices();

    static std::map<std::string,sServiceUplinkConnectionPool *> publishedServicesConnectionUplinkPool;
    static std::atomic<uint64_t> uplinkId;
};

#endif // UPLINK_SERVER_H

