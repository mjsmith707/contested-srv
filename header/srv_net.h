#ifndef SRV_NET_H_
#define SRV_NET_H_
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"

class SRV_NET {
    private:
    bool connectionStatus;
    Config* runningConfig;
    Logger* runningLog;

    std::string intToString(int val);

    public:
    SRV_NET(Config* newConfig, Logger* newLog);
    ~SRV_NET();

};

#endif
