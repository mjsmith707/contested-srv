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

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

boost::tribool request_parser::newParser(request& req, std::string& input, Config* runningConfig, Logger* runningLog) {
    try {
        if (req.readHeader) {
            // Some stupid stuff is happening in here
            // Splitting single string into vector here
            // Then rejoining it to a single string later for json eh...
            // Needs major cleanup
            std::vector<std::string> httpTokens;
            httpTokens.push_back(std::string("Content-Type: "));
            httpTokens.push_back(std::string("User-Agent: "));
            httpTokens.push_back(std::string("Host: "));
            httpTokens.push_back(std::string("Connection: "));
            httpTokens.push_back(std::string("Accept-Encoding: "));
            httpTokens.push_back(std::string("Content-Length: "));
            httpTokens.push_back(std::string("Accept: "));
            std::map<std::string, std::string> httpHeaders;
            std::vector<std::string> inputVctr;
            std::stringstream inputStream(input);
            std::string temp;
            while (std::getline(inputStream, temp)) {
                inputVctr.push_back(temp);
            }

            if (runningConfig->getDebug()) {
                runningLog->sendMsg("Parser Input:");
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
                    runningLog->sendMsg("postHeader parse failed.");
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
                        runningLog->sendMsg("contentLength parse failed.");
                    }
                    return false;
                }
                std::string contentLength = it->second;
                req.contentLength = stoi(contentLength);
            } catch (std::exception e) {
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("contentLength parse failed.");
                }
                return false;
            }

            std::string jsonRequest;
            for (unsigned int i=httpHeaders.size()+1; i<inputVctr.size(); i++) {
                std::string temp = inputVctr.at(i);
                req.readAmount += temp.length();
                req.jsonRequest += inputVctr.at(i);
                if (runningConfig->getDebug()) {
                    runningLog->sendMsg("%s", temp.c_str());
                }
            }
            req.readHeader = false;
            if (req.contentLength > req.readAmount) {
                return boost::indeterminate;
            }
        }
        else if (!req.readHeader) {

            std::string jsonRequest;
            req.readAmount += input.length();
            req.jsonRequest+= input;
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("%s", input.c_str());
                runningLog->sendMsg("Content-Length = %d", req.contentLength);
                runningLog->sendMsg("Read Amount = %d", req.readAmount);
            }
            if ((input.length() != 0) && (req.readAmount < req.contentLength)) {
                return boost::indeterminate;
            }
        }

        if (runningConfig->getDebug()) {
            runningLog->sendMsg("=End Parser Input=");
        }

        // Rebuild json object
        Json::Value root;
        Json::Reader jsonObject;

        bool jsonParsed = jsonObject.parse(req.jsonRequest, root);
        if (!jsonParsed) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("JSON Object parse failed.");
            }
            return false;
        }

        const Json::Value username = root["username"];
        const Json::Value password = root["password"];
        const Json::Value requestid = root["requestid"];
        const Json::Value reqparam1 = root["reqparam1"];
        const Json::Value reqparam2 = root["reqparam2"];
        const Json::Value reqparam3 = root["reqparam3"];
        if (username.empty() || password.empty() || requestid.empty()) {
            if (runningConfig->getDebug()) {
                runningLog->sendMsg("JSON Object parse failed.");
            }
            return false;
        }

        req.username = username.asString();
        req.password = password.asString();
        req.requestid = requestid.asString();
        req.reqparam1 = reqparam1.asString();
        req.reqparam2 = reqparam2.asString();
        req.reqparam3 = reqparam3.asString();

        if (runningConfig->getDebug()) {
            runningLog->sendMsg("JSON Parse: username=%s, password=%s, requestid=%s, param1=%s, param2=%s, param3=%s", req.username.c_str(), req.password.c_str(), req.requestid.c_str(), req.reqparam1.c_str(), req.reqparam2.c_str(), req.reqparam3.c_str());
        }

        return true;
    } catch (std::exception e) {
        return false;
    }
}

boost::tribool request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return boost::indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.method.push_back(input);
      return boost::indeterminate;
    }
  case uri_start:
    if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = uri;
      req.uri.push_back(input);
      return boost::indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      req.uri.push_back(input);
      return boost::indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return boost::indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return boost::indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return boost::indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return boost::indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return boost::indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case expecting_newline_3:
    return (input == '\n');
  default:
    return false;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (((c >= 0) && (c <= 31)) || (c == 127));
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server3
} // namespace http
