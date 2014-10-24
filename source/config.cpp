#include "../header/config.h"

Config::Config(int& argc, char**& argv, std::string cfgFile) {
    Debug = true;
    Daemon = false;
    Port = 1234;
    SQLPort = "3306";
    httpThreads = 4;
    ListenAddress = "0.0.0.0";
    LogFile = "contested-srv.log";
    ConfigFile = "contested-srv.conf";
    SQLServerAddress = "0.0.0.0";

    std::vector<std::string> parameters;
    for (int i=1; i<argc; i++) {
        std::string param = "";
        param += argv[i];
        parameters.push_back(param);
    }

    Config::readConfigFile(cfgFile);
    // readParams when finished..
}

Config::Config(std::string cfgFile) {
    Debug = true;
    Daemon = false;
    Port = 1234;
    SQLPort = "";
    ListenAddress = "0.0.0.0";
    LogFile = "./contested-srv.log";
    ConfigFile = cfgFile;
    SQLServerAddress = "0.0.0.0";

    Config::readConfigFile(cfgFile);
}

void Config::readConfigFile(std::string fileName) {
    int lineNum = 0;
    try {
        std::fstream cfgFile;
        cfgFile.open(fileName.c_str(), std::fstream::in);
        if (!cfgFile.is_open())
        {
            throw std::runtime_error(std::string("Failed to open configuration file: " + ConfigFile));
        }

        // Pretty slacker way to do this without resorting to a real parser.
        // Also getline cannot handle non-unix newlines properly on a nix platform...
        do {
            lineNum++;
            std::string param = "";
            std::getline(cfgFile,param);
            if (param.find("=") == std::string::npos) {
                if (param.length() == 0) {
                    continue;
                }
                throw std::runtime_error(std::string("Failed to parse config file at line " + intToString(lineNum)));
                return;
            }
            std::string token = param.substr(0,param.find("="));
            std::string val = param.substr(param.find("=")+1, param.length()-1);
            if (token.compare("Debug") == 0) {
                if (val.compare("true") == 0) {
                    Debug = true;
                }
                else if (val.compare("false") == 0) {
                    Debug = false;
                }
                else {
                    throw std::runtime_error(std::string("Failed to parse config file at line " + intToString(lineNum)));
                }
            }
            else if (token.compare("Daemon") == 0) {
                if (val.compare("true") == 0) {
                    Daemon = true;
                }
                else if (val.compare("false") == 0) {
                    Daemon = false;
                }
                else {
                    throw std::runtime_error(std::string("Failed to parse config file at line " + intToString(lineNum)));
                }
            }
            else if (token.compare("Port") == 0) {
                Port = std::stoi(val);
            }
            else if (token.compare("SQLPort") == 0) {
                SQLPort = val;
            }
            else if (token.compare("HTTPThreads") == 0) {
                httpThreads = std::stoi(val);
            }
            else if (token.compare("ListenAddress") == 0) {
                ListenAddress = val;
            }
            else if (token.compare("LogFile") == 0) {
                LogFile = val;
            }
            else if (token.compare("SQLServerAddress") == 0) {
                SQLServerAddress = val;
            }
            else if (token.compare("SQLUsername") == 0) {
                SQLUsername = val;
            }
            else if (token.compare("SQLPassword") == 0) {
                SQLPassword = val;
            }
            else {
                throw std::runtime_error(std::string("Failed to parse config file at line " + intToString(lineNum)));
            }
        } while (!cfgFile.eof());
        cfgFile.close();
    }
    catch (std::runtime_error e) {
            std::cout << e.what() << std::endl;
    }
}

// Parses command line parameters for saving into config.
// Put it on the todo list for uhhhh later.
void Config::readParameters(std::vector<std::string> parameters) {
    // This is really low priority
}

std::string Config::intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

void Config::reloadConfig() {
    readConfigFile(getConfigFile());
}

bool Config::getDebug() {
    return Debug;
}

bool Config::getDaemon() {
    return Daemon;
}

unsigned int Config::getPort() {
    return Port;
}

std::string Config::getSqlFullAddress() {
    if (this->getSqlPort().empty()) {
        return this->getSqlServerAddress();
    }
    return this->getSqlServerAddress() + ":" + this->getSqlPort();
}

std::string Config::getSqlPort() {
    return SQLPort;
}

unsigned int Config::getHttpThreads() {
    return httpThreads;
}

std::string Config::getListenAddress() {
    return ListenAddress;
}

std::string Config::getLogFile() {
    return LogFile;
}

std::string Config::getConfigFile() {
    return ConfigFile;
}

std::string Config::getSqlServerAddress() {
    return SQLServerAddress;
}

void Config::setDebug(bool val) {
    if (!Daemon) {
        Debug = val;
    }
}

void Config::setDaemon(bool val) {
    if (!Debug) {
        Daemon = val;
    }
}

void Config::setPort(unsigned int val) {
    Port = val;
}
void Config::setSqlPort(unsigned int val) {
    SQLPort = val;
}
void Config::setHttpThreads(unsigned int val) {
    httpThreads = val;
}

void Config::setListenAddress(std::string address) {
    ListenAddress = address;
}

void Config::setLogFile(std::string logFile) {
    LogFile = logFile;
}

void Config::setConfigFile(std::string cfgFile) {
    ConfigFile = cfgFile;
}

void Config::setSqlServerAddress(std::string address) {
    SQLServerAddress = address;
}

std::string Config::getSqlUsername() {
    return SQLUsername;
}

std::string Config::getSqlPassword() {
    return SQLPassword;
}

void Config::setSqlUsername(std::string username) {
    SQLUsername = username;
}

void Config::setSqlPassword(std::string password) {
    SQLPassword = password;
}
