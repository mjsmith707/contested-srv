/* main located in here */
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include "../header/srv_db.h"
#include "../header/server.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <pthread.h>
#include <signal.h>
using namespace std;

void srv_main();
void initializeLogging(Config* runningConfig, Logger* runningLog);
void join_all(std::vector<std::thread>& v);
void do_join(std::thread& t);
std::string intToString(int val);
static void createContestWrapper(const void* dbPtr, std::string username, std::string description, std::string base64image);

int main (int argc, char* argv[]) {
    std::cout << "cwd: " << argv[0] << std::endl;
    srv_main();

    return 0;
}

void srv_main() {
    Config* runningConfig = new Config("./bin/Debug/srv.cfg");
    Logger* runningLog = new Logger(runningConfig);
    initializeLogging(runningConfig, runningLog);
    SRV_DB* srv_db = new SRV_DB(runningConfig, runningLog);

    try {
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        // Run server in background thread.
        std::size_t num_threads = boost::lexical_cast<std::size_t>(runningConfig->getHttpThreads());
        http::server3::server http_main(runningConfig->getListenAddress(), intToString(runningConfig->getPort()), "./tmproot/", num_threads, runningConfig, runningLog);
        boost::thread http_main_thread(boost::bind(&http::server3::server::run, &http_main));

        // Restore previous signals.
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);

        // Wait for signal indicating time to shut down.
        sigset_t wait_mask;
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
        int sig = 0;
        sigwait(&wait_mask, &sig);

        // Stop the server.
        http_main.stop();
        http_main_thread.join();
    }
    catch (std::exception& e) {
        runningLog->sendMsg("%s", e.what());
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
