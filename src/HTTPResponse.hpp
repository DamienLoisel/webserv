#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <algorithm>
#include "HTTPRequest.hpp"
#include "CGIHandler.hpp"
#include "ConfigTypes.hpp"

class HTTPResponse {
private:
    std::string response;
    static const ServerConfig* config;
    bool isMethodAllowed(const std::string& uri, const std::string& method);
    bool isCGI(const std::string& uri);
    void executeCGI(const std::string& script_path, HTTPRequest& req, int client_fd);
    void sendErrorPage(int client_fd, int status_code, HTTPRequest& req);
    void serveFile(const std::string& path, int client_fd);
    void generateDirectoryListing(const std::string& path, int client_fd);
    std::string findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations);

public:
    HTTPResponse() {}
    HTTPResponse(int status, std::string content_type, std::string body);
    std::string toString();
    void handle_request(HTTPRequest& req, int client_fd);
    static void setConfig(const ServerConfig* cfg) { config = cfg; }
};

#endif
