#ifndef CLIENT_H
#define CLIENT_H


#include <atomic>
#include <cstdint>

class Uplink_Client
{
public:
    Uplink_Client();
    static void startUplinkClients();
    static void uplinkClientThread();

private:
    static std::atomic<uint64_t> uplinkId;

};

#endif // CLIENT_H
