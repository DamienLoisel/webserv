/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 15:40:57 by dmathis           #+#    #+#             */
/*   Updated: 2025/01/26 15:53:11 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPResponse.hpp"

HTTPResponse::HTTPResponse(int status, std::string content_type, std::string body) {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status << "\r\n";
    ss << "Content-Type: " << content_type << "\r\n";
    ss << "Content-Length: " << body.length() << "\r\n\r\n";
    ss << body;
    response = ss.str();
}

std::string HTTPResponse::toString() {
    return response;
}

void HTTPResponse::handle_request(HTTPRequest& req, int client_fd) {
   if (req.getMethod() == "GET") {
       std::string uri = "." + req.getURI();
       std::ifstream file(uri.c_str());
       
       if (!file.is_open()) {
           HTTPResponse resp(404, "text/plain", "File not found");
           send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
           return;
       }

       std::stringstream ss;
       char c;
       while (file.get(c)) {
           ss << c;
       }
       file.close();

       HTTPResponse resp(200, "text/html", ss.str());
       send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
   }
   else if (req.getMethod() == "POST") {
    std::string uri = "." + req.getURI();
    std::string body = req.getBody();
    
    std::cout << "Opening file: " << uri << std::endl;
    std::cout << "Body content: " << body << std::endl;
    
    std::ofstream file(uri.c_str());
    if (!file.is_open()) {
        std::cout << "Failed to open file" << std::endl;
        HTTPResponse resp(500, "text/plain", "Cannot create file");
        send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        return;
    }
    
    file << body;
    file.close();
    HTTPResponse resp(201, "text/plain", "File created");
    send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
}
   else if (req.getMethod() == "DELETE") {
       std::string uri = "." + req.getURI();
       if (remove(uri.c_str()) != 0) {
           HTTPResponse resp(404, "text/plain", "File not found");
           send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
           return;
       }
       
       HTTPResponse resp(200, "text/plain", "Deleted");
       send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
   }
   else {
       HTTPResponse resp(405, "text/plain", "Method not allowed");
       send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
   }
}