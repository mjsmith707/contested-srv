/* main located in here */
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include "../header/srv_db.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void srv_main();
void initializeLogging(Config* runningConfig, Logger* runningLog);

int main (int argc, char* argv[]) {
    srv_main();


    return 0;
}

void srv_main() {
    Config* runningConfig = new Config("srv.cfg");
    Logger* runningLog = new Logger(runningConfig);
    initializeLogging(runningConfig, runningLog);
    SRV_DB* srv_db = new SRV_DB(runningConfig, runningLog);

    /* presumably threading will be managed in here... */

    for(;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

void initializeLogging(Config* runningConfig, Logger* runningLog) {
    runningLog->sendMsg(" ");
    runningLog->sendMsg("Starting up...");
    runningLog->sendMsg("Configuration Dump:");
    runningLog->sendMsg("Debug: %d", runningConfig->getDebug());
    runningLog->sendMsg("Daemon: %d", runningConfig->getDaemon());
    runningLog->sendMsg("Config: %s", runningConfig->getConfigFile().c_str());
    runningLog->sendMsg("LogFile: %s", runningConfig->getLogFile().c_str());
    runningLog->sendMsg("ListenAddress: %s", runningConfig->getListenAddress().c_str());
    runningLog->sendMsg("Port: %d", runningConfig->getPort());
    runningLog->sendMsg("SQLAddress: %s", runningConfig->getSqlServerAddress().c_str());
    runningLog->sendMsg("SQLPort: %d", runningConfig->getSqlPort());
    runningLog->sendMsg("SQLUsername: %s", runningConfig->getSqlUsername().c_str());
    runningLog->sendMsg("SQLPassword: %s", runningConfig->getSqlPassword().c_str());
}
