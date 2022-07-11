#ifndef SERVICE_SERVER_H
#define SERVICE_SERVER_H

#include <mdz_net_sockets/socket_tcp.h>

class Service_Server
{
public:
    Service_Server();

    bool serviceStart(  );
    static bool pubClientHandler(void * obj, Mantids::Network::Sockets::Socket_StreamBase * baseClientSocket, const char * remotePair, bool secure);


    const std::string &getPoolName() const;
    void setPoolName(const std::string &newPoolName);

    const std::string &getListeningHost() const;
    void setListeningHost(const std::string &newListeningHost);

    uint16_t getPort() const;
    void setPort(uint16_t newPort);

private:

    std::string poolName;
    std::string listeningHost;
    uint16_t port;
};

#endif // SERVICE_SERVER_H
