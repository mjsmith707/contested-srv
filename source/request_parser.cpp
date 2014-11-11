//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../header/request_parser.h"
#include "../header/request.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <jsoncpp/json/json.h>

namespace http {
namespace server3 {

request_parser::request_parser() {

}

/// HTTP Header Token Initializer
const std::vector<std::string> request_parser::httpTokens({"Content-Type: ", "User-Agent: ",
"Host: ", "Connection: ", "Accept-Encoding: ", "Content-Length: ", "Accept-Language: ", "Accept: ",
"Pragma: ", "Cache-Control: "});

boost::tribool request_parser::parser(request& req, std::string& input, Config* runningConfig, Logger* runningLog) {
    try {
        if (req.readHeader) {
            // Header parsing in here.
            // Limited to up to 12 http header fields.
            std::map<std::string, std::string> httpHeaders;
            std::vector<std::string> inputVctr;

            std::stringstream inputStream(input);
            std::string temp;
            while (std::getline(inputStream, temp)) {
                inputVctr.push_back(temp);
            }

            if (runningConfig->getDebug()) {
                runningLog->sendMsg("[%s] Parser Input (First Read):", req.ipAddress.c_str());
                for (unsigned int i=0; i<inputVctr.size(); i++) {
                    runningLog->sendMsg("%s", inputVctr[i].c_str());
                }
            }

            size_t postHeader = inputVctr[0].find("POST / HTTP/1.1", 0);
            if ((postHeader != std::string::npos) && (postHeader == 0)) {
                req.method = "POST";
                req.uri = "/";
                req.http_version_major = 1;
                req.http_version_minor = 1;
            }
            else {
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("[%s] postHeader parse failed.", req.ipAddress.c_str());
                }
                return false;
            }

            for (unsigned int i=1; i<inputVctr.size() && i<12; i++) {
                for (unsigned int j=0; j<httpTokens.size(); j++) {
                    size_t delimPos = inputVctr.at(i).find(httpTokens.at(j));
                    if (delimPos != std::string::npos) {
                        size_t splitPos = inputVctr.at(i).find(": ");
                        if (splitPos == std::string::npos) {
                            continue;
                        }
                        std::string tempstr = inputVctr.at(i).substr(splitPos+2,inputVctr.at(i).size()-2);
                        httpHeaders.insert(std::pair<std::string, std::string>(httpTokens.at(j), tempstr));
                        continue;
                    }
                }
            }

            try {
                std::map<std::string,std::string>::iterator it;
                it = httpHeaders.find("Content-Length: ");
                if (it == httpHeaders.end()) {
                    if (runningConfig->getDebug()) {
                        runningLog->sendMsg("[%s] contentLength parse failed.", req.ipAddress.c_str());
                    }
                    return false;
                }
                std::string contentLength = it->second;
                req.contentLength = stoi(contentLength);
                if (req.contentLength < 0) {
                    if (runningConfig->getDebug()) {
                        runningLog->sendMsg("[%s] contentLength parse failed.", req.ipAddress.c_str());
                    }
                    return false;
                }
            } catch (std::exception e) {
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("[%s] contentLength parse failed.", req.ipAddress.c_str());
                }
                return false;
            }

            // Reading the rest of the http request
            for (unsigned int i=httpHeaders.size()+1; i<inputVctr.size(); i++) {
                std::string temp = inputVctr.at(i);
                req.readAmount += temp.length();
                req.jsonRequest += inputVctr.at(i);
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("[%s] Content-Length = %d", req.ipAddress.c_str(), req.contentLength);
                    runningLog->sendMsg("[%s] Read Amount = %d", req.ipAddress.c_str(), req.readAmount);
                }
            }
            req.readHeader = false;
            if (req.contentLength > req.readAmount) {
                return boost::indeterminate;
            }
        }
        else if (!req.readHeader) {
            // Subsequent calls for more reads go here
            req.readAmount += input.length();
            req.jsonRequest+= input;
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("[%s] Content-Length = %d", req.ipAddress.c_str(), req.contentLength);
                runningLog->sendMsg("[%s] Read Amount = %d", req.ipAddress.c_str(), req.readAmount);
            }
            if ((input.length() != 0) && (req.readAmount < req.contentLength)) {
                return boost::indeterminate;
            }
        }

        if (runningConfig->getDebug()) {
            runningLog->sendMsg("[%s] ==Truncated==", req.ipAddress.c_str());
            runningLog->sendMsg("[%s] =End Parser Input=", req.ipAddress.c_str());
        }

        // Rebuild json object
        Json::Value root;
        Json::Reader jsonObject;

        bool jsonParsed = jsonObject.parse(req.jsonRequest, root);
        if (!jsonParsed) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("[%s] JSON Object parse failed.", req.ipAddress.c_str());
            }
            return false;
        }

        const Json::Value username = root["username"];
        const Json::Value password = root["password"];
        const Json::Value requestid = root["requestid"];
        const Json::Value reqparam1 = root["reqparam1"];
        const Json::Value reqparam2 = root["reqparam2"];
        const Json::Value reqparam3 = root["reqparam3"];
        const Json::Value reqparam4 = root["reqparam4"];
        if (username.empty() || password.empty() || requestid.empty()) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("[%s] JSON Object parse failed.", req.ipAddress.c_str());
            }
            return false;
        }

        req.username = username.asString();
        req.password = password.asString();
        req.requestid = requestid.asString();
        req.reqparam1 = reqparam1.asString();
        req.reqparam2 = reqparam2.asString();
        req.reqparam3 = reqparam3.asString();
        req.reqparam4 = reqparam4.asString();

        if (runningConfig->getDebug()) {
            if (req.requestid.compare("updateimage") == 0) {
                runningLog->sendMsg("[%s] JSON Parse: username='%s', password='%s', requestid='%s', param1='%s', param2='base64imagedata', param3='%s', param4='%s'",
                    req.ipAddress.c_str(), req.username.c_str(), req.password.c_str(), req.requestid.c_str(),
                    req.reqparam1.c_str(), req.reqparam3.c_str(), req.reqparam4.c_str());
            }
            else {
                runningLog->sendMsg("[%s] JSON Parse: username='%s', password='%s', requestid='%s', param1='%s', param2='%s', param3='%s', param4='%s'",
                    req.ipAddress.c_str(), req.username.c_str(), req.password.c_str(), req.requestid.c_str(),
                    req.reqparam1.c_str(), req.reqparam2.c_str(), req.reqparam3.c_str(), req.reqparam4.c_str());
            }
        }

        return true;
    } catch (std::exception e) {
        return false;
    }
}

} // namespace server3
} // namespace http
