//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../header/request_handler.h"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "../header/mime_types.h"
#include "../header/reply.h"
#include "../header/request.h"
#include "../header/srv_db.h"

namespace http {
namespace server3 {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{

}

void request_handler::handle_request(const request& req, reply& rep, Config* runningConfig, Logger* runningLog, SRV_DB* srvDB)
{
    if (!srvDB->getConnectionStatus()) {
        rep = reply::stock_reply(reply::service_unavailable);
        return;
    }
    std::string results;
    // User account creation
    if (req.requestid.compare("createuser") == 0) {
        if (req.reqparam1.empty()) {
                rep = reply::json_reply(reply::json_empty_parameter);
                return;
        }
        int dbResult = srvDB->createUser(req.username, req.password, req.reqparam1);
        rep = reply::json_reply((reply::status_type)dbResult);
    }
    else if (req.requestid.compare("authenticate") == 0) {
        if (srvDB->authenticateUser(req.username, req.password)) {
            rep = reply::json_reply(reply::json_auth_success);
        }
        else {
            rep = reply::json_reply(reply::json_auth_fail);
        }
    }
    // Authenticated commands
    else if (srvDB->authenticateUser(req.username, req.password)) {
        if (req.requestid.compare("getmycontests") == 0) {
            try {
                results = srvDB->getUserContests(req.username, 0, 10000);
            }
            catch (std::exception e) {
                rep = reply::json_reply(reply::json_db_bad_sql);
                return;
            }
        }
        else if (req.requestid.compare("getcontest") == 0) {
            if (req.reqparam1.empty()) {
                rep = reply::json_reply(reply::json_empty_parameter);
                return;
            }
            try {
                int contestid = stoi(req.reqparam1);
                results = srvDB->getContest(contestid);
            }
            catch (std::exception e) {
                rep = reply::json_reply(reply::json_db_bad_sql);
                return;
            }
        }
        else if (req.requestid.compare("createcontest") == 0) {
            if (req.reqparam1.empty()) {
                rep = reply::json_reply(reply::json_empty_parameter);
                return;
            }
            try {
                results = srvDB->createContest(req.username, req.reqparam1);
                runningLog->sendMsg("createContest results = %s", results.c_str());
            }
            catch (std::exception e) {
                rep = reply::json_reply(reply::json_db_bad_sql);
                return;
            }
        }
        else if (req.requestid.compare("updateimage") == 0) {
            if (req.reqparam1.empty()) {
                rep = reply::json_reply(reply::json_empty_parameter);
                return;
            }
            try {
                // username, contestid, base64image, slot (1st or 2nd)
                int contestid = stoi(req.reqparam1);
                int slot = stoi(req.reqparam3);
                results = srvDB->updateImage(req.username, contestid, req.reqparam2, slot);
            }
            catch (std::exception e) {
                rep = reply::json_reply(reply::json_db_bad_sql);
                return;
            }
        }
        else {
            rep = reply::json_reply(reply::json_bad_request);
            return;
        }
    }
    else {
        rep = reply::stock_reply(reply::unauthorized);
        return;
    }

  /*if (results.find("error") != std::string::npos) {
        Haven't decided on just pushing out a json error
        or using a standard http error.
  }*/
  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;

  rep.content.append(results);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "application/json";
}

} // namespace server3
} // namespace http
