/* main located in here */
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include "../header/srv_db.h"
#include "../header/server.h"
#include "../header/srv_randcontst.h"
#include "../header/srv_topcontst.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
using namespace std;

void srv_main();
void initializeLogging(Config* runningConfig, Logger* runningLog);
std::string intToString(int val);

int main (int argc, char* argv[]) {
    srv_main();

    return 0;
}

void srv_main() {
    Config* runningConfig = nullptr;
    Logger* runningLog = nullptr;
    SRV_DB* runningRandDB = nullptr;
    SRV_DB* runningTopDB = nullptr;
    SRV_RANDCONTST* runningRandContests = nullptr;
    SRV_TOPCONTST* runningTopContests = nullptr;
    try {
        runningConfig = new Config("srv.cfg");
        runningLog = new Logger(runningConfig);
        initializeLogging(runningConfig, runningLog);
        runningRandDB = new SRV_DB(runningConfig, runningLog);
        runningTopDB = new SRV_DB(runningConfig, runningLog);
        runningRandContests = new SRV_RANDCONTST(runningConfig, runningLog, runningRandDB);
        runningTopContests = new SRV_TOPCONTST(runningConfig, runningLog, runningTopDB);

        // Attempt to detach
        if (runningConfig->getDaemon()) {
            std::cout << "Attempting to daemonize process..." << std::endl;
            pid_t pid;
            pid_t sid;
            pid = fork();
            if (pid < 0) {
                throw new std::runtime_error("Failed to fork parent process.");
            }
            if (pid > 0) {
                exit(EXIT_SUCCESS);
            }
            umask(0);

            sid = setsid();
            if (sid < 0) {
                exit(EXIT_FAILURE);
            }

            if ((chdir("/")) < 0) {
                exit(EXIT_FAILURE);
            }

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
    }
    catch (std::exception e) {
        delete runningConfig;
        delete runningLog;
        cerr << "FATAL ERROR: Initialization failed. Dragons beyond this point.\n Error: " << e.what() << endl;
        return;
    }

    try {
        // Block all signals for background threads.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        // Start logging service
        boost::thread logging_thread(boost::bind(&Logger::run, runningLog));
        // Start Random Contests polling service
        boost::thread randcontests_thread(boost::bind(&SRV_RANDCONTST::run, runningRandContests));
        // Start Top Contests polling service
		boost::thread topcontests_thread(boost::bind(&SRV_TOPCONTST::run, runningTopContests));
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
        runningLog->sendMsg("Server shutting down...");
        http_main_thread.join();
        runningLog->sendMsg("Server threads stopped");
        runningRandContests->stop();
        runningLog->sendMsg("Random Contests thread shutting down...");
        randcontests_thread.interrupt();
        randcontests_thread.join();
        runningLog->sendMsg("Random Contests thread stopped");
        runningTopContests->stop();
        runningLog->sendMsg("Top Contests thread shutting down...");
        topcontests_thread.interrupt();
        topcontests_thread.join();
        runningLog->sendMsg("Top Contests thread stopped");
        runningLog->stop();
        runningLog->sendMsg("Logging thread shutting down...");
        logging_thread.join();
		delete runningRandContests;
		delete runningTopContests;
		delete runningRandDB;
		delete runningTopDB;
        delete runningConfig;
        delete runningLog;
    }
    catch (std::exception& e) {
		std::string err = e.what();
        runningLog->sendMsg("Caught Exception on shutdown!" + err);
    }
}

std::string intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

void initializeLogging(Config* runningConfig, Logger* runningLog) {
    runningLog->sendMsg(" ");
    runningLog->sendMsg("Starting up...");
    runningLog->sendMsg("Configuration Dump:");
    runningLog->sendMsg("Debug: " + intToString(runningConfig->getDebug()));
    runningLog->sendMsg("Daemon: " + intToString(runningConfig->getDaemon()));
    runningLog->sendMsg("Config: " + runningConfig->getConfigFile());
    runningLog->sendMsg("LogFile: " + runningConfig->getLogFile());
    runningLog->sendMsg("ListenAddress: " + runningConfig->getListenAddress());
    runningLog->sendMsg("Port: " + intToString(runningConfig->getPort()));
    runningLog->sendMsg("SQLAddress: " + runningConfig->getSqlServerAddress());
    runningLog->sendMsg("SQLPort: " + runningConfig->getSqlPort());
    runningLog->sendMsg("SQLUsername: " + runningConfig->getSqlUsername());
    runningLog->sendMsg("SQLPassword: " + runningConfig->getSqlPassword());
    runningLog->sendMsg(" ");
}
