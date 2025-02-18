/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 10:35:54 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/17 03:34:08 by dloisel          ###   ########.fr       */
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

// Fonction pour trouver la location correspondante à une URI
std::string findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations)
{
    for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        if (uri.find(it->first) == 0) // Vérifie si l'URI commence par le chemin de la location
        {
            return it->first;
        }
    }
    return "";
}

// Fonction pour joindre des éléments d'un vecteur en une chaîne
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
    char buffer[1024];

    // Création socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1)
    {
        std::cout << "Failed to create socket." << std::endl;
        return (false);
    }   

    // Socket non-bloquant
    fcntl(socketfd, F_SETFL, O_NONBLOCK);
    
    // Options socket
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cout << "Failed to set socket options." << std::endl;
        close(socketfd);
        return false;
    }   

    // Configuration adresse
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(config.listen_port); // Utilisation du port configuré
    
    // Bind
    if (bind(socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cout << "Failed to bind to port " << config.listen_port << "." << std::endl;
        return (false);
    }
    
    // Listen
    if (listen(socketfd, 10) < 0)
    {
        std::cout << "Failed to listen on socket." << std::endl;
        return (false);
    }   

    std::cout <<  "\033[1;32m" 
     << "\n""███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗     ██████╗ ███████╗ █████╗ ██████╗ ██╗   ██╗\n"
     << "██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗    ██╔══██╗██╔════╝██╔══██╗██╔══██╗╚██╗ ██╔╝\n"
     << "███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝    ██████╔╝█████╗  ███████║██║  ██║ ╚████╔╝ \n"
     << "╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗    ██╔══██╗██╔══╝  ██╔══██║██║  ██║  ╚██╔╝  \n"
     << "███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║    ██║  ██║███████╗██║  ██║██████╔╝   ██║   \n"
     << "╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝    ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝    ╚═╝   \n"
     << "\033[0m" "\n" << std::endl;
    
    // Initialisation poll avec socket serveur
    pollfd server_fd = {socketfd, POLLIN, 0};
    fds.push_back(server_fd);   

    // Boucle principale
    while (true)
    {
        if (poll(fds.data(), fds.size(), -1) < 0) 
        {
            std::cout << "Poll failed." << std::endl;
            break;
        }   

        // Parcours des fds actifs
        for (size_t i = 0; i < fds.size(); i++)
        {
            // Nouvelle connexion sur socket serveur
            if (fds[i].fd == socketfd && (fds[i].revents & POLLIN))
            {
                socklen_t addrlen = sizeof(sockaddr);
                int client_fd = accept(socketfd, (struct sockaddr*)&sockaddr, &addrlen);
                if (client_fd >= 0)
                {
                    fcntl(client_fd, F_SETFL, O_NONBLOCK);
                    pollfd client = {client_fd, POLLIN, 0};
                    fds.push_back(client);
                    std::cout << "New client connected." << std::endl;
                }
            }
            // Données reçues d'un client
            else if (fds[i].revents & POLLIN)
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                
                if (bytes <= 0)
                {
                    std::cout << "Client disconnected." << std::endl;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
                }
                else
                {
                    std::cout << "Received: " << buffer << std::endl;
                    HTTPRequest request(buffer);

                    // Trouver la location correspondante
                    std::string uri = request.getURI();
                    std::string location_path = findLocationForURI(uri, config.locations);

                    if (!location_path.empty())
                    {
                        const LocationConfig& location = config.locations.at(location_path);
                        // Appliquer les règles de la location (par exemple, vérifier les méthodes autorisées)
                        bool isMethodAllowed = false;
                        std::string method = request.getMethod();
                        for (std::vector<std::string>::const_iterator it = location.allowed_methods.begin(); it != location.allowed_methods.end(); ++it)
                        {
                            if (*it == method)
                            {
                                isMethodAllowed = true;
                                break;
                            }
                        }
                        
                        if (!isMethodAllowed)
                        {
                            // Method not allowed: Send a 405 Method Not Allowed response
                            std::string response = "HTTP/1.1 405 Method Not Allowed\r\n";
                            response += "Allow: ";
                            for (std::vector<std::string>::const_iterator it = location.allowed_methods.begin(); it != location.allowed_methods.end(); ++it)
                            {
                                response += *it;
                                if (it + 1 != location.allowed_methods.end())
                                {
                                    response += ", ";
                                }
                            }
                            response += "\r\n\r\n";
                            send(fds[i].fd, response.c_str(), response.size(), 0);
                            continue; // Skip further processing for this request
                        }
                        // Traiter la requête en fonction de la location
                        // ...
                    }

                    HTTPResponse::handle_request(request, fds[i].fd);
                }
            }
        }
    }   

    // Nettoyage
    for (size_t i = 0; i < fds.size(); i++)
        close(fds[i].fd);
    
    return true;
}