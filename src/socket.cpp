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

// Définition des variables globales
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
        // Fermer tous les sockets sauf stdin
        for (std::vector<pollfd>::iterator it = g_fds->begin(); it != g_fds->end(); ++it) {
            if (it->fd >= 0 && it->fd != STDIN_FILENO) {
                close(it->fd);
            }
        }
        g_fds->clear();
        g_fds = NULL;  // On ne delete pas car c'est un vecteur statique
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
    // Si c'est un socket serveur, accepter la nouvelle connexion
    for (size_t j = 0; j < g_fds->size(); ++j) {
        if (g_fds->at(j).fd == fds[i].fd && j < 2) { // Les 2 premiers fds sont les serveurs
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(fds[i].fd, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_fd < 0) {
                std::cerr << "Failed to accept client connection: " << strerror(errno) << std::endl;
                return;
            }

            // Mettre le client en mode non-bloquant
            int flags = fcntl(client_fd, F_GETFL, 0);
            if (flags >= 0) {
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                pollfd client = {client_fd, POLLIN, 0};
                g_fds->push_back(client);
                std::cerr << "New client connected on server " << j << std::endl;
            } else {
                close(client_fd);
            }
            return;
        }
    }

    // Sinon, c'est un client existant, traiter sa requête
    char buffer[4096] = {0};  // Initialiser à zéro
    ssize_t bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cerr << "Client disconnected." << std::endl;
        } else {
            std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
        }
        close(fds[i].fd);
        fds[i].fd = -1;
        return;
    }

    buffer[bytes_read] = '\0';
    
    try {
        HTTPRequest req(buffer);
        std::cerr << "[DEBUG] Method: " << req.getMethod() << std::endl;
        std::cerr << "[DEBUG] URI: " << req.getURI() << std::endl;
        std::cerr << "[DEBUG] Body: " << req.getBody() << std::endl;

        // Trouver le bon serveur en fonction du Host header
        std::string host = req.getHeader("Host");
        if (!host.empty()) {
            size_t colon_pos = host.find(':');
            if (colon_pos != std::string::npos) {
                host = host.substr(0, colon_pos);
            }

            extern std::vector<ServerConfig> server_configs;  // Utiliser les configs globales
            for (size_t j = 0; j < server_configs.size(); ++j) {
                if (server_configs[j].host == host) {
                    HTTPResponse::setConfig(&server_configs[j]);
                    break;
                }
            }
        }

        HTTPResponse resp;
        resp.handle_request(req, fds[i].fd);
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        std::string error_msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error\n";
        send(fds[i].fd, error_msg.c_str(), error_msg.length(), 0);
    }
}