// Provides timestamped logging to a file specified by the runningConfig
// as well as console output if Debug=true
// The only thing you want to use from here is sendMsg(printfFormatString, params...)

#ifndef LOGGER_H_
#define LOGGER_H_
#include "../header/constants.h"
#include "../header/config.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <ctime>

class Logger {
    private:
    Config* runningConfig;
    time_t unixTime;
    struct tm* localTime;

    void writeMsg(std::string message);
    void printMsg(std::string message);

    std::string doubleToString(double val);
    std::string intToString(int val);
    std::string uintToString(unsigned int val);
    std::string longToString(long val);

    public:
    Logger(Config* configuration);
    Config* getConfig();
    void setConfig(Config* configuration);
    void sendMsg(const char* strFormat, ...);
};

#endif
