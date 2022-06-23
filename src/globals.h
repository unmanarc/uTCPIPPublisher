#ifndef GLOBALS_H
#define GLOBALS_H

#include "config_localini.h"
#include <boost/property_tree/ini_parser.hpp>
#include <mdz_prg_logs/applog.h>

#include <mutex>
#include <list>

#define ENCRYPTED 1

class Globals : public Config_LocalIni
{
public:
    Globals();

    static Mantids::Application::Logs::AppLog *getAppLog();
    static void setAppLog(Mantids::Application::Logs::AppLog *value);


private:
    static Mantids::Application::Logs::AppLog * applog;
};


#endif // GLOBALS_H
