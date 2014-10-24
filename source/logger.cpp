#include "../header/logger.h"

Logger::Logger(Config* configuration) {
    runningConfig = configuration;
    running = false;
}

void Logger::run() {
    running = true;
    while (running) {
        try {
            std::lock_guard<std::mutex> lock(msgMute);
            while (!messageList.empty()) {
                if (runningConfig->getDaemon()) {
                    if (!writeMsg(messageList.front())) {
                        // If we've failed to write over 200 messages
                        // Flush the queue and hope next time goes better
                        if (messageList.size() > 200) {
                            while (!messageList.empty()) {
                                messageList.pop();
                            }
                        }
                        break;
                    }
                    messageList.pop();
                }
                else {
                    printMsg(messageList.front());
                    if (!writeMsg(messageList.front())) {
                        // If we've failed to write over 200 messages
                        // Flush the queue and hope next time goes better
                        if (messageList.size() > 200) {
                            while (!messageList.empty()) {
                                messageList.pop();
                            }
                        }
                        break;
                    }
                    messageList.pop();
                }
            }
        }
        catch (std::exception e) {
            if (!runningConfig->getDaemon()) {
                std::cerr << "Logging thread exited unexpectedly. Error: " << e.what() << std::endl;
            }
            running = false;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Flush remaining messages.
    try {
        std::lock_guard<std::mutex> lock(msgMute);
        while (!messageList.empty()) {
            printMsg(messageList.front());
            writeMsg(messageList.front());
            messageList.pop();
        }
    }
    catch (std::exception e) {
        if (!runningConfig->getDaemon()) {
            std::cerr << "Logging thread exited unexpectedly. Error: " << e.what() << std::endl;
        }
        running = false;
        return;
    }

    printMsg("Logging thread stopped");
    writeMsg("Logging thread stopped");
    running = false;
}

void Logger::stop() {
    running = false;
}

Config* Logger::getConfig() {
    return runningConfig;
}

void Logger::setConfig(Config* configuration) {
    runningConfig = configuration;
}

void Logger::sendMsg(const char* strFormat, ...) {
    std::lock_guard<std::mutex> lock(msgMute);
    va_list varArgs;
    va_start(varArgs, strFormat);
    try {
        std::string formatStr = strFormat;
        std::string message = "";

        for (unsigned int i=0; i<formatStr.length(); i++) {
            if (formatStr.at(i) == '%') {
                if (i+1 < formatStr.length()) {
                    if (formatStr[i+1] == 'f') {
                        double var = va_arg(varArgs, double);
                        message += doubleToString(var);
                        i++;
                    }
                    else if (formatStr[i+1] == 'd') {
                        int var = va_arg(varArgs, int);
                        message += intToString(var);
                        i++;
                    }
                    else if (formatStr[i+1] == 'u') {
                        unsigned int var = va_arg(varArgs, unsigned int);
                        message += uintToString(var);
                        i++;
                    }
                    else if (formatStr[i+1] == 'l') {
                        if (i+2 < formatStr.length()) {
                            if (formatStr[i+2] == 'd') {
                                long var = va_arg(varArgs, long);
                                message += longToString(var);
                                i=i+2;
                            }
                        }
                    }
                    else if (formatStr[i+1] == 'c') {
                        char var = va_arg(varArgs, int);
                        message += var;
                        i++;
                    }
                    else if (formatStr[i+1] == 's') {
                        char* var = va_arg(varArgs, char*);
                        for (int j=0; var[j] != '\0'; j++) {
                            message += var[j];
                        }
                        i++;
                    }
                    else {
                        message += formatStr.at(i);
                    }
                }
            }
            else {
                message += formatStr.at(i);
            }
        }
        va_end(varArgs);
        messageList.push(message);
        return;
    }
    catch (std::exception e) {
        va_end(varArgs);
        if (!runningConfig->getDaemon()) {
            if (runningConfig->getDebug()) {
                std::cerr << "DEBUG: Something bad just happened in sendMsg(). Error: " << e.what() << std::endl;
            }
        }
    }
}

std::string Logger::doubleToString(double val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%f", val);
    str = tmpstr;
    return str;
}

std::string Logger::intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

std::string Logger::uintToString(unsigned int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%u", val);
    str = tmpstr;
    return str;
}

std::string Logger::longToString(long val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%ld", val);
    str = tmpstr;
    return str;
}

bool Logger::writeMsg(std::string message) {
    std::string logPath = runningConfig->getLogFile();
    try {
        std::ofstream logFile;
        logFile.open(logPath.c_str(), std::fstream::app);
        if (!logFile.is_open()) {
            logFile.open(logPath.c_str(), std::fstream::out);
            if (!logFile.is_open()) {
                throw std::runtime_error("Failed to create log file.");
            }
        }
        time(&unixTime);
        localTime = localtime(&unixTime);
        std::string logtime = asctime(localTime);
        logtime.erase(logtime.length()-1, 1);
        logFile << "[" << logtime << "] " << message << std::endl;
        logFile.flush();
        logFile.close();
    }
    catch (std::runtime_error e) {
        if (!runningConfig->getDaemon()) {
            std::cerr << "ERROR: " << e.what() << std::cerr;
        }
        return false;
    }
    return true;
}

void Logger::printMsg(std::string message) {
    if (runningConfig->getDaemon()) {
        return;
    }

    if (runningConfig->getDebug()) {
        std::cout << "DEBUG: " << message << std::endl;
    }
}
