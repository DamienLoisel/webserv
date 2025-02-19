/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 15:40:57 by dmathis           #+#    #+#             */
/*   Updated: 2025/02/19 12:34:03 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPResponse.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>

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
    if (!config) return false;

    if (uri.find("/cgi-bin/") != 0) {
        return false;
    }

    size_t dot_pos = uri.find_last_of('.');
    if (dot_pos == std::string::npos) return false;
    
    std::string extension = uri.substr(dot_pos);
    return (extension == ".py" || extension == ".sh");
}

void HTTPResponse::executeCGI(const std::string& script_path, HTTPRequest& req, int client_fd) {
    std::cout << "[DEBUG] Executing CGI script: " << script_path << std::endl;
    std::cout << "[DEBUG] Method: " << req.getMethod() << std::endl;
    std::cout << "[DEBUG] URI: " << req.getURI() << std::endl;

    struct stat script_stat;
    if (stat(script_path.c_str(), &script_stat) != 0) {
        std::cout << "[ERROR] CGI script not found: " << script_path << std::endl;
        sendErrorPage(client_fd, 404, req);
        return;
    }

    if (!(script_stat.st_mode & S_IRUSR) || !(script_stat.st_mode & S_IXUSR)) {
        std::cout << "[ERROR] CGI script not executable: " << script_path << std::endl;
        sendErrorPage(client_fd, 403, req);
        return;
    }

    try {
        // Trouve l'interpréteur approprié
        std::string interpreter;
        size_t dot_pos = script_path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = script_path.substr(dot_pos);
            if (ext == ".py") {
                interpreter = "/usr/bin/python3";
            }
            else if (ext == ".sh") {
                interpreter = "/bin/bash";
            }
        }

        if (interpreter.empty()) {
            std::cout << "[ERROR] No interpreter found for: " << script_path << std::endl;
            sendErrorPage(client_fd, 500, req);
            return;
        }

        // Exécute le script CGI avec un timeout
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            std::cout << "[ERROR] Failed to create pipe" << std::endl;
            sendErrorPage(client_fd, 500, req);
            return;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            std::cout << "[ERROR] Fork failed" << std::endl;
            sendErrorPage(client_fd, 500, req);
            return;
        }

        if (pid == 0) {  // Child process
            close(pipefd[0]);  // Close read end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // Set up environment variables
            std::stringstream content_length;
            content_length << req.getBody().length();
            
            setenv("SCRIPT_NAME", script_path.c_str(), 1);
            setenv("REQUEST_METHOD", req.getMethod().c_str(), 1);
            setenv("CONTENT_LENGTH", content_length.str().c_str(), 1);
            setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);

            // Execute the script
            execl(interpreter.c_str(), interpreter.c_str(), script_path.c_str(), NULL);
            exit(1);  // In case execl fails
        }

        // Parent process
        close(pipefd[1]);  // Close write end

        // Read output with timeout
        fd_set read_fds;
        struct timeval timeout;
        char buffer[4096];
        std::string output;
        bool timeout_occurred = false;

        while (true) {
            FD_ZERO(&read_fds);
            FD_SET(pipefd[0], &read_fds);
            timeout.tv_sec = 5;  // 5 seconds timeout
            timeout.tv_usec = 0;

            int ready = select(pipefd[0] + 1, &read_fds, NULL, NULL, &timeout);
            if (ready == -1) {
                std::cout << "[ERROR] Select failed" << std::endl;
                break;
            }
            else if (ready == 0) {
                std::cout << "[ERROR] CGI timeout" << std::endl;
                timeout_occurred = true;
                break;
            }

            ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) break;
            
            buffer[bytes_read] = '\0';
            output += buffer;
        }

        close(pipefd[0]);

        // Kill child process if timeout occurred
        if (timeout_occurred) {
            kill(pid, SIGKILL);
            sendErrorPage(client_fd, 504, req);  // Gateway Timeout
            return;
        }

        // Wait for child process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Format CGI response
            std::stringstream content_length;
            content_length << output.length();
            
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/plain\r\n";
            response += "Content-Length: " + content_length.str() + "\r\n";
            response += "\r\n";
            response += output;

            send(client_fd, response.c_str(), response.length(), 0);
        }
        else {
            std::cout << "[ERROR] CGI script failed with status: " << WEXITSTATUS(status) << std::endl;
            sendErrorPage(client_fd, 500, req);
        }
    }
    catch (const std::exception& e) {
        std::cout << "[ERROR] CGI execution failed: " << e.what() << std::endl;
        sendErrorPage(client_fd, 500, req);
    }
}

void HTTPResponse::sendErrorPage(int client_fd, int error_code, HTTPRequest& req) {
    // Obtenir le User-Agent
    std::string user_agent = req.getHeader("User-Agent");
    bool is_browser = (user_agent.find("Mozilla") != std::string::npos || 
                     user_agent.find("Chrome") != std::string::npos || 
                     user_agent.find("Safari") != std::string::npos || 
                     user_agent.find("Edge") != std::string::npos || 
                     user_agent.find("Opera") != std::string::npos);

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

bool HTTPResponse::isMethodAllowed(const std::string& uri, const std::string& method) {
    if (!config) return false;

    // Trouve la location correspondante
    std::string best_match;
    const LocationConfig* matching_location = NULL;
    
    // Recherche la meilleure correspondance de location
    for (std::map<std::string, LocationConfig>::const_iterator it = config->locations.begin(); 
         it != config->locations.end(); ++it) {
        if (uri.find(it->first) == 0 && it->first.length() > best_match.length()) {
            best_match = it->first;
            matching_location = &it->second;
        }
    }

    // Si aucune location n'est trouvée, on utilise la configuration par défaut
    if (!matching_location) {
        matching_location = &config->locations.at("/");
    }

    // Si allowed_methods est vide ou contient "NONE", aucune méthode n'est autorisée
    if (matching_location->allowed_methods.empty() || 
        (matching_location->allowed_methods.size() == 1 && matching_location->allowed_methods[0] == "NONE")) {
        return false;
    }

    // Vérifie si la méthode est dans la liste des méthodes autorisées
    return (std::find(matching_location->allowed_methods.begin(), 
                    matching_location->allowed_methods.end(), 
                    method) != matching_location->allowed_methods.end());
}

void HTTPResponse::handle_request(HTTPRequest& req, int client_fd) {
    try {
        std::string uri = req.getURI();
        std::string method = req.getMethod();
        
        std::cout << "[DEBUG] Handling request for URI: " << uri << std::endl;
        std::cout << "[DEBUG] Method: " << method << std::endl;

        // Vérifier si la méthode est autorisée
        if (!isMethodAllowed(uri, method)) {
            if ((uri == "/forbidden") || (uri.find("/forbidden/") == 0)) {
                sendErrorPage(client_fd, 403, req);
            } else {
                sendErrorPage(client_fd, 405, req);
            }
            return;
        }

        // Trouver la location correspondante
        std::string location = findLocationForURI(uri, config->locations);
        const LocationConfig& loc_config = config->locations.at(location);

        std::cout << "[DEBUG] Location found: " << location << std::endl;
        std::cout << "[DEBUG] Root: " << loc_config.root << std::endl;

        // Construire le chemin complet
        std::string root = loc_config.root;
        // Supprimer le / à la fin s'il existe
        if (!root.empty() && root[root.length() - 1] == '/') {
            root = root.substr(0, root.length() - 1);
        }

        std::string full_path = root + uri;
        // Supprimer les doubles slashes
        size_t pos = 0;
        while ((pos = full_path.find("//", pos)) != std::string::npos) {
            full_path.replace(pos, 2, "/");
        }

        std::cout << "[DEBUG] Full path: " << full_path << std::endl;

        struct stat path_stat;
        if ((method == "GET")) {
            if (isCGI(uri)) {
                executeCGI(full_path, req, client_fd);
                return;
            }

            // Si le fichier n'existe pas
            if (stat(full_path.c_str(), &path_stat) != 0) {
                sendErrorPage(client_fd, 404, req);
                return;
            }

            // Si c'est un répertoire
            if (S_ISDIR(path_stat.st_mode)) {
                std::string index_path = full_path + "/index.html";
                if (stat(index_path.c_str(), &path_stat) == 0) {
                    serveFile(index_path, client_fd);
                } else if (loc_config.autoindex) {
                    generateDirectoryListing(full_path, client_fd);
                } else {
                    sendErrorPage(client_fd, 403, req);
                }
                return;
            }

            // Servir le fichier
            serveFile(full_path, client_fd);
        }
        else if ((method == "POST")) {
            std::cout << "[DEBUG] POST request body: " << req.getBody() << std::endl;

            // Vérifier si le chemin est un répertoire ou se termine par /
            if (((stat(full_path.c_str(), &path_stat) == 0) && (S_ISDIR(path_stat.st_mode))) || 
                (full_path[full_path.length() - 1] == '/')) {
                // Créer un nouveau fichier avec un nom unique dans le répertoire
                char buffer[20];
                sprintf(buffer, "%ld", time(NULL));
                full_path = full_path + (full_path[full_path.length() - 1] == '/' ? "" : "/") + 
                           "post_" + std::string(buffer) + ".txt";
                std::cout << "[DEBUG] Creating file at: " << full_path << std::endl;
            }

            // Créer le répertoire parent si nécessaire
            size_t last_slash = full_path.find_last_of('/');
            if ((last_slash != std::string::npos)) {
                std::string dir = full_path.substr(0, last_slash);
                std::cout << "[DEBUG] Parent directory: " << dir << std::endl;
                struct stat dir_stat;
                if (stat(dir.c_str(), &dir_stat) != 0) {
                    std::cout << "[DEBUG] Creating parent directory" << std::endl;
                    // Créer le répertoire avec les permissions 755
                    if (mkdir(dir.c_str(), 0755) != 0) {
                        std::cout << "[ERROR] Failed to create directory: " << strerror(errno) << std::endl;
                        sendErrorPage(client_fd, 500, req);
                        return;
                    }
                }
            }

            // Écrire le fichier
            std::cout << "[DEBUG] Opening file for writing: " << full_path << std::endl;
            std::ofstream file(full_path.c_str(), std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                std::cout << "[ERROR] Failed to open file: " << strerror(errno) << std::endl;
                sendErrorPage(client_fd, 500, req);
                return;
            }

            file.write(req.getBody().c_str(), req.getBody().length());
            file.close();

            if (file.fail()) {
                std::cout << "[ERROR] Failed to write to file" << std::endl;
                sendErrorPage(client_fd, 500, req);
                return;
            }

            std::cout << "[DEBUG] File written successfully" << std::endl;
            HTTPResponse resp(201, "text/plain", "Resource created successfully");
            send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        }
        else if ((method == "DELETE")) {
            if (stat(full_path.c_str(), &path_stat) != 0) {
                sendErrorPage(client_fd, 404, req);
                return;
            }

            if (!S_ISREG(path_stat.st_mode)) {
                sendErrorPage(client_fd, 403, req);
                return;
            }

            if (remove(full_path.c_str()) != 0) {
                sendErrorPage(client_fd, 500, req);
                return;
            }

            HTTPResponse resp(200, "text/plain", "Resource deleted successfully");
            send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        }
        else {
            sendErrorPage(client_fd, 501, req);
        }
    }
    catch (const std::exception& e) {
        std::cout << "[ERROR] " << e.what() << std::endl;
        sendErrorPage(client_fd, 500, req);
    }
}

void HTTPResponse::serveFile(const std::string& path, int client_fd) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        HTTPResponse resp(500, "text/plain", "Internal server error");
        send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        return;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    std::string ext = path.substr(path.find_last_of('.') + 1);
    std::string content_type = "text/plain";
    
    if ((ext == "html") || (ext == "htm")) content_type = "text/html";
    else if (ext == "css") content_type = "text/css";
    else if (ext == "js") content_type = "application/javascript";
    else if ((ext == "jpg") || (ext == "jpeg")) content_type = "image/jpeg";
    else if (ext == "png") content_type = "image/png";
    else if (ext == "gif") content_type = "image/gif";

    HTTPResponse resp(200, content_type, ss.str());
    send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
}

void HTTPResponse::generateDirectoryListing(const std::string& path, int client_fd) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        HTTPResponse resp(500, "text/plain", "Internal server error");
        send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
        return;
    }

    std::stringstream html;
    html << "<html><head><title>Index of " << path << "</title></head><body>";
    html << "<h1>Index of " << path << "</h1><hr><pre>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if ((name != ".") && (name != "..")) {
            struct stat st;
            std::string full_path = path + "/" + name;
            if (stat(full_path.c_str(), &st) == 0) {
                char time_str[26];
                ctime_r(&st.st_mtime, time_str);
                time_str[24] = '\0';  // Remove newline
                
                html << "<a href=\"" << name;
                if (S_ISDIR(st.st_mode)) html << "/";
                html << "\">" << name;
                if (S_ISDIR(st.st_mode)) html << "/";
                html << "</a>";
                
                html << std::string(50 - name.length(), ' ');
                html << time_str << "  ";
                
                std::stringstream size_str;
                if (S_ISDIR(st.st_mode)) 
                    size_str << "-";
                else 
                    size_str << st.st_size;
                
                html << size_str.str();
                html << "\n";
            }
        }
    }
    closedir(dir);

    html << "</pre><hr></body></html>";
    
    HTTPResponse resp(200, "text/html", html.str());
    send(client_fd, resp.toString().c_str(), resp.toString().length(), 0);
}

std::string HTTPResponse::findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations) {
    std::string best_match = "/";
    for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        if ((uri.find(it->first) == 0) && (it->first.length() > best_match.length())) {
            best_match = it->first;
        }
    }
    return best_match;
}