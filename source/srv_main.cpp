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
    try {
        runningConfig = new Config("srv.cfg");
        runningLog = new Logger(runningConfig);
        initializeLogging(runningConfig, runningLog);

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
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        // Start logging service
        boost::thread logging_thread(boost::bind(&Logger::run, runningLog));
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
        runningLog->stop();
        logging_thread.join();
        delete runningConfig;
        delete runningLog;
    }
    catch (std::exception& e) {
        runningLog->sendMsg("%s", e.what());
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
    runningLog->sendMsg("Debug: %d", runningConfig->getDebug());
    runningLog->sendMsg("Daemon: %d", runningConfig->getDaemon());
    runningLog->sendMsg("Config: %s", runningConfig->getConfigFile().c_str());
    runningLog->sendMsg("LogFile: %s", runningConfig->getLogFile().c_str());
    runningLog->sendMsg("ListenAddress: %s", runningConfig->getListenAddress().c_str());
    runningLog->sendMsg("Port: %d", runningConfig->getPort());
    runningLog->sendMsg("SQLAddress: %s", runningConfig->getSqlServerAddress().c_str());
    runningLog->sendMsg("SQLPort: %s", runningConfig->getSqlPort().c_str());
    runningLog->sendMsg("SQLUsername: %s", runningConfig->getSqlUsername().c_str());
    runningLog->sendMsg("SQLPassword: %s", runningConfig->getSqlPassword().c_str());
    runningLog->sendMsg(" ");
}
