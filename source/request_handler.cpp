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
    try {
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
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else if (req.requestid.compare("createcontest") == 0) {
                if (req.reqparam1.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                if (req.reqparam2.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                if (req.reqparam3.empty()) {
                    try {
                        results = srvDB->createContest(req.username, req.reqparam1, req.reqparam2, "0000-00-00 00:00:00");
                    }
                    catch (std::exception e) {
                        rep = reply::json_reply(reply::json_bad_parameter);
                        return;
                    }
                }
                else {
                    try {
                        results = srvDB->createContest(req.username, req.reqparam1, req.reqparam2, req.reqparam3);
                    }
                catch (std::exception e) {
                        rep = reply::json_reply(reply::json_bad_parameter);
                        return;
                    }
                }
            }
            else if (req.requestid.compare("updateimage") == 0) {
                if (req.reqparam2.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                if (req.reqparam4.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                try {
                    // username, contestid, base64image, slot (1st or 2nd), thumbnail
                    int contestid = stoi(req.reqparam1);
                    int slot = stoi(req.reqparam3);
                    rep = reply::json_reply((reply::status_type)srvDB->updateImage(req.username, contestid, req.reqparam2, slot, req.reqparam4));
                    return;
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else if (req.requestid.compare("addfriend") == 0) {
                if (req.reqparam1.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                try {
                    // username, friendname
                    results = srvDB->addFriend(req.username, req.reqparam1);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else if (req.requestid.compare("removefriend") == 0) {
                if (req.reqparam1.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                try {
                    // username, friendname
                    results = srvDB->removeFriend(req.username, req.reqparam1);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else if (req.requestid.compare("getmyfriends") == 0) {
                try {
                    results = srvDB->getMyFriends(req.username);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_db_bad_sql);
                    return;
                }
            }
            else if (req.requestid.compare("getfriendrequests") == 0) {
                try {
                    results = srvDB->getFriendRequests(req.username);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_db_bad_sql);
                    return;
                }
            }
            else if (req.requestid.compare("vote") == 0) {
                if (req.reqparam1.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                if (req.reqparam2.empty()) {
                    rep = reply::json_reply(reply::json_empty_parameter);
                    return;
                }
                try {
                    int contestid = stoi(req.reqparam1);
                    int slot = stoi(req.reqparam2);
                    results = srvDB->vote(req.username, contestid, slot);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else if (req.requestid.compare("getimage") == 0) {
				if (req.reqparam1.empty()) {
					rep = reply::json_reply(reply::json_empty_parameter);
                    return;
				}
				if (req.reqparam2.empty()) {
					rep = reply::json_reply(reply::json_empty_parameter);
                    return;
				}
				try {
                    int contestid = stoi(req.reqparam1);
                    int slot = stoi(req.reqparam2);
                    results = srvDB->publicGetImage(req.username, contestid, slot);
                }
                catch (std::exception e) {
                    rep = reply::json_reply(reply::json_bad_parameter);
                    return;
                }
            }
            else {
                rep = reply::json_reply(reply::json_bad_request);
                return;
            }
        }
        else {
            rep = reply::json_reply(reply::json_auth_fail);
            return;
        }

        rep.status = reply::ok;

        rep.content.append(results);
        rep.headers.resize(2);
        rep.headers[0].name = "Content-Length";
        rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
        rep.headers[1].name = "Content-Type";
        rep.headers[1].value = "application/json";
    } catch (std::exception e) {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }
}

} // namespace server3
} // namespace http
