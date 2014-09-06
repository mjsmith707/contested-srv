/* main located in here */
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include "../header/srv_db.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
using namespace std;

void srv_main();
void initializeLogging(Config* runningConfig, Logger* runningLog);
void join_all(std::vector<std::thread>& v);
void do_join(std::thread& t);
std::string intToString(int val);
static void createContestWrapper(const void* dbPtr, std::string username, std::string description, std::string base64image);

int main (int argc, char* argv[]) {
    srv_main();

    return 0;
}

void srv_main() {
    Config* runningConfig = new Config("srv.cfg");
    Logger* runningLog = new Logger(runningConfig);
    initializeLogging(runningConfig, runningLog);
    SRV_DB* srv_db = new SRV_DB(runningConfig, runningLog);
    srv_db->createUser("testing3", "helloworld", "testing3@testing.org");
    srv_db->authenticateUser("testing3", "helloworld");
    //srv_db->deleteUser("Bogus", "Data", "bogus@data.com");
    //srv_db->createContest("testing3", "This is a test of the contest system", "base64imageislocatedhere");
    std::vector<std::string> results = srv_db->getContest(53);

    std::cout << "====RESULTS====" << std::endl;
    for (int i=0; i<results.size(); i++) {
        std::cout << results.at(i) << std::endl;
    }
    /* presumably threading will be managed in here... */

    //std::vector<std::thread> threadPool;
    //std::vector<SRV_DB*> dbPool;
    //for (int i=0; i<100; i++) {
    //    dbPool.push_back(new SRV_DB(runningConfig, runningLog));
    //}

    //for (int i=0; i<100; i++) {
    //    threadPool.push_back(std::thread (&createContestWrapper,dbPool.at(i), "testing3", "This is a test of the contest system #" + intToString(i), "base64imageislocatedhere"));
    //}

    //join_all(threadPool);

    for(;;) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

void join_all(std::vector<std::thread>& v)
{
    std::for_each(v.begin(),v.end(),do_join);
}

void do_join(std::thread& t)
{
    t.join();
}

std::string intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

static void createContestWrapper(const void* dbPtr, std::string username, std::string description, std::string base64image) {
    SRV_DB* srv_db = (SRV_DB*)dbPtr;
    srv_db->createContest(username, description, base64image);
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
