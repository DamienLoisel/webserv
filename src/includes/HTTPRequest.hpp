/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 13:09:37 by dmathis           #+#    #+#             */
/*   Updated: 2025/02/19 02:20:37 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "webserv.hpp"

class HTTPRequest {
private:

    std::string method;
    std::string uri;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    void parseRequestLine(std::string& line);

    void parseHeaders(std::istringstream& iss);

    void parseBody(std::istringstream& iss);

public:

    HTTPRequest(const char* raw_request);

    std::string getMethod() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& key) const;
    std::string getBody() const;
};

#endif