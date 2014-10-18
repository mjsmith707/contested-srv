// Handles all transactions to/from the mysql database

#ifndef SRV_DB_H_
#define SRV_DB_H_
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include <jsoncpp/json/json.h>
#include <vector>

/* MySQL Connector/C++ specific headers */
// You must link to mysqlcppcon.so (probably in /usr/lib/)
// This should be handled in the makefile/cbp project.
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/exception.h>
#include <cppconn/warning.h>

class SRV_DB {
    private:
    bool connectionStatus;
    Config* runningConfig;
    Logger* runningLog;
    sql::Driver *driver;
    sql::Connection *connection;

    static const int reservedWordsSize = 230;
    static const std::string reservedWords[];

    bool openConnection();
    std::string intToString(int val);

    public:
    SRV_DB(Config* newConfig, Logger* newLog);
    ~SRV_DB();

    bool getConnectionStatus();
    int createUser(std::string username, std::string password, std::string email_address);
    bool authenticateUser(std::string username, std::string password);
    int getUserID(std::string& username);
    std::string getUsername(int userid);
    bool deleteUser(std::string& username, std::string& password, std::string& email_address);
    std::string createContest(std::string username, std::string contest_name, std::string permissions, std::string endtime);
    std::string getUserContests(std::string username, unsigned int startPos, unsigned int endPos);
    int updateImage(std::string username, int contestID, std::string image, int imgslot);
    int insertImage(std::string& username, std::string& base64image);
    std::string getContest(int contestID);
    std::string getImage(int imageID);
    std::string getMyFriends(std::string username);
    std::string getFriendRequests(std::string username);
    std::string addFriend(std::string username, std::string friendname);
    std::string removeFriend(std::string username, std::string friendname);
    std::string vote(std::string username, int contestid, int imgslot);
    int getContestPermission(int contestid);
    bool checkString(std::string& input);
};

#endif
