#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "HTTPRequest.hpp"
#include "CGIHandler.hpp"

// Constantes
#define MAX_BODY_SIZE 1048576  // 1MB
#define CGI_TIMEOUT 30         // 30 secondes

class HTTPResponse {
private:
    std::string response;

public:
    HTTPResponse(int status, std::string content_type, std::string body);
    std::string toString();
    
    static bool isCGI(const std::string& uri);
    static void executeCGI(const std::string& script_path, HTTPRequest& req, int client_fd);
    static void handle_request(HTTPRequest& req, int client_fd);
    static void sendErrorPage(int client_fd, int error_code, HTTPRequest& req);
};

#endif
