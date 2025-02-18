/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 15:41:07 by dmathis           #+#    #+#             */
/*   Updated: 2025/01/26 15:41:10 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPRequest.hpp"
 
HTTPRequest::HTTPRequest(const char* raw_request) {
        std::string request(raw_request);
        std::istringstream iss(request);
        
        // Parse ligne par ligne
        std::string request_line;
        std::getline(iss, request_line);
        parseRequestLine(request_line);
        parseHeaders(iss);
        parseBody(iss);

        // Vérifie les méthodes supportées après avoir parsé toute la requête
        if (method != "GET" && method != "POST" && method != "DELETE") {
            version = "HTTP/1.1"; // Assure que la version est définie pour la réponse d'erreur
            throw std::runtime_error("501 Not Implemented");
        }
}

void HTTPRequest::parseRequestLine(std::string& line) {
        std::istringstream iss(line);
        iss >> method >> uri >> version;
    }

void HTTPRequest::parseHeaders(std::istringstream& iss) {
    std::string line;
    while (std::getline(iss, line) && line != "\r")
    {
        size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);     // "Host"
            std::string value = line.substr(pos + 2);  // "example.com"
            headers[key] = value;
        }
    }
}

void HTTPRequest::parseBody(std::istringstream& iss) {
    if (headers.count("Content-Length") > 0) {
        int length = atoi(headers["Content-Length"].c_str());
        std::vector<char> body_buffer(length);
        iss.read(&body_buffer[0], length);
        body = std::string(body_buffer.begin(), body_buffer.end());
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