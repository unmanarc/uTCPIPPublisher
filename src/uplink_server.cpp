#include "uplink_server.h"
#include "globals.h"

#include <mdz_net_sockets/socket_tcp.h>
#include <mdz_net_sockets/streamsocketsbridge.h>
#include <mdz_net_sockets/socket_acceptor_multithreaded.h>
#include <mdz_net_sockets/cryptostream.h>

#include <inttypes.h>
#include <thread>

#ifdef ENCRYPTED
#include <mdz_net_chains/chainsockets.h>
#include <mdz_net_chains/socketchain_aes.h>
using namespace Mantids::Network::Chains;
#endif
using namespace Mantids::Application;
using namespace Mantids::Network::Sockets;
using namespace Mantids::Network::Streams;
using namespace std;

std::map<std::string,sServiceUplinkConnectionPool *> Uplink_Server::publishedServicesConnectionUplinkPool;
std::atomic<uint64_t> Uplink_Server::uplinkId;
std::atomic<uint64_t> Uplink_Server::serviceConnectionId;

Uplink_Server::Uplink_Server()
{

}

bool Uplink_Server::uplinkClientHdlr(void * obj, StreamSocket * baseClientSocket, const char * remotePair, bool secure)
{
    pthread_setname_np(pthread_self(), "U:clientThrSrv");
    uint64_t curUplinkId = uplinkId++;

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Connection uplink received from %s",curUplinkId, remotePair  );


    StreamSocket * uplinkSock = baseClientSocket;

#ifdef ENCRYPTED
    // Create the chain socket, delete the base socket on exit = true.
    ChainSockets * chainSockets = new ChainSockets(baseClientSocket,true);
    Protocols::SocketChain_AES * aesChain = new Protocols::SocketChain_AES;
    aesChain->setPhase1Key( Globals::getLC_ServerPSK().c_str() );

    if (!chainSockets->addToChain( aesChain, true ))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Connection uplink received from %s failed at PSK authentication.",curUplinkId, remotePair );
    }
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Pseudo-Encryption established.",curUplinkId);
        uplinkSock = chainSockets;
#endif
        std::string serviceName = uplinkSock->readStringEx<uint8_t>();
        std::string key;

        if (publishedServicesConnectionUplinkPool.find(serviceName) != publishedServicesConnectionUplinkPool.end())
            key = publishedServicesConnectionUplinkPool[serviceName]->serviceKey;

        Mantids::Network::Streams::CryptoStream cstream(uplinkSock );

        if (cstream.mutualChallengeResponseSHA256Auth(key,true) == std::make_pair(true,true))
        {
            if (publishedServicesConnectionUplinkPool.find(serviceName) == publishedServicesConnectionUplinkPool.end())
            {
                Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Connection uplink received from %s failed: Published service name not defined.", curUplinkId, remotePair);
                // Report not inserted and exit.
                uplinkSock->writeU<uint8_t>(0);
            }
            else
            {
                // Report inserted:
                uplinkSock->writeU<uint8_t>(1);
                publishedServicesConnectionUplinkPool[serviceName]->addConnectionUplink({curUplinkId,uplinkSock});
                Globals::getAppLog()->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Connection uplink received from %s pooled for published service '%s'", curUplinkId, remotePair,
                                           serviceName.c_str());


                // Not delete/close the connection after exit:
                return false;
            }
        }
        else
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Connection uplink received from %s failed at published service authentication.",curUplinkId, remotePair );
        }
#ifdef ENCRYPTED
    }
#endif

    return true;
}

bool Uplink_Server::addPublishedServiceConnectionPool(const std::string &poolName, sServiceUplinkConnectionPool *pool)
{
    if (publishedServicesConnectionUplinkPool.find(poolName) != publishedServicesConnectionUplinkPool.end())
        return false;
    publishedServicesConnectionUplinkPool[poolName] = pool;
    return true;
}

void Uplink_Server::poolMonitorGCThread()
{    
    pthread_setname_np(pthread_self(), "U:poolGCThread");

    for (;;)
    {

        auto start = chrono::high_resolution_clock::now();
        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = finish - start;

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_DEBUG, "Starting Connection Garbage Collector");
        for (const auto & pool : publishedServicesConnectionUplinkPool)
        {
            pool.second->pingerGC();
        }

        finish = chrono::high_resolution_clock::now();
        elapsed = finish - start;
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_DEBUG, "Connection Garbage Collector finished after %f", elapsed.count());
        sleep(Globals::getLC_ServerGCPeriod());
    }
}

void Uplink_Server::startPoolMonitorGC()
{
    thread(poolMonitorGCThread).detach();
}

sServiceUplinkConnectionPool * Uplink_Server::getPublishedServiceConnectionPool(const std::string &poolName)
{
    if (publishedServicesConnectionUplinkPool.find(poolName) == publishedServicesConnectionUplinkPool.end())
        return nullptr;
    return publishedServicesConnectionUplinkPool[poolName];
}

void Uplink_Server::uplinkServerStart()
{
    serviceConnectionId = 1;
    uplinkId = 1;

    startPoolMonitorGC();
    startPublishedServices();

    Acceptors::Socket_Acceptor_MultiThreaded * vClientAcceptor = new Acceptors::Socket_Acceptor_MultiThreaded();

    Socket_TCP * tcpServer = new Socket_TCP;

    if (!tcpServer->listenOn(Globals::getLC_ServerPort(),Globals::getLC_ServerHost().c_str(),true))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR,"Error creating TCP listener @%s:%" PRIu16,
                                   Globals::getLC_ServerHost().c_str(),Globals::getLC_ServerPort());
        delete vClientAcceptor;
        delete tcpServer;
        return;
    }

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN,"Main Listener running on TCP @%s:%" PRIu16 "...", Globals::getLC_ServerHost().c_str(),tcpServer->getPort());

    // STREAM MANAGER:
    vClientAcceptor->setAcceptorSocket(tcpServer);
    vClientAcceptor->setCallbackOnConnect(&uplinkClientHdlr, nullptr);
    vClientAcceptor->startThreaded();
}

void Uplink_Server::startPublishedServices()
{
    for (auto & i : publishedServicesConnectionUplinkPool)
    {
        i.second->server.serviceStart();
    }
}


