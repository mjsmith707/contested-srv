// Handles all transactions to/from the mysql database

#ifndef SRV_DB_H_
#define SRV_DB_H_
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"

/* MySQL Connector/C++ specific headers */
// You must link to mysqlcppcon.so (probably in /usr/lib/)
// This should be handled in the cbp project.
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

    bool openConnection();


    std::string intToString(int val);

    public:
    SRV_DB(Config* newConfig, Logger* newLog);
    bool getConnectionStatus();

};

#endif
