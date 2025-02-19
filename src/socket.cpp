/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 10:35:54 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/19 02:27:57 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "ConfigParser.hpp"
#include <poll.h>
#include <vector>
#include <fcntl.h>
#include <iostream>
#include <algorithm>

void handle_client(struct pollfd *fds, int i);
std::string findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations);
std::string join(const std::vector<std::string>& vec, const std::string& delimiter);

std::string findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations)
{
    for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        if (uri.find(it->first) == 0)
        {
            return it->first;
        }
    }
    return "";
}

std::string join(const std::vector<std::string>& vec, const std::string& delimiter)
{
    std::string result;
    for (size_t i = 0; i < vec.size(); i++)
    {
        result += vec[i];
        if (i < vec.size() - 1)
        {
            result += delimiter;
        }
    }
    return result;
}

bool socket(const ServerConfig& config)
{
    int socketfd;
    sockaddr_in sockaddr;
    std::vector<pollfd> fds;

    // Création socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1)
    {
        std::cerr << "Failed to create socket." << std::endl;
        return (false);
    }   

    fcntl(socketfd, F_SETFL, O_NONBLOCK);
    
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options." << std::endl;
        close(socketfd);
        return false;
    }   

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(config.listen_port);
    
    if (bind(socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cerr << "Failed to bind to port " << config.listen_port << "." << std::endl;
        return (false);
    }
    
    if (listen(socketfd, 10) < 0)
    {
        std::cerr << "Failed to listen on socket." << std::endl;
        return (false);
    }   

    std::cerr <<  "\033[1;32m" 
     << "\n""███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗     ██████╗ ███████╗ █████╗ ██████╗ ██╗   ██╗\n"
     << "██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗    ██╔══██╗██╔════╝██╔══██╗██╔══██╗╚██╗ ██╔╝\n"
     << "███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝    ██████╔╝█████╗  ███████║██║  ██║ ╚████╔╝ \n"
     << "╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗    ██╔══██╗██╔══╝  ██╔══██║██║  ██║  ╚██╔╝  \n"
     << "███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║    ██║  ██║███████╗██║  ██║██████╔╝   ██║   \n"
     << "╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝    ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝    ╚═╝   \n"
     << "\033[0m" "\n" << std::endl;
    
    pollfd server_fd = {socketfd, POLLIN, 0};
    fds.push_back(server_fd);   

    while (true)
    {
        if (poll(fds.data(), fds.size(), -1) < 0) 
        {
            std::cerr << "Poll failed." << std::endl;
            break;
        }   

        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].fd == socketfd && (fds[i].revents & POLLIN))
            {
                socklen_t addrlen = sizeof(sockaddr);
                int client_fd = accept(socketfd, (struct sockaddr*)&sockaddr, &addrlen);
                if (client_fd >= 0)
                {
                    fcntl(client_fd, F_SETFL, O_NONBLOCK);
                    pollfd client = {client_fd, POLLIN, 0};
                    fds.push_back(client);
                    std::cerr << "New client connected." << std::endl;
                }
            }
            else if (fds[i].revents & POLLIN)
            {
                handle_client(&fds[0], i);
            }
        }
    }
    for (size_t i = 0; i < fds.size(); i++) {
        close(fds[i].fd);
    }
    return true;
}

void handle_client(struct pollfd *fds, int i) {
    char buffer[1024];
    std::string request;
    ssize_t bytes_received;
    const size_t MAX_REQUEST_SIZE = 8192;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(fds[i].fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error setting socket timeout" << std::endl;
        close(fds[i].fd);
        fds[i].fd = -1;
        return;
    }

    std::cerr << "\n[DEBUG] Starting to read request..." << std::endl;

    while ((bytes_received = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        std::cerr << "[DEBUG] Received " << bytes_received << " bytes" << std::endl;
        
        if (request.length() + bytes_received > MAX_REQUEST_SIZE) {
            std::string error_msg = "HTTP/1.1 413 Request Entity Too Large\r\nContent-Length: 0\r\n\r\n";
            send(fds[i].fd, error_msg.c_str(), error_msg.length(), 0);
            close(fds[i].fd);
            fds[i].fd = -1;
            return;
        }

        buffer[bytes_received] = '\0';
        request += buffer;

        std::cerr << "[DEBUG] Current request:\n" << request << "\n[END REQUEST]" << std::endl;

        size_t header_end = request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::cerr << "[DEBUG] Found end of headers at position " << header_end << std::endl;
            
            size_t cl_pos = request.find("Content-Length: ");
            if (cl_pos != std::string::npos) {
                size_t cl_end = request.find("\r\n", cl_pos);
                if (cl_end != std::string::npos) {
                    size_t content_length = std::atoi(request.substr(cl_pos + 16, cl_end - (cl_pos + 16)).c_str());
                    std::cerr << "[DEBUG] Content-Length: " << content_length << std::endl;
                    
                    size_t current_body_length = request.length() - (header_end + 4);
                    std::cerr << "[DEBUG] Current body length: " << current_body_length << std::endl;
                    
                    if (current_body_length >= content_length) {
                        std::cerr << "[DEBUG] Got complete body" << std::endl;
                        break;
                    }
                }
            }
            else
            {
                std::cerr << "[DEBUG] No Content-Length found" << std::endl;
                break;
            }
        }
    }

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cerr << "Request timeout" << std::endl;
            std::string error_msg = "HTTP/1.1 408 Request Timeout\r\nContent-Length: 0\r\n\r\n";
            send(fds[i].fd, error_msg.c_str(), error_msg.length(), 0);
        }
        else
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        close(fds[i].fd);
        fds[i].fd = -1;
        return;
    }

    if (request.empty())
    {
        std::cerr << "Client disconnected" << std::endl;
        close(fds[i].fd);
        fds[i].fd = -1;
        return;
    }

    try {
        std::cerr << "[DEBUG] Creating HTTPRequest..." << std::endl;
        HTTPRequest req(request.c_str());
        std::cerr << "[DEBUG] Created HTTPRequest" << std::endl;
        
        std::cerr << "[DEBUG] Method: " << req.getMethod() << std::endl;
        std::cerr << "[DEBUG] URI: " << req.getURI() << std::endl;
        std::cerr << "[DEBUG] Body: " << req.getBody() << std::endl;
        
        HTTPResponse resp;
        resp.handle_request(req, fds[i].fd);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        std::string error_msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error\n";
        send(fds[i].fd, error_msg.c_str(), error_msg.length(), 0);
    }
}