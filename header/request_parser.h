//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_PARSER_HPP
#define HTTP_SERVER3_REQUEST_PARSER_HPP

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/assign.hpp>
#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include <iostream>

namespace http {
    namespace server3 {
        struct request;

        /// Parser for incoming requests.
        class request_parser {
            public:
            /// Construct ready to parse the request method.
            request_parser();

            /// Static collection of HTTP Header Tokens
            static const std::vector<std::string> httpTokens;

            /// Parse some data. The tribool return value is true when a complete request
            /// has been parsed, false if the data is invalid, indeterminate when more
            /// data is required. The InputIterator return value indicates how much of the
            /// input has been consumed.
            template <typename InputIterator>
            boost::tuple<boost::tribool, InputIterator> parse(request& req,
            InputIterator begin, InputIterator end, Config* runningConfig, Logger* runningLog) {
                std::string input;
                while (begin != end) {
                    input += *begin++;
                }

                boost::tribool result = parser(req, input, runningConfig, runningLog);
                if (result || !result) {
                    return boost::make_tuple(result, begin);
                }

                result = boost::indeterminate;
                return boost::make_tuple(result, begin);
            }

            private:
            /// Handles the next socket read of input.
            boost::tribool parser(request& req, std::string& input, Config* runningConfig, Logger* runningLog);

        }; // class request_parser


    } // namespace server3
} // namespace http



#endif // HTTP_SERVER3_REQUEST_PARSER_HPP
