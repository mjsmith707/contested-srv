// Holds all configuration data as read from a file specified in the ctor

#ifndef CONFIG_H_
#define CONFIG_H_
#include "../header/constants.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>

class Config {
    private:
    bool Debug;
    bool Daemon;
    unsigned int Port;
    unsigned int SQLPort;
    unsigned int httpThreads;
    std::string ListenAddress;
    std::string LogFile;
    std::string ConfigFile;
    std::string SQLServerAddress;
    std::string SQLUsername;
    std::string SQLPassword;

    void readConfigFile(std::string fileName);
    void readParameters(std::vector<std::string> parameters);
    std::string intToString(int val);

    public:
    Config(int& argc, char**& argv, std::string cfgFile);
    Config(std::string cfgFile);
    void reloadConfig();
    bool getDebug();
    bool getDaemon();
    unsigned int getPort();
    unsigned int getSqlPort();
    unsigned int getHttpThreads();
    std::string getListenAddress();
    std::string getLogFile();
    std::string getConfigFile();
    std::string getSqlServerAddress();
    std::string getSqlUsername();
    std::string getSqlPassword();
    void setDebug(bool val);
    void setDaemon(bool val);
    void setPort(unsigned int val);
    void setSqlPort(unsigned int val);
    void setHttpThreads(unsigned int val);
    void setListenAddress(std::string address);
    void setLogFile(std::string logFile);
    void setConfigFile(std::string cfgFile);
    void setSqlServerAddress(std::string address);
    void setSqlUsername(std::string username);
    void setSqlPassword(std::string password);
};

#endif
