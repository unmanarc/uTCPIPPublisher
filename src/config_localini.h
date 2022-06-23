#ifndef CONFIG_LOCALINI_H
#define CONFIG_LOCALINI_H

#include <boost/property_tree/ini_parser.hpp>

#ifdef _WIN32
static std::string dirSlash =  "\\";
#else
static std::string dirSlash =  "/";
#endif

class Config_LocalIni
{
public:
    Config_LocalIni();

    static void setLocalInitConfig(const boost::property_tree::ptree &config);

    static bool getLC_LogsUsingSyslog()
    {
        return pLocalConfig.get<bool>("Logs.Syslog",true);
    }

    static bool getLC_LogsShowColors()
    {
         return pLocalConfig.get<bool>("Logs.ShowColors",true);
    }

    static bool getLC_LogsShowDate()
    {
        return pLocalConfig.get<bool>("Logs.ShowDate",true);
    }

    static bool getLC_LogsDebug()
    {
        return pLocalConfig.get<bool>("Logs.Debug",false);
    }

    static uint16_t getLC_ServerPort()
    {
        return pLocalConfig.get<uint16_t>("Server.Port",false);
    }

    static uint16_t getLC_ServerPoolSize()
    {
        return pLocalConfig.get<uint16_t>("Server.PoolSize",10);
    }

    static std::string getLC_ServerPSK()
    {
        return pLocalConfig.get<std::string>("Server.PSK","");
    }

    static std::string getLC_ServerHost()
    {
        return pLocalConfig.get<std::string>("Server.Host","");
    }

    static std::string getLC_ServerServicesFile()
    {
        return pLocalConfig.get<std::string>("Server.Services","");
    }

    static std::string getLC_ClientServiceName()
    {
        return pLocalConfig.get<std::string>("Client.ServiceName","");
    }

    static std::string getLC_ClientServiceAuthKey()
    {
        return pLocalConfig.get<std::string>("Client.ServiceAuthKey","");
    }
    static std::string getLC_ClientLocalHostAddr()
    {
        return pLocalConfig.get<std::string>("Client.LocalHostAddr","");
    }
    static uint16_t getLC_ClientLocalPort()
    {
        return pLocalConfig.get<uint16_t>("Client.LocalPort",80);
    }


private:
    static boost::property_tree::ptree pLocalConfig;

};

#endif // CONFIG_LOCALINI_H
