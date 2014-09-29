#include "../header/srv_db.h"

const std::string SRV_DB::reservedWords[] = {"ACCESSIBLE", "ADD", "ALL", "ALTER", "ANALYZE", "AND", "AS", "ASC",
        "ASENSITIVE", "BEFORE", "BETWEEN", "BIGINT", "BINARY", "BLOB", "BOTH", "BY",
        "CALL", "CASCADE", "CASE", "CHANGE", "CHAR", "CHARACTER", "CHECK", "COLLATE",
        "COLUMN", "CONDITION", "CONSTRAINT", "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT_DATE",
        "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE", "DATABASES", "DAY_HOUR", "DAY_MICROSECOND",
        "DAY_MINUTE", "DAY_SECOND", "DEC", "DECIMAL", "DECLARE", "DEFAULT", "DELAYED", "DELETE",
        "DESC", "DESCRIBE", "DETERMINISTIC", "DISTINCT", "DISTINCTROW", "DIV", "DOUBLE", "DROP",
        "DUAL", "EACH", "ELSE", "ELSEIF", "ENCLOSED", "ESCAPED", "EXISTS", "EXIT",
        "EXPLAIN", "FALSE", "FETCH", "FLOAT", "FLOAT4", "FLOAT8", "FOR", "FORCE",
        "FOREIGN", "FROM", "FULLTEXT", "GRANT", "GENERAL", "GROUP", "HAVING", "HIGH_PRIORITY",
        "HOUR_MICROSECOND", "HOUR_MINUTE", "HOUR_SECOND", "IF", "IGNORE", "IGNORE_SERVER_IDS", "IN", "INDEX",
        "INFILE", "INNER", "INOUT", "INSENSITIVE", "INSERT", "INT", "INT1", "INT2",
        "INT3", "INT4", "INT8", "INTEGER", "INTERVAL", "INTO", "IS", "ITERATE",
        "JOIN", "KEY", "KEYS", "KILL", "LEADING", "LEAVE", "LEFT", "LIKE",
        "LIMIT", "LINEAR", "LINES", "LOAD", "LOCALTIME", "LOCALTIMESTAMP", "LOCK", "LONG",
        "LONGBLOB", "LONGTEXT", "LOOP", "LOW_PRIORITY", "MASTER_SSL_VERIFY_SERVER_CERT", "MASTER_HEARTBEAT_PERIOD", "MATCH", "MAXVALUE",
        "MEDIUMBLOB", "MEDIUMINT", "MEDIUMTEXT", "MIDDLEINT", "MINUTE_MICROSECOND", "MINUTE_SECOND", "MOD", "MODIFIES",
        "NATURAL", "NOT", "NO_WRITE_TO_BINLOG", "NULL", "NUMERIC", "ON", "OPTIMIZE", "OPTION",
        "OPTIONALLY", "OR", "ORDER", "OUT", "OUTER", "OUTFILE", "PRECISION", "PRIMARY",
        "PROCEDURE", "PURGE", "RANGE", "READ", "READS", "READ_WRITE", "REAL", "REFERENCES",
        "REGEXP", "RELEASE", "RENAME", "REPEAT", "REPLACE", "REQUIRE", "RESIGNAL", "RESTRICT",
        "RETURN", "REVOKE", "RIGHT", "RLIKE", "SCHEMA", "SCHEMAS", "SECOND_MICROSECOND", "SELECT",
        "SENSITIVE", "SEPARATOR", "SET", "SHOW", "SIGNAL", "SLOW", "SMALLINT", "SPATIAL",
        "SPECIFIC", "SQL", "SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "SQL_BIG_RESULT", "SQL_CALC_FOUND_ROWS", "SQL_SMALL_RESULT",
        "SSL", "STARTING", "STRAIGHT_JOIN", "TABLE", "TERMINATED", "THEN", "TINYBLOB", "TINYINT",
        "TINYTEXT", "TO", "TRAILING", "TRIGGER", "TRUE", "UNDO", "UNION", "UNIQUE",
        "UNLOCK", "UNSIGNED", "UPDATE", "USAGE", "USE", "USING", "UTC_DATE", "UTC_TIME",
        "UTC_TIMESTAMP", "VALUES", "VARBINARY", "VARCHAR", "VARCHARACTER", "VARYING", "WHEN", "WHERE",
        "WHILE", "WITH", "WRITE", "XOR", "YEAR_MONTH", "ZEROFILL"};

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

int SRV_DB::createUser(std::string username, std::string password, std::string email_address) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("createUser() failed. Unable to connect to database.");
            return 1002;
        }
    }

    if (!checkString(username) || !checkString(password) || !checkString(email_address)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s , %s", username.c_str(), password.c_str(), email_address.c_str());
        }
        return 1003;
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
        return 1003;
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
        return 1006;
    }
    else {
        std::string createStmnt = "INSERT INTO users(user_name, user_score, password, email_address) values('" + username + "','0','" + password + "','" + email_address + "');";
        try {
            sqlStatement = connection->createStatement();
            sqlStatement->execute("USE contested;");
            sqlStatement = connection->createStatement();
            sqlStatement->execute(createStmnt);
            return 1005;
        }
        catch (sql::SQLException e) {
            std::string error = e.what();
            if (error.find("for key 'email_address_UNIQUE'") != std::string::npos) {
                return 1008;
            }
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("createUser() failed. %s", e.what());
            }
            return 1003;
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

    if (!checkString(username) || !checkString(password)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s", username.c_str(), password.c_str());
        }
        return false;
    }

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

int SRV_DB::getUserID(std::string& username) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserID() failed. Unable to connect to database.");
            return 0;
        }
    }

    if (!checkString(username)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s", username.c_str());
        }
        return 0;
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

std::string SRV_DB::getUsername(int userid) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserID() failed. Unable to connect to database.");
            return "";
        }
    }

    if (userid == 0) {
        return "";
    }

    std::string query = "SELECT user_name FROM users WHERE user_id='" + intToString(userid) + "';";
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
    std::string result;
    while(results->next()) {
        result = results->getString(1);
    }

    return result;
}

// Todo: Delete all contests/images associated with user
bool SRV_DB::deleteUser(std::string& username, std::string& password, std::string& email_address) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("deleteUser() failed. Unable to connect to database.");
            return false;
        }
    }

    if (!checkString(username) || !checkString(password) || !checkString(email_address)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s , %s", username.c_str(), password.c_str(), email_address.c_str());
        }
        return false;
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

std::string SRV_DB::createContest(std::string username, std::string contest_name) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserID() failed. Unable to connect to database.");
            return "{\"RESULT\": \"1002\"}";
        }
    }

    if (!checkString(username) || !checkString(contest_name)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s", username.c_str(), contest_name.c_str());
        }
        return "{\"RESULT\": \"1013\"}";
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return "{\"RESULT\": \"1013\"}";
    }

    std::string query = "INSERT INTO contest(user1, name) values('" + intToString(userid) + "','" + contest_name + "');";
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
        sqlStatement = connection->createStatement();
        results = sqlStatement->executeQuery("SELECT LAST_INSERT_ID();");
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return "{\"RESULT\": \"1003\"}";
    }

    int resultInt = 0;
    while(results->next()) {
        resultInt = results->getInt(1);
    }
    std::string result = "{\"CONTESTKEY\": \"";
    result += intToString(resultInt);
    result += "\"}";
    return result;
}

std::string SRV_DB::getUserContests(std::string username, unsigned int startPos, unsigned int endPos) {
    if (endPos < startPos) {
        runningLog->sendMsg("getUserContests() failed. endPos is smaller than startPos.");
        return "{\"RESULT\": \"1013\"}";
    }

    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getUserContests() failed. Unable to connect to database.");
            return "{\"RESULT\": \"1002\"}";
        }
    }

    if (!checkString(username)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s", username.c_str());
        }
        return "{\"RESULT\": \"1013\"}";
    }

    int userid = getUserID(username);
    if (userid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("getUserID returned UID 0 (AKA does not exist) for username: %s", username.c_str());
        }
        return "{\"RESULT\": \"1013\"}";
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
        return "{\"RESULT\": \"1003\"}";
    }

    unsigned int i=0;
    unsigned int limit = endPos - startPos;
    Json::Value event;
    while(results->next() && (i<limit)) {
        event["contests"][i]["contestID"] = results->getString(1).asStdString();
        event["contests"][i]["userOneID"] = results->getString(2).asStdString();
        event["contests"][i]["userTwoID"] = results->getString(3).asStdString();
        event["contests"][i]["contestName"] = results->getString(4).asStdString();
        event["contests"][i]["image1"] = getImage(results->getInt(7));
        event["contests"][i]["image2"] = getImage(results->getInt(8));
        event["contests"][i]["userOne"] = getUsername(results->getInt(2));
        event["contests"][i]["userTwo"] = getUsername(results->getInt(3));
        i++;
    }
    return event.toStyledString();
}

// Internally used to store image in the images table. Only called by updateImage. Should be private.
int SRV_DB::insertImage(std::string& username, std::string& base64image) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("insertImage() failed. Unable to connect to database.");
            return 0;
        }
    }

    if (!checkString(username) || !checkString(base64image)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s", username.c_str(), base64image.c_str());
        }
        return 0;
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

int SRV_DB::updateImage(std::string username, int contestID, std::string image, int imgslot) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("insertImage() failed. Unable to connect to database.");
            return 1002;
        }
    }

    if (!checkString(username) || !checkString(image)) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("Caught Special Character: %s , %s", username.c_str(), image.c_str());
        }
        return 1013;
    }

    int imageid = insertImage(username, image);
    if (imageid == 0) {
        if (runningConfig->getDebug()) {
            runningLog->sendMsg("updateImage() failed. Couldn't insert image.");
        }
        return 999;
    }
    std::string query;
    if (imgslot == 1) {
        query = "UPDATE contest SET image1='" + intToString(imageid) + "', user1='" + intToString(getUserID(username)) +"' WHERE contest_id='" + intToString(contestID)+ "';";
    }
    else if (imgslot == 2) {
        query = "UPDATE contest SET image2='" + intToString(imageid) + "', user2='" + intToString(getUserID(username)) + "' WHERE contest_id='" + intToString(contestID)+ "';";
    }
    else {
        return 1013;
    }

    if (runningConfig->getDebug()) {
        runningLog->sendMsg("SQL: %s", query.c_str());
    }

    sql::Statement* sqlStatement;
    try {
        sqlStatement = connection->createStatement();
        sqlStatement->execute("USE contested;");
        sqlStatement = connection->createStatement();
        sqlStatement->executeUpdate(query);
    }
    catch (sql::SQLException e) {
        runningLog->sendMsg("SQL: %s", e.what());
        return 1012;
    }

    return 1011;
}

std::string SRV_DB::getContest(int contestID) {
    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getContest() failed. Unable to connect to database.");
            return "{\"RESULT\": \"1002\"}";
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
        return "{\"RESULT\": \"1003\"}";
    }

    Json::Value event;
    while(results->next()) {
        event["contest"]["contestID"] = results->getString(1).asStdString();
        event["contest"]["userOneID"] = results->getInt(2);
        event["contest"]["userTwoID"] = results->getInt(3);
        event["contest"]["contestName"] = results->getString(4).asStdString();
        event["contest"]["image1"] = getImage(results->getInt(7));
        event["contest"]["image2"] = getImage(results->getInt(8));
        event["contest"]["userOne"] = getUsername(results->getInt(2));
        event["contest"]["userTwo"] = getUsername(results->getInt(3));
        event["contest"]["userOneScore"] = results->getString(5).asStdString();
        event["contest"]["userTwoScore"] = results->getString(6).asStdString();
    }

    return event.toStyledString();
}

std::string SRV_DB::getImage(int imageID) {
    std::string resultString = "NULL";

    if (!connectionStatus) {
        if(!openConnection()) {
            runningLog->sendMsg("getImage() failed. Unable to connect to database.");
            return resultString;
        }
    }

    if (imageID == 0) {
        return resultString;
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

// Enforce a minimal set of characters. 0-9, a-z, A-Z,
// and a few special chars.
// Find mysql reserved words and other bad stuff
bool SRV_DB::checkString(std::string& input) {
    for (unsigned int i=0; i<input.size(); i++) {
        char temp = input.at(i);
        switch (temp) {
            case ' ':
                continue;
            case '.':
                continue;
            case '@':
                continue;
            case '/':
                continue;
            case '+':
                continue;
            default:
                break;
        }
        if ((temp < 48) && (temp != 32)) {
            return false;
        }
        else if ((temp > 57) && (temp < 65)) {
            return false;
        }
        else if ((temp > 91) && (temp < 97)) {
            return false;
        }
        else if ((temp > 123)) {
            return false;
        }
    }

    for (int i=0; i<reservedWordsSize; i++) {
        if (input.find(reservedWords[i]) != std::string::npos) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("Caught SQL Reserved Word: %s", input.c_str());
            }
            return false;
        }
    }
    return true;
}
