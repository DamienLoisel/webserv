#include "webserv.hpp"
#include "ConfigParser.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

std::vector<pollfd>* g_fds = NULL;
volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cerr << "\nReceived signal " << signum << ". Cleaning up and exiting..." << std::endl;
        g_running = 0;
    }
}

void cleanup_resources() {
    if (g_fds) {
       
        for (std::vector<pollfd>::iterator it = g_fds->begin(); it != g_fds->end(); ++it) {
            if (it->fd >= 0 && it->fd != STDIN_FILENO) {
                close(it->fd);
            }
        }
        g_fds->clear();
        g_fds = NULL; 
    }
}

bool socket(const ServerConfig& config)
{
    int socketfd;
    struct sockaddr_in sockaddr;

    socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1)
    {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }   

    int flags = fcntl(socketfd, F_GETFL, 0);
    if (flags == -1 || fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode." << std::endl;
        close(socketfd);
        return false;
    }
    
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options." << std::endl;
        close(socketfd);
        return false;
    }   

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(config.listen_port);
    if (inet_pton(AF_INET, config.host.c_str(), &sockaddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << config.host << std::endl;
        close(socketfd);
        return false;
    }
    
    if (bind(socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cerr << "Failed to bind to " << config.host << ":" << config.listen_port << std::endl;
        close(socketfd);
        return false;
    }
    
    if (listen(socketfd, SOMAXCONN) < 0)
    {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(socketfd);
        return false;
    }   

    std::cerr << "\033[32mServer listening on " << config.host << ":" << config.listen_port << "\033[0m" << std::endl;
    
    pollfd server_fd = {socketfd, POLLIN, 0};
    if (g_fds) {
        g_fds->push_back(server_fd);
    }

    return true;
}

void handle_client(struct pollfd *fds, int i)
{
   
    for (size_t j = 0; j < g_fds->size(); ++j) {
        if (g_fds->at(j).fd == fds[i].fd && j < 2) { 
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(fds[i].fd, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_fd < 0) {
                std::cerr << "Failed to accept client connection" << std::endl;
                return;
            }

            int flags = fcntl(client_fd, F_GETFL, 0);
            if (flags >= 0) {
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                pollfd client = {client_fd, POLLIN | POLLOUT, 0};  
                g_fds->push_back(client);
                std::cerr << "New client connected on server " << j << std::endl;
            } else {
                close(client_fd);
            }
            return;
        }
    }

  
    if (fds[i].revents & POLLIN) {
        char buffer[4096] = {0}; 
        ssize_t bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            std::cerr << "Client disconnected" << std::endl;
            close(fds[i].fd);
            fds[i].fd = -1;
            return;
        }

        buffer[bytes_read] = '\0';
        
        try {
         
            std::string host;
            size_t host_end = std::string(buffer).find("\r\n");
            if (host_end != std::string::npos) {
                std::string first_line = std::string(buffer).substr(0, host_end);
                size_t host_start = first_line.find("Host: ");
                if (host_start != std::string::npos) {
                    host = first_line.substr(host_start + 6);
                    size_t colon_pos = host.find(':');
                    if (colon_pos != std::string::npos) {
                        host = host.substr(0, colon_pos);
                    }
                }
            }

            extern std::vector<ServerConfig> server_configs;
            const ServerConfig* config = NULL;
            for (size_t j = 0; j < server_configs.size(); ++j) {
                if (server_configs[j].host == host) {
                    config = &server_configs[j];
                    HTTPResponse::setConfig(config);
                    break;
                }
            }

        
            if (!config && !server_configs.empty()) {
                config = &server_configs[0];
                HTTPResponse::setConfig(config);
            }

            HTTPRequest req(buffer, config);
            std::cerr << "[DEBUG] Method: " << req.getMethod() << std::endl;
            std::cerr << "[DEBUG] URI: " << req.getURI() << std::endl;
            std::cerr << "[DEBUG] Body: " << req.getBody() << std::endl;

            HTTPResponse resp;
            resp.handle_request(req, fds[i].fd);
        } catch (const std::exception& e) {
            std::string error_msg = e.what();
            int status_code = 500; 

            if (error_msg.find("413") == 0) {
                status_code = 413;
            }

            extern std::vector<ServerConfig> server_configs;
            if (!server_configs.empty()) {
                HTTPResponse::setConfig(&server_configs[0]);
            }

            std::string error_content;
            std::stringstream error_path_ss;
            error_path_ss << "./error/" << status_code << ".html";
            std::string error_path = error_path_ss.str();
            std::ifstream error_file(error_path.c_str());
            
            if (error_file.is_open()) {
                std::stringstream buffer;
                buffer << error_file.rdbuf();
                error_content = buffer.str();
                error_file.close();
            } else {
                error_content = error_msg;
            }

            std::stringstream ss;
            ss << error_content.length();
            std::stringstream status_ss;
            status_ss << status_code;

            std::string status_text = (status_code == 413) ? "Payload Too Large" : "Internal Server Error";
            std::string response = "HTTP/1.1 " + status_ss.str() + " " + status_text + 
                                 "\r\nContent-Type: text/html\r\nContent-Length: " + 
                                 ss.str() + "\r\n\r\n" + error_content;

            send(fds[i].fd, response.c_str(), response.length(), 0);
        }
    }
}