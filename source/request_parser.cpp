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
                runningLog->sendMsg("[" + req.ipAddress + "] Parser Input (First Read):");
                for (unsigned int i=0; i<inputVctr.size(); i++) {
                    runningLog->sendMsg(inputVctr[i]);
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
                    runningLog->sendMsg("[" + req.ipAddress + "] postHeader parse failed");
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
                        runningLog->sendMsg("[" + req.ipAddress + "] contentLength parse failed.");
                    }
                    return false;
                }
                std::string contentLength = it->second;
                req.contentLength = stoi(contentLength);
                if (req.contentLength < 0) {
                    if (runningConfig->getDebug()) {
                        runningLog->sendMsg("[" + req.ipAddress + "] contentLength parse failed.");
                    }
                    return false;
                }
            } catch (std::exception e) {
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("[" + req.ipAddress + "] contentLength parse failed.");
                }
                return false;
            }

            // Reading the rest of the http request
            for (unsigned int i=httpHeaders.size()+1; i<inputVctr.size(); i++) {
                std::string temp = inputVctr.at(i);
                req.readAmount += temp.length();
                req.jsonRequest += inputVctr.at(i);
                if (runningConfig->getDebug()) {
                    try {
						runningLog->sendMsg("[" + req.ipAddress + "] Content-Length = " + intToString(req.contentLength));
						runningLog->sendMsg("[" + req.ipAddress + "] Read Amount = " + intToString(req.readAmount));
                    }
                    catch (std::exception e){
						runningLog->sendMsg("[" + req.ipAddress + "] Caught failed int to string conversion for content-length/read ammount");
                    }
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
				try {
					runningLog->sendMsg("[" + req.ipAddress + "] Content-Length = " + intToString(req.contentLength));
					runningLog->sendMsg("[" + req.ipAddress + "] Read Amount = " + intToString(req.readAmount));
				}
				catch (std::exception e){
					runningLog->sendMsg("[" + req.ipAddress + "] Caught failed stoi conversion for content-length/read ammount");
				}
            }
            if ((input.length() != 0) && (req.readAmount < req.contentLength)) {
                return boost::indeterminate;
            }
        }

        if (runningConfig->getDebug()) {
            runningLog->sendMsg("[" + req.ipAddress + "] ==Truncated==");
            runningLog->sendMsg("[" + req.ipAddress + "] =End Parser Input=");
        }

        // Rebuild json object
        Json::Value root;
        Json::Reader jsonObject;

        bool jsonParsed = jsonObject.parse(req.jsonRequest, root);
        if (!jsonParsed) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("[" + req.ipAddress + "] JSON Object parse failed.");
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
                runningLog->sendMsg("[" + req.ipAddress + "] JSON Object parse failed.");
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
				runningLog->sendMsg("[" + req.ipAddress + "] JSON Parse: username ='" + req.username + "', password='" + req.password + "', requestid='" + req.requestid + "', param1='" + req.reqparam1 + "', param2='base64imagedata', param3='" + req.reqparam3 + "', param4='base64thumbdata'");
            }
            else {
				runningLog->sendMsg("[" + req.ipAddress + "] JSON Parse: username ='" + req.username + "', password='" + req.password + "', requestid='" + req.requestid + "', param1='" + req.reqparam1 + "', param2='" + req.reqparam2 + "', param3='" + req.reqparam3 + "', param4='" + req.reqparam4 + "'");
            }
        }

        return true;
    } catch (std::exception e) {
        return false;
    }
}

// This thing just keeps cropping up everywhere..
std::string request_parser::intToString(int val) {
    std::string str = "";
    char tmpstr[256];
    sprintf(tmpstr, "%d", val);
    str = tmpstr;
    return str;
}

} // namespace server3
} // namespace http
