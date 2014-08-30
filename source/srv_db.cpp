#include "../header/srv_db.h"

SRV_DB::SRV_DB(Config* newConfig, Logger* newLog) {
    runningConfig = newConfig;
    runningLog = newLog;

    driver = nullptr;
    connection = nullptr;
    connectionStatus = openConnection();
}

bool SRV_DB::openConnection() {
    try {
        driver = get_driver_instance();
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("Failed to connect to database driver. ErrorMsg: %s ", e.what());
        return false;
    }

    try {
        connection = driver->connect(runningConfig->getSqlServerAddress() + ":" + intToString(runningConfig->getSqlPort()),
                        runningConfig->getSqlUsername(), runningConfig->getSqlPassword());
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("Failed to connect to database. ErrorMsg: %s ", e.what());
        return false;
    }
    runningLog->sendMsg("Successfully connected to database at %s:%d", runningConfig->getSqlServerAddress().c_str(), runningConfig->getSqlPort());
    return true;
}

// This thing just keeps cropping up everywhere..
std::string SRV_DB::intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

bool SRV_DB::getConnectionStatus() {
    return connectionStatus;
}
