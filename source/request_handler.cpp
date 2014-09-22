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

    if (srvDB->authenticateUser(req.username, req.password)) {
        if (req.requestid.compare("getmycontests") == 0) {
            results = srvDB->getUserContests(req.username, 0, 10000);
        }
        else if (req.requestid.compare("getcontest") == 0) {
            if (req.reqparam1.empty()) {
                rep = reply::stock_reply(reply::bad_request);
                return;
            }
            try {
                int contestid = stoi(req.reqparam1);
                results = srvDB->getContest(contestid);
            }
            catch (std::exception e) {
                rep = reply::stock_reply(reply::bad_request);
                return;
            }
        }
        else {
            rep = reply::stock_reply(reply::bad_request);
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
