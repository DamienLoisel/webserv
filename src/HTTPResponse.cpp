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

bool HTTPResponse::isCGI(const std::string& uri) {
    std::cout << "[DEBUG] Checking if URI is CGI: " << uri << std::endl;
    
    // Vérifie si l'URI commence par /cgi-bin/
    if (uri.substr(0, 9) != "/cgi-bin/") {
        std::cout << "[DEBUG] Not in /cgi-bin/ directory" << std::endl;
        return false;
    }
    
    // Vérifie si le fichier a l'extension .py
    bool is_py = uri.substr(uri.length() - 3) == ".py";
    std::cout << "[DEBUG] Is Python file? " << (is_py ? "yes" : "no") << std::endl;
    return is_py;
}

void HTTPResponse::executeCGI(const std::string& script_path, HTTPRequest& req, int client_fd) {
    std::cout << "[DEBUG] Executing CGI script: " << script_path << std::endl;
    std::cout << "[DEBUG] Method: " << req.getMethod() << std::endl;
    std::cout << "[DEBUG] URI: " << req.getURI() << std::endl;
    
    CGIHandler cgi(script_path);
    try {
        std::cout << "[DEBUG] Calling CGIHandler::executeCGI" << std::endl;
        std::string output = cgi.executeCGI(req.getMethod(), req.getURI(), req.getBody());
        std::cout << "[DEBUG] CGI Output: " << output.substr(0, 100) << "..." << std::endl;
        send(client_fd, output.c_str(), output.length(), 0);
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] CGI execution failed: " << e.what() << std::endl;
        HTTPResponse resp(500, "text/plain", "CGI execution failed: " + std::string(e.what()));
        send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
    }
}

void HTTPResponse::handle_request(HTTPRequest& req, int client_fd) {
   std::cout << "[DEBUG] Handling request for URI: " << req.getURI() << std::endl;
   std::cout << "[DEBUG] Method: " << req.getMethod() << std::endl;
   
   if (req.getMethod() == "GET" || req.getMethod() == "POST") {
       std::string uri = req.getURI();
       
       // Vérifie si c'est une requête CGI
       if (isCGI(uri)) {
           std::string script_path = "." + uri;
           // Vérifie si le script existe et est exécutable
           if (access(script_path.c_str(), X_OK) == 0) {
               executeCGI(script_path, req, client_fd);
               return;
           } else {
               HTTPResponse resp(403, "text/plain", "CGI script not executable");
               send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
               return;
           }
       }
       
       // Si ce n'est pas un CGI, traite comme un fichier normal
       std::string file_path = "." + uri;
       std::ifstream file(file_path.c_str());
       
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