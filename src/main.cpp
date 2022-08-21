#include <mdz_prg_service/application.h>
#include <mdz_prg_logs/applog.h>
#include <mdz_mem_vars/a_string.h>

#include "uplink_client.h"
#include "uplink_server.h"
#include "config.h"
#include "globals.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <sys/stat.h>

using namespace boost;

using namespace Mantids;
using namespace Mantids::Application;

class uTCPIPPublisher : public Mantids::Application::Application
{
public:
    uTCPIPPublisher() {
    }

    void _shutdown()
    {
        LOG_APP->log0(__func__,Logs::LEVEL_INFO, "SIGTERM received.");
    }

    void _initvars(int argc, char *argv[], Arguments::GlobalArguments * globalArguments)
    {
        globalArguments->setInifiniteWaitAtEnd(true);

        globalArguments->setVersion( atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PROJECT_VER_PATCH), "a" );
        globalArguments->setLicense("LGPLv3");
        globalArguments->setAuthor("Aarón Mizrachi");
        globalArguments->setEmail("aaron@unmanarc.com");
        globalArguments->setDescription(PROJECT_DESCRIPTION);

        globalArguments->addCommandLineOption("Client Options", 'c', "client-config" ,   "Client Configuration File"  , "", Mantids::Memory::Abstract::Var::TYPE_STRING );
        globalArguments->addCommandLineOption("Server Options", 's', "server-dir" ,      "Configuration directory with server_config.ini inside"  , "/etc/uTCPIPPublisher", Mantids::Memory::Abstract::Var::TYPE_STRING );
    }

    void loadServiceFile(const std::string &filePath)
    {
        // Create a root ptree
        property_tree::ptree root;

        property_tree::read_json(filePath, root);

        for (const auto & i : root)
        {
            std::string serviceName = i.second.get<std::string>("serviceName");
            std::string  serviceAuthKey = i.second.get<std::string>("serviceAuthKey");
            std::string  listenAddr = i.second.get<std::string>("listenAddr");
            uint16_t listenPort = i.second.get<uint16_t>("listenPort");

            sServiceUplinkConnectionPool * connectionPool = new sServiceUplinkConnectionPool;
            connectionPool->server.setPoolName(serviceName);
            connectionPool->server.setListeningHost(listenAddr);
            connectionPool->server.setPort(listenPort);
            connectionPool->serviceKey = serviceAuthKey;

            Uplink_Server::addPublishedServiceConnectionPool(serviceName,connectionPool);
        }

    }

    bool _config(int argc, char *argv[], Arguments::GlobalArguments * globalArguments)
    {
        unsigned int logMode = Logs::MODE_STANDARD;

        Logs::AppLog initLog(Logs::MODE_STANDARD);
        initLog.setPrintEmptyFields(true);
        initLog.setUsingAttributeName(false);
        initLog.setUserAlignSize(1);

        std::string configDir = globalArguments->getCommandLineOptionValue("server-dir")->toString();
        std::string configFile = "server_config.ini";

        if ( !((Mantids::Memory::Abstract::STRING *)globalArguments->getCommandLineOptionValue("client-config"))->getValue().empty() )
        {
            initLog.log0(__func__,Logs::LEVEL_INFO, "Client Mode.");
            configFile = ((Mantids::Memory::Abstract::STRING *)globalArguments->getCommandLineOptionValue("client-config"))->getValue();
            clientMode=true;
        }
        else
        {
            initLog.log0(__func__,Logs::LEVEL_INFO, "Server Mode.");

            if (access(configDir.c_str(),R_OK))
            {
                initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing server configuration dir: %s", configDir.c_str());
                return false;
            }
            chdir(configDir.c_str());
            clientMode=false;
        }

        initLog.log0(__func__,Logs::LEVEL_INFO, "Loading configuration: %s", (configFile).c_str());

        boost::property_tree::ptree pRunningConfig;

        if (!access((configFile).c_str(),R_OK))
        {
            struct stat stats;
            // Check file properties...
            stat((configFile).c_str(), &stats);

            int smode = stats.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

            if ( smode != 0600 )
            {
                initLog.log0(__func__,Logs::LEVEL_WARN, "config file permissions are not 0600 and some secret values may be exposed, changing...");

                if (chmod((configFile).c_str(),0600))
                {
                    initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Configuration file permissions can't be changed to 0600");
                    return false;
                }
                initLog.log0(__func__,Logs::LEVEL_INFO, "config file permissions changed to 0600");
            }

            boost::property_tree::ini_parser::read_ini((configFile).c_str(),pRunningConfig);
        }
        else
        {
            initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration: %s", (configFile).c_str());
            return false;
        }

        Globals::setLocalInitConfig(pRunningConfig);

        if ( Globals::getLC_LogsUsingSyslog() ) logMode|=Logs::MODE_SYSLOG;

        Globals::setAppLog(new Logs::AppLog(logMode));
        LOG_APP->setPrintEmptyFields(true);
        LOG_APP->setUsingColors(Globals::getLC_LogsShowColors());
        LOG_APP->setUsingPrintDate(Globals::getLC_LogsShowDate());
        LOG_APP->setModuleAlignSize(26);
        LOG_APP->setUsingAttributeName(false);
        LOG_APP->setDebug(Globals::getLC_LogsDebug());

        LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Configuration file loaded.");

        if (!clientMode)
        {
            LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Loading published services definitions...");
            loadServiceFile(Globals::getLC_ServerServicesFile());
        }

        return true;
    }

    int _start(int argc, char *argv[], Arguments::GlobalArguments * globalArguments)
    {
        if (clientMode)
        {
            // CLIENT:
            Uplink_Client::startUplinkClients();
        }
        else
        {
            // SERVER:
            Uplink_Server::uplinkServerStart();
        }

        return 0;
    }

private:
    bool clientMode;
};

int main(int argc, char *argv[])
{
    uTCPIPPublisher * main = new uTCPIPPublisher;
    return StartApplication(argc,argv,main);
}
