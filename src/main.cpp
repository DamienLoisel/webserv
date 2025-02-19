/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/25 17:18:12 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/19 22:38:51 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "ConfigParser.hpp"
#include "HTTPResponse.hpp"
#include <iostream>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <set>
#include <sstream>

static std::vector<pollfd> fds;  // Vecteur statique pour éviter les problèmes de delete
std::vector<ServerConfig> server_configs;  // Garder les configs en mémoire

// Vérifier les doublons d'IP et de port
bool check_duplicate_configs(const std::vector<ServerConfig>& configs) {
    std::set<std::string> ips;
    std::set<int> ports;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        // Vérifier les IPs en double
        if (ips.find(configs[i].host) != ips.end()) {
            std::cerr << "\033[31mError: Duplicate IP address found: " << configs[i].host << "\033[0m" << std::endl;
            std::cerr << "\033[31mEach server must have a unique IP address.\033[0m" << std::endl;
            return false;
        }
        ips.insert(configs[i].host);

        // Vérifier les ports en double
        if (ports.find(configs[i].listen_port) != ports.end()) {
            std::cerr << "\033[31mError: Duplicate port found: " << configs[i].listen_port << "\033[0m" << std::endl;
            std::cerr << "\033[31mEach server must have a unique port.\033[0m" << std::endl;
            return false;
        }
        ports.insert(configs[i].listen_port);
    }
    return true;
}

void display(const char *file)
{
    ConfigParser parser;
    if (!parser.parse(file))
        return;
    const std::vector<ServerConfig>& servers = parser.getServers();
    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::cout << "\nServer " << i + 1 << " Configuration Details:" << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << "Listen Port: " << servers[i].listen_port << std::endl;
        std::cout << "Host: " << servers[i].host << std::endl;
        std::cout << "Server Name: " << servers[i].server_name << std::endl;
        std::cout << "Error Page: " << servers[i].error_page << std::endl;
        std::cout << "Client Max Body Size: " << servers[i].client_max_body_size << " bytes" << std::endl;
        std::cout << "Root Directory: " << servers[i].root << std::endl;
        std::cout << "Default Index: " << servers[i].index << std::endl;

        std::cout << "\nLocation Configurations:" << std::endl;
        std::cout << "----------------------" << std::endl;
        for (std::map<std::string, LocationConfig>::const_iterator it = servers[i].locations.begin(); it != servers[i].locations.end(); ++it)
        {
            std::cout << "Location Path: " << it->first << std::endl;
            std::cout << "  Root: " << it->second.root << std::endl;
            std::cout << "  Autoindex: " << (it->second.autoindex ? "On" : "Off") << std::endl;
            std::cout << "  Allowed Methods:";
            for (std::vector<std::string>::const_iterator mit = it->second.allowed_methods.begin(); mit != it->second.allowed_methods.end(); ++mit)
                std::cout << " " << *mit;
            std::cout << " " << std::endl;
            std::cout << "  Index: " << it->second.index << std::endl;
            std::cout << "  Return Path: " << it->second.return_path << std::endl;
            std::cout << "  Alias: " << it->second.alias << std::endl;
            std::cout << "  CGI Path: " << it->second.cgi_path << std::endl;
            std::cout << "  CGI Extensions: ";
            for (std::vector<std::string>::const_iterator eit = it->second.cgi_ext.begin(); eit != it->second.cgi_ext.end(); ++eit)
                std::cout << *eit << " ";
            std::cout << std::endl << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    const char *config_file = (argc > 1) ? argv[1] : "conf/webserv.conf";

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        std::cerr << "Failed to set up signal handlers." << std::endl;
        return 1;
    }

    ConfigParser parser;
    if (!parser.parse(config_file)) {
        std::cerr << "Failed to parse configuration file." << std::endl;
        return 1;
    }

    display(config_file);

    server_configs = parser.getServers();
    if (server_configs.empty()) {
        std::cerr << "No server configurations found." << std::endl;
        return 1;
    }

    // Vérifier les doublons d'IP et de port
    if (!check_duplicate_configs(server_configs)) {
        std::cerr << "\033[31mError: Server configuration contains duplicate IP or port.\033[0m" << std::endl;
        return 1;
    }

    // Définir la configuration pour HTTPResponse
    HTTPResponse::setConfig(&server_configs[0]); // On utilise le premier serveur par défaut

    g_fds = &fds;  // Utiliser le vecteur statique

    for (size_t i = 0; i < server_configs.size(); ++i) {
        if (!socket(server_configs[i])) {
            std::cerr << "Failed to start server on " << server_configs[i].host << ":" << server_configs[i].listen_port << std::endl;
            return 1;
        }
    }

    pollfd stdin_fd = {STDIN_FILENO, POLLIN, 0};
    fds.push_back(stdin_fd);

    while (g_running)
    {
        if (poll(fds.data(), fds.size(), 1000) < 0) 
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Poll failed: " << strerror(errno) << std::endl;
            break;
        }   

        for (size_t i = 0; i < fds.size() && g_running; i++)
        {
            if (fds[i].fd == STDIN_FILENO && (fds[i].revents & POLLIN))
            {
                char buf[1];
                if (read(STDIN_FILENO, buf, 1) <= 0) {
                    std::cerr << "Received EOF (CTRL+D). Cleaning up and exiting..." << std::endl;
                    g_running = 0;
                    break;
                }
            }
            else if (fds[i].revents & (POLLIN | POLLERR | POLLHUP))
            {
                handle_client(&fds[0], i);
            }
        }
    }

    cleanup_resources();
    return 0;
}