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
#include <sstream>

// Définition de la variable statique
const ServerConfig* HTTPResponse::config = NULL;

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
        // Configurer un timeout pour le CGI
        alarm(CGI_TIMEOUT);
        
        std::cout << "[DEBUG] Calling CGIHandler::executeCGI" << std::endl;
        std::string output = cgi.executeCGI(req.getMethod(), req.getURI(), req.getBody());
        
        // Désactiver le timeout
        alarm(0);
        
        std::cout << "[DEBUG] CGI Output: " << output.substr(0, 100) << "..." << std::endl;
        ssize_t sent = send(client_fd, output.c_str(), output.length(), 0);
        if (sent <= 0) {
            std::cout << "[DEBUG] Failed to send CGI output" << std::endl;
            sendErrorPage(client_fd, 500, req);
        }
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] CGI execution failed: " << e.what() << std::endl;
        sendErrorPage(client_fd, 500, req);
    }
}

void HTTPResponse::sendErrorPage(int client_fd, int error_code, HTTPRequest& req) {
    // Obtenir le User-Agent
    std::string user_agent = req.getHeader("User-Agent");
    bool is_browser = user_agent.find("Mozilla") != std::string::npos || 
                     user_agent.find("Chrome") != std::string::npos || 
                     user_agent.find("Safari") != std::string::npos || 
                     user_agent.find("Edge") != std::string::npos || 
                     user_agent.find("Opera") != std::string::npos;

    if (is_browser) {
        // Pour les navigateurs, envoyer la page HTML complète
        std::stringstream error_path;
        error_path << "./error/" << error_code << ".html";
        std::ifstream error_file(error_path.str().c_str());
        
        if (error_file.is_open()) {
            std::stringstream ss;
            ss << error_file.rdbuf();
            error_file.close();
            HTTPResponse resp(error_code, "text/html", ss.str());
            send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
            return;
        }
    }

    // Pour les terminaux ou si la page HTML n'existe pas
    std::string error_message;
    switch (error_code) {
        case 400: error_message = "Bad Request"; break;
        case 403: error_message = "Forbidden"; break;
        case 404: error_message = "Not Found"; break;
        case 405: error_message = "Method Not Allowed"; break;
        case 413: error_message = "Content Too Large"; break;
        case 500: error_message = "Internal Server Error"; break;
        case 501: error_message = "Not Implemented"; break;
        default: error_message = "Unknown Error"; break;
    }

    std::stringstream error_msg;
    error_msg << error_code << " " << error_message;
    HTTPResponse resp(error_code, "text/plain", error_msg.str());
    send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
}
void HTTPResponse::handle_request(HTTPRequest& req, int client_fd) {
    try {
        std::cout << "[DEBUG] Handling request for URI: " << req.getURI() << std::endl;
        std::cout << "[DEBUG] Method: " << req.getMethod() << std::endl;
        
        // Vérification de la validité de l'URI
        std::string uri = req.getURI();
        if (uri.find('\0') != std::string::npos) {
            sendErrorPage(client_fd, 400, req);
            return;
        }

        // Vérification de la méthode
        std::string method = req.getMethod();
        if (method.empty()) {
            sendErrorPage(client_fd, 400, req);
            return;
        }

    // Vérification des méthodes autorisées selon la configuration
    if (config) {
        std::string uri = req.getURI();
        bool method_allowed = false;

        // Cherche la location qui correspond à l'URI
        for (std::map<std::string, LocationConfig>::const_iterator it = config->locations.begin();
             it != config->locations.end(); ++it) {
            if (uri.find(it->first) == 0) {  // Si l'URI commence par le chemin de la location
                const LocationConfig& loc = it->second;
                // Vérifie si la méthode est autorisée
                for (size_t i = 0; i < loc.allowed_methods.size(); ++i) {
                    if (loc.allowed_methods[i] == method) {
                        method_allowed = true;
                        break;
                    }
                }
                if (!method_allowed) {
                    sendErrorPage(client_fd, 405, req);
                    return;
                }
                break;
            }
        }
    }

    // Vérification des méthodes autorisées pour index.html
    if (uri == "/index.html" || uri == "/") {
        if (method != "GET") {
            sendErrorPage(client_fd, 405, req);
            return;
        }
    }

    // Vérification de la taille du body
    if (req.getBody().length() > MAX_BODY_SIZE) {
        sendErrorPage(client_fd, 413, req);
        return;
    }

    // Vérification des chemins relatifs
    if (uri.find("../") != std::string::npos) {
        sendErrorPage(client_fd, 403, req);
        return;
    }

    // Traitement selon la méthode
    if (method == "GET" || method == "POST") {
        // Vérifie si c'est une requête CGI
        if (isCGI(uri)) {
            std::string script_path = "." + uri;
            // Vérifie d'abord si le fichier existe
            if (access(script_path.c_str(), F_OK) != 0) {
                sendErrorPage(client_fd, 404, req);
                return;
            }
            // Exécute le CGI et gère les erreurs
            try {
                executeCGI(script_path, req, client_fd);
            } catch (const std::runtime_error& e) {
                std::string error = e.what();
                if (error.find("Permission denied") != std::string::npos) {
                    sendErrorPage(client_fd, 403, req);
                } else {
                    sendErrorPage(client_fd, 500, req);
                }
            } catch (...) {
                sendErrorPage(client_fd, 500, req);
            }
            return;
        }

        if (method == "POST") {
            std::string file_path = "." + uri;
            
            // Vérifie si le fichier existe déjà
            if (access(file_path.c_str(), F_OK) == 0) {
                // Vérifie les droits d'écriture
                if (access(file_path.c_str(), W_OK) != 0) {
                    sendErrorPage(client_fd, 403, req);
                    return;
                }
            } else {
                // Vérifie les droits d'écriture sur le dossier parent
                std::string parent_dir = file_path.substr(0, file_path.find_last_of('/'));
                if (access(parent_dir.c_str(), W_OK) != 0) {
                    sendErrorPage(client_fd, 403, req);
                    return;
                }
            }
            
            // Tente de créer ou modifier le fichier
            std::ofstream file(file_path.c_str());
            if (!file.is_open()) {
                sendErrorPage(client_fd, 500, req);
                return;
            }
            file << req.getBody();
            file.close();
            
            HTTPResponse resp(201, "text/plain", "File created successfully");
            send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
            return;
        }
        
        // GET : traite comme un fichier normal
        std::string file_path = "." + uri;
        std::ifstream file(file_path.c_str());
        
        if (!file.is_open()) {
            sendErrorPage(client_fd, 404, req);
            return;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        file.close();

        HTTPResponse resp(200, "text/html", ss.str());
        send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        return;
    }
    else if (method == "DELETE") {
        std::string file_path = "." + uri;
        
        // Vérifie d'abord si le fichier existe
        if (access(file_path.c_str(), F_OK) != 0) {
            sendErrorPage(client_fd, 404, req);
            return;
        }
        
        // Vérifie si on a les droits d'écriture
        if (access(file_path.c_str(), W_OK) != 0) {
            sendErrorPage(client_fd, 403, req);
            return;
        }
        
        // Tente de supprimer le fichier
        if (remove(file_path.c_str()) == 0) {
            HTTPResponse resp(200, "text/plain", "File deleted successfully");
            send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        } else {
            sendErrorPage(client_fd, 500, req);
        }
        return;
    }
    
        // Si on arrive ici, c'est qu'il y a un problème
        sendErrorPage(client_fd, 500, req);
    } catch (const std::runtime_error& e) {
        std::string error = e.what();
        if (error == "501 Not Implemented") {
            sendErrorPage(client_fd, 501, req);
            return;
        }
        sendErrorPage(client_fd, 500, req);
    } catch (...) {
        sendErrorPage(client_fd, 500, req);
    }
}