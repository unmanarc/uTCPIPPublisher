#include "uplink_client.h"
#include "globals.h"

#include <inttypes.h>

#include <mdz_net_sockets/socket_tcp.h>
#include <mdz_net_sockets/streams_bridge.h>
#include <mdz_net_sockets/streams_cryptochallenge.h>

#include <thread>

using namespace Mantids::Application;
using namespace Mantids::Network::Sockets;
using namespace std;

#ifdef ENCRYPTED
#include <mdz_net_chains/socket_chain.h>
#include <mdz_net_chains/socket_chain_aes.h>
using namespace Mantids::Network::Sockets;
#endif

std::atomic<uint64_t> Uplink_Client::uplinkId;

Uplink_Client::Uplink_Client()
{

}

void Uplink_Client::startUplinkClients()
{
    uplinkId = 0;
    for (uint16_t i=0; i<Globals::getLC_ServerPoolSize();i++)
    {
        thread(uplinkClientThread).detach();
    }
}

void Uplink_Client::uplinkClientThread()
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "U:clientThr");
#endif

    for (;;)
    {
        uint64_t curUpCntId = uplinkId++;
        Socket_TCP tcpClient;
        LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Connecting to uplink %s:%" PRIu16 " - '%s'",curUpCntId,
                                   Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort(),Globals::getLC_ClientServiceName().c_str()
                                   );

        if (tcpClient.connectTo( Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort() ))
        {

            LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Connected to uplink %s:%" PRIu16,curUpCntId,
                                       Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort());

            Socket_StreamBase * uplinkSock = &tcpClient;

#ifdef ENCRYPTED
            // Create the chain socket, delete the base socket on exit = false.
            Socket_Chain chainSocketAES(&tcpClient,false);
            ChainProtocols::Socket_Chain_AES aesChain;
            aesChain.setPhase1Key( Globals::getLC_ServerPSK().c_str() );
            if (!chainSocketAES.addToChain( &aesChain ))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_ERR, "[upCntId=0x%08" PRIX64 "] Invalid PSK",curUpCntId);
            }
            else
            {
                LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Pseudo-Encryption established.",curUpCntId);
                uplinkSock = &chainSocketAES;
#endif

                uplinkSock->writeStringEx<uint8_t>( Globals::getLC_ClientServiceName().c_str() );

                NetStreams::CryptoChallenge cstream( uplinkSock );

                if (cstream.mutualChallengeResponseSHA256Auth(Globals::getLC_ClientServiceAuthKey().c_str(),false) == std::make_pair(true,true))
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Authenticated with uplink %s:%" PRIu16,curUpCntId,
                                               Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort()  );

                    // Wait until we need to use this connection.
                    uint8_t ok = uplinkSock->readU<uint8_t>();
                    Socket_TCP tcpLocal;

                    switch(ok)
                    {
                    case 1:
                        LOG_APP->log0(__func__,Logs::LEVEL_DEBUG,"[upCntId=0x%08" PRIX64 "] The connection was inserted in the remote pool", curUpCntId);

                        // Wait until necesary...
                        ok = 7;
                        while (ok == 7)
                        {
                            ok = uplinkSock->readU<uint8_t>();
                            // Linked.
                            if (ok == 1)
                            {
                                // Connection is used now, create a new reverse connection to keep the pool.
                                thread(uplinkClientThread).detach();

                                LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] Connecting to TCP/IP service %s:%" PRIu16 " at the uplink server request", curUpCntId,
                                                           Globals::getLC_ClientLocalHostAddr().c_str(), Globals::getLC_ClientLocalPort());

                                if (tcpLocal.connectTo( Globals::getLC_ClientLocalHostAddr().c_str(), Globals::getLC_ClientLocalPort() ))
                                {
                                    LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] TCP/IP Connection established to %s:%" PRIu16 " at the uplink server request", curUpCntId,
                                                               Globals::getLC_ClientLocalHostAddr().c_str(), Globals::getLC_ClientLocalPort());

                                    // Notify that the connection was established.
                                    uplinkSock->writeU<uint8_t>(1);
                                    // Connected. Pipe two stream connections...
                                    NetStreams::Bridge bridge;
                                    bridge.setPeer(0,uplinkSock);
                                    bridge.setPeer(1,&tcpLocal);
                                    bridge.process();
                                    LOG_APP->log0(__func__,Logs::LEVEL_DEBUG, "[upCntId=0x%08" PRIX64 "] TCP/IP Connection to %s:%" PRIu16 " finished",curUpCntId,
                                                               Globals::getLC_ClientLocalHostAddr().c_str(), Globals::getLC_ClientLocalPort());
                                }
                                else
                                {
                                    LOG_APP->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] TCP/IP Connection to %s:%" PRIu16 " failed with: %s", curUpCntId,
                                                               Globals::getLC_ClientLocalHostAddr().c_str(), Globals::getLC_ClientLocalPort(), tcpLocal.getLastError().c_str());

                                    // Notify that the connection was not established.
                                    uplinkSock->writeU<uint8_t>(0);
                                }
                                // Connection finished, and a replacement exist, no need to iterate.
                                return;
                            }
                            else if (ok == 7)
                            {
                                uplinkSock->writeU<uint8_t>(7);
                            }
                            else
                            {
                                LOG_APP->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Connection to uplink %s:%" PRIu16 " has been terminated.",
                                                           curUpCntId, Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort());
                            }
                        }
                        break;
                    case 0:
                    default:
                        LOG_APP->log0(__func__,Logs::LEVEL_WARN,"[upCntId=0x%08" PRIX64 "] The uplink connection was not inserted in the remote pool, aborting...", curUpCntId);
                        exit(-1);
                        break;
                    }
                }
                else
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_ERR, "[upCntId=0x%08" PRIX64 "] Authentication failed at uplink %s:%" PRIu16 ".",curUpCntId,
                                               Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort());
                    exit(-2);
                }
#ifdef ENCRYPTED
            }
#endif
        }
        else
        {
            LOG_APP->log0(__func__,Logs::LEVEL_WARN, "[upCntId=0x%08" PRIX64 "] Error connecting to uplink %s:%" PRIu16 ": %s",curUpCntId, Globals::getLC_ServerHost().c_str(), Globals::getLC_ServerPort(), tcpClient.getLastError().c_str());
        }

        sleep(3);
    }
}
