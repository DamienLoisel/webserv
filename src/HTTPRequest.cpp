/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 15:41:07 by dmathis           #+#    #+#             */
/*   Updated: 2025/02/19 02:24:49 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
 
HTTPRequest::HTTPRequest(const char* raw_request) {
    std::string request(raw_request);
    
    size_t headers_end = request.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        headers_end = request.length();
    }
    
    std::string headers_part = request.substr(0, headers_end);
    std::istringstream headers_stream(headers_part);
    
    std::string request_line;
    std::getline(headers_stream, request_line);
    parseRequestLine(request_line);
    
    parseHeaders(headers_stream);
    
    if (headers_end < request.length()) {
        body = request.substr(headers_end + 4);
        std::cout << "[DEBUG] Read body: " << body << std::endl;
    }
    
    if (method != "GET" && method != "POST" && method != "DELETE") {
        version = "HTTP/1.1";
        throw std::runtime_error("501 Not Implemented");
    }
}

void HTTPRequest::parseRequestLine(std::string& line) {
    size_t first_space = line.find(' ');
    if (first_space == std::string::npos) {
        throw std::runtime_error("400 Bad Request");
    }
    
    method = line.substr(0, first_space);
    
    size_t second_space = line.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        throw std::runtime_error("400 Bad Request");
    }
    
    uri = line.substr(first_space + 1, second_space - first_space - 1);
    
    version = line.substr(second_space + 1);
}

void HTTPRequest::parseHeaders(std::istringstream& iss) {
    std::string line;
    while (std::getline(iss, line) && line != "\r")
    {
        size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            if (!value.empty() && value[value.length()-1] == '\r') {
                value = value.substr(0, value.length()-1);
            }
            headers[key] = value;
        }
    }
}

std::string HTTPRequest::getMethod() const {
     return method;
}

std::string HTTPRequest::getURI() const {
    return uri;
}

std::string HTTPRequest::getVersion() const {
    return version;
}

std::string HTTPRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    return it != headers.end() ? it->second : "";
}

std::string HTTPRequest::getBody() const {
    return body;
}