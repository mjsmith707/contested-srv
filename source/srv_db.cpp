#include "../header/srv_db.h"

SRV_DB::SRV_DB(Config* newConfig, Logger* newLog) {
    runningConfig = newConfig;
    runningLog = newLog;

    driver = nullptr;
    connection = nullptr;
    connectionStatus = openConnection();
}

SRV_DB::~SRV_DB() {
    // Nothing yet.
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
            return true;
        }
        catch (sql::SQLException e) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("createUser() failed. %s", e.what());
            }
            return false;
        }
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
    sql::SQLString resultPw;
    while(results->next()) {
        resultPw = results->getString(1);
    }

    if (resultPw.compare(password) == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Authentication successful for %s.", username.c_str());
        }
        return true;
    }
    else {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Authentication failed for %s.", username.c_str());
        }
        return false;
    }
}

int SRV_DB::getUserID(std::string username) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserID() failed. Unable to connect to database.");
            return 0;
        }
    }

    std::string query = "SELECT user_id FROM users WHERE user_name='" + username + "';";
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
        return 0;
    }

    // Should never get more than one result... if we do were in trouble.
    int resultInt = 0;
    while(results->next()) {
        resultInt = results->getInt(1);
    }

    return resultInt;
}

// Todo: Delete all contests/images associated with user
bool SRV_DB::deleteUser(std::string username, std::string password, std::string email_address) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("deleteUser() failed. Unable to connect to database.");
            return false;
        }
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return false;
    }

    std::string query = "DELETE FROM users WHERE user_id='" + intToString(userid) + "' AND user_name='" + username + "';";
    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        sqlStatement->execute(query);
        return true;
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return false;
    }
}

bool SRV_DB::createContest(std::string username, std::string contest_name, std::string image1) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserID() failed. Unable to connect to database.");
            return false;
        }
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return false;
    }

    int imageid = insertImage(username, image1);
    if (imageid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("createContest() failed. Couldn't insert image.");
        }
        return false;
    }

    std::string query = "INSERT INTO contest(user1, name, image1) values('" + intToString(userid) + "','" + contest_name + "','" + intToString(imageid) + "');";
    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        sqlStatement->execute(query);
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return false;
    }

    return true;
}

std::vector<std::string> SRV_DB::getUserContests(std::string username, unsigned int startPos, unsigned int endPos) {
    std::vector<std::string> resultVctr;

    if (endPos < startPos) {
        runningLog->sendMsg("getUserContests() failed. endPos is smaller than startPos.");
        return resultVctr;
    }

    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserContests() failed. Unable to connect to database.");
            return resultVctr;
        }
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return resultVctr;
    }

    std::string query = "SELECT * FROM contest WHERE user1='" + intToString(userid) + "' OR user2='" + intToString(userid) + "' LIMIT " + intToString(startPos) + "," + intToString(endPos) + ";";
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
        return resultVctr;
    }

    sql::SQLString resultStr;
    unsigned int i=0;
    unsigned int limit = endPos - startPos;
    while(results->next() && (i<limit)) {
        resultVctr.push_back(results->getString(1));
        resultVctr.push_back(results->getString(2));
        resultVctr.push_back(results->getString(3));
        resultVctr.push_back(results->getString(4));
        resultVctr.push_back(results->getString(5));
        resultVctr.push_back(results->getString(6));
        resultVctr.push_back(results->getString(7));
        resultVctr.push_back(results->getString(8));
        i++;
    }
    return resultVctr;
}

int SRV_DB::insertImage(std::string username, std::string base64image) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("insertImage() failed. Unable to connect to database.");
            return 0;
        }
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return 0;
    }

    std::string query = "INSERT INTO images(owner_id, image) values('" + intToString(userid) + "','" + base64image + "');";
    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    sql::ResultSet* results;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        sqlStatement->execute(query);
        results = sqlStatement->executeQuery("SELECT LAST_INSERT_ID();");
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return 0;
    }

    int resultInt = 0;
    while(results->next()) {
        resultInt = results->getInt(1);
    }

    return resultInt;
}

std::vector<std::string> SRV_DB::getContest(int contestID) {
    std::vector<std::string> resultVctr;

    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getContest() failed. Unable to connect to database.");
            return resultVctr;
        }
    }

    std::string query = "SELECT * FROM contest WHERE contest_id='" + intToString(contestID) + "';";
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
        return resultVctr;
    }

    sql::SQLString resultStr;
    while(results->next()) {
        resultVctr.push_back(results->getString(1));
        resultVctr.push_back(results->getString(2));
        resultVctr.push_back(results->getString(3));
        resultVctr.push_back(results->getString(4));
        resultVctr.push_back(results->getString(5));
        resultVctr.push_back(results->getString(6));
        resultVctr.push_back(getImage(results->getInt(7)));
        resultVctr.push_back(getImage(results->getInt(8)));
    }

    return resultVctr;
}

std::string SRV_DB::getImage(int imageID) {
    std::string resultString = "NULL";

    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getImage() failed. Unable to connect to database.");
            return resultString;
        }
    }

    std::string query = "SELECT image FROM images WHERE image_id='" + intToString(imageID) + "';";
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
        return resultString;
    }

    sql::SQLString resultStr;
    while(results->next()) {
        resultStr = results->getString(1);
    }

    return resultStr.asStdString();
}
