#include "service_server.h"
#include "globals.h"

#include <inttypes.h>

#include <mdz_net_sockets/streams_bridge.h>
#include <mdz_net_sockets/acceptor_multithreaded.h>
#include <mdz_net_sockets/streams_bridge.h>
#include <thread>

#include "uplink_server.h"

using namespace Mantids::Application;
using namespace Mantids::Network::Sockets;
using namespace Mantids::Network::Sockets::NetStreams;
using namespace std;

// TODO: limpiador del pool...
Service_Server::Service_Server()
{
}

bool Service_Server::serviceStart()
{
    Acceptors::MultiThreaded * vClientAcceptor = new Acceptors::MultiThreaded();

    // Clean TCP Server (no chains)
    Socket_TCP * tcpServer = new Socket_TCP;
    if (!tcpServer->listenOn(port,listeningHost.c_str(),true))
    {
        LOG_APP->log0(__func__,Logs::LEVEL_ERR,"Failed to create published service TCP listener @%s:%" PRIu16,
                                   listeningHost.c_str(),port);
        delete vClientAcceptor;
        delete tcpServer;
        return false;
    }

    LOG_APP->log0(__func__,Logs::LEVEL_INFO,"Published TCP/IP Service '%s' listening at %s:%" PRIu16 "...", poolName.c_str(), listeningHost.c_str(),tcpServer->getPort());

    // STREAM MANAGER:
    vClientAcceptor->setAcceptorSocket(tcpServer);
    vClientAcceptor->setCallbackOnConnect(&pubClientHandler, this);
    vClientAcceptor->startThreaded();
    return true;
}

bool Service_Server::pubClientHandler(void *obj, Mantids::Network::Sockets::Socket_StreamBase *baseClientSocket, const char *remotePair, bool secure)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "S:pubClientHdlr");
#endif

    uint64_t cntId = Uplink_Server::serviceConnectionId++;

    Service_Server * servServer = (Service_Server *)obj;
    Socket_TCP * tcp = (Socket_TCP *)baseClientSocket;
    LOG_APP->log0(__func__,Logs::LEVEL_INFO,"[cntId=0x%08" PRIX64 "] Received TCP/IP connection from '%s' to published service %s:%" PRIu16,cntId, remotePair,servServer->getListeningHost().c_str(),servServer->getPort() );

    auto pooledUplinkConnection = Uplink_Server::getPublishedServiceConnectionPool(servServer->getPoolName())->getConnectionUplink();

    if (!pooledUplinkConnection.uplinkSocket)
    {
        LOG_APP->log0(__func__,Logs::LEVEL_WARN,"[cntId=0x%08" PRIX64 "] TCP/IP Connection from %s at published service %s:%" PRIu16 " failed: Empty Uplink Pool", cntId, remotePair,
                                   servServer->getListeningHost().c_str(),servServer->getPort() );
        return true;
    }

    // Activate and communicate:
    pooledUplinkConnection.uplinkSocket->writeU<uint8_t>(1);

    // Wait until the connection is established.
    auto ok = pooledUplinkConnection.uplinkSocket->readU<uint8_t>();

    if (ok == 1)
    {
        LOG_APP->log0(__func__,Logs::LEVEL_INFO,"[cntId=0x%08" PRIX64 "]->[upCntId=0x%08" PRIX64 "] Remotely Established Connection at uplink", cntId, pooledUplinkConnection.upCntId );
        Bridge bridge;
        bridge.setToCloseRemotePeer(false);
        bridge.setPeer(0,pooledUplinkConnection.uplinkSocket);
        bridge.setPeer(1,tcp);
        bridge.process();

        LOG_APP->log0(__func__,Logs::LEVEL_INFO,"[cntId=0x%08" PRIX64 "]->[upCntId=0x%08" PRIX64 "] Remotely Established Connection Finished", cntId, pooledUplinkConnection.upCntId  );
    }
    else
    {
        LOG_APP->log0(__func__,Logs::LEVEL_WARN,"[cntId=0x%08" PRIX64 "]->[upCntId=0x%08" PRIX64 "] Connection from %s at published service %s:%" PRIu16 " failed to be established at uplink ",cntId, pooledUplinkConnection.upCntId,
                                   remotePair,servServer->getListeningHost().c_str(),servServer->getPort()  );
    }

    delete pooledUplinkConnection.uplinkSocket;

    return true;
}

const std::string &Service_Server::getPoolName() const
{
    return poolName;
}

void Service_Server::setPoolName(const std::string &newPoolName)
{
    poolName = newPoolName;
}

const std::string &Service_Server::getListeningHost() const
{
    return listeningHost;
}

void Service_Server::setListeningHost(const std::string &newListeningHost)
{
    listeningHost = newListeningHost;
}

uint16_t Service_Server::getPort() const
{
    return port;
}

void Service_Server::setPort(uint16_t newPort)
{
    port = newPort;
}
