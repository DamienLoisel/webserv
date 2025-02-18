/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 13:09:37 by dmathis           #+#    #+#             */
/*   Updated: 2025/01/26 15:41:03 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "webserv.hpp"

class HTTPRequest {
private:
    // Variables membres stockant les composants de la requête
    std::string method;      // GET, POST, etc.
    std::string uri;         // /index.html
    std::string version;     // HTTP/1.1
    std::map<std::string, std::string> headers;  // Host: example.com
    std::string body;        // Données POST

    // Parse la première ligne : "GET /index.html HTTP/1.1"
    void parseRequestLine(std::string& line);

    // Parse les headers ligne par ligne
    void parseHeaders(std::istringstream& iss);

    void parseBody(std::istringstream& iss);

public:
    // Constructeur prenant la requête brute
    HTTPRequest(const char* raw_request);

    // Getters pour accéder aux données parsées
    std::string getMethod() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& key) const;
    std::string getBody() const;
};

#endif