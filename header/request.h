//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HPP
#define HTTP_SERVER3_REQUEST_HPP

#include <string>
#include <vector>
#include "../header/header.h"

namespace http {
namespace server3 {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  std::string ipAddress;
  int http_version_major;
  int http_version_minor;
  bool readHeader = true;
  size_t contentLength = 0;
  size_t readAmount = 0;
  std::string jsonRequest;
  std::vector<header> headers;

  std::string username;
  std::string password;
  std::string requestid;
  std::string reqparam1;
  std::string reqparam2;
  std::string reqparam3;
  std::string reqparam4;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REQUEST_HPP
