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

bool SRV_DB::createUser(std::string username, std::string password, std::string email_address) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("createUser() failed. Unable to connect to database.");
            return false;
        }
    }

    std::string query = "SELECT user_name FROM users WHERE user_name='" + username + "';";
    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    sql::ResultSet* results;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        results = sqlStatement->executeQuery(query);
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return false;
    }
    // Should never get more than one result... if we do were in trouble.
    sql::SQLString resultStr;
    while(results->next()) {
        resultStr = results->getString(1);
    }

    std::string resultUser = resultStr.asStdString();

    if (resultUser.compare(username) == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("createUser() failed. User '%s' already exists.", username.c_str());
        }
        return false;
    }
    else {
        std::string createStmnt = "INSERT INTO users(user_name, user_score, password, email_address) values('" + username + "','0','" + password + "','" + email_address + "');";
        try {
            sqlStatement = connection->createStatement();
            sqlStatement->execute("USE contested;");
            sqlStatement = connection->createStatement();
            sqlStatement->execute(createStmnt);
        }
        catch (sql::SQLException e) {
            if (runningConfig->getDebug()) {
            runningLog->sendMsg("createUser() failed. ", e.what());
            }
        }
        return false;
    }

}

bool SRV_DB::authenticateUser(std::string username, std::string password) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("authenticateUser() failed. Unable to connect to database.");
            return false;
        }
    }

    // Todo: sha512, scrubbing username input for bad stuff (SELECT FROM * DROP *) etc.
    std::string query = "SELECT password FROM users WHERE user_name='" + username + "';";
    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    sql::ResultSet* results;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        results = sqlStatement->executeQuery(query);
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return false;
    }
    // Should never get more than one result... if we do were in trouble.
    sql::SQLString resultStr;
    while(results->next()) {
        resultStr = results->getString(1);
    }

    std::string resultPw = resultStr.asStdString();
    runningLog->sendMsg("%s", resultPw.c_str());
    if (resultPw.compare(password) == 0) {
        runningLog->sendMsg("Authentication successful for %s.", username.c_str());
        return true;
    }
    else {
        runningLog->sendMsg("Authentication failed for %s.", username.c_str());
        return false;
    }
}
