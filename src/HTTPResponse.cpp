/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 15:40:57 by dmathis           #+#    #+#             */
/*   Updated: 2025/02/21 22:52:06 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HTTPResponse.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include "CGIHandler.hpp" 

const ServerConfig* HTTPResponse::_config = NULL;

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
    if (!_config) return false;

    if (uri.find("/cgi-bin/") != 0) {
        return false;
    }

    size_t dot_pos = uri.find_last_of('.');
    if (dot_pos == std::string::npos) return false;
    
    std::string extension = uri.substr(dot_pos);
    return (extension == ".py" || extension == ".sh");
}

void HTTPResponse::executeCGI(const std::string& script_path, HTTPRequest& req, int client_fd) {
    struct stat script_stat;
    if (stat(script_path.c_str(), &script_stat) != 0) {
        sendErrorPage(client_fd, 404, req);
        return;
    }

    if (!(script_stat.st_mode & S_IRUSR) || !(script_stat.st_mode & S_IXUSR)) {
        sendErrorPage(client_fd, 403, req);
        return;
    }

    try {
        CGIHandler cgi(script_path);
        std::string contentType = req.getHeader("Content-Type");
        
        std::string cgi_response = cgi.executeCGI(
            req.getMethod(),
            "",  
            req.getBody(),
            contentType
        );

     
        std::stringstream response_stream;
        std::istringstream iss(cgi_response);
        std::string line;
        bool headers_done = false;
        int status_code = 200;
        std::string status_message = "OK";
        std::string content_type = "text/plain";
        std::string body;

    
        while (std::getline(iss, line)) {
      
            if (!line.empty() && line[line.length()-1] == '\r') {
                line = line.substr(0, line.length()-1);
            }

            if (line.empty()) {
                headers_done = true;
                continue;
            }
            
            if (!headers_done) {
                if (line.find("Status:") == 0) {
                    std::string status = line.substr(7);
          
                    status = status.substr(status.find_first_not_of(" \t"));
                    size_t space_pos = status.find(' ');
                    if (space_pos != std::string::npos) {
                        status_code = std::atoi(status.substr(0, space_pos).c_str());
                        status_message = status.substr(space_pos + 1);
                    }
                }
                else if (line.find("Content-type:") == 0 || line.find("Content-Type:") == 0) {
                    content_type = line.substr(line.find(':') + 1);
               
                    content_type = content_type.substr(content_type.find_first_not_of(" \t"));
                }
            }
            else {
                body += line + "\n";
            }
        }

   
        response_stream << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
        response_stream << "Content-Type: " << content_type << "\r\n";
        response_stream << "Content-Length: " << body.length() << "\r\n";
        response_stream << "\r\n";
        response_stream << body;

        std::cerr << "=== Response Debug ===\n";
        std::cerr << "Status code: " << status_code << std::endl;
        std::cerr << "Status message: " << status_message << std::endl;
        std::cerr << "Content-Type: " << content_type << std::endl;
        std::cerr << "Body length: " << body.length() << std::endl;
        std::cerr << "First 100 chars of body:\n" << body.substr(0, 100) << std::endl;

     
        std::string response = response_stream.str();
        if (send(client_fd, response.c_str(), response.length(), 0) < 0) {
            throw std::runtime_error("Failed to send response");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        sendErrorPage(client_fd, 500, req);
    }
}

void HTTPResponse::sendErrorPage(int client_fd, int error_code, HTTPRequest& req) {
  
    std::string user_agent = req.getHeader("User-Agent");
    bool is_browser = (user_agent.find("Mozilla") != std::string::npos || 
                     user_agent.find("Chrome") != std::string::npos || 
                     user_agent.find("Safari") != std::string::npos || 
                     user_agent.find("Edge") != std::string::npos || 
                     user_agent.find("Opera") != std::string::npos);

    if (is_browser) {
    
        std::stringstream error_path;
        error_path << "./error/" << error_code << ".html";
        std::ifstream error_file(error_path.str().c_str());
        
        if (error_file.is_open()) {
            std::stringstream ss;
            ss << error_file.rdbuf();
            error_file.close();
            HTTPResponse resp(error_code, "text/html", ss.str());
            try {
                std::string response = resp.toString();
                sendResponse(client_fd, response);
            } catch (const std::exception& e) {
         
                close(client_fd);
            }
            return;
        }
    }

  
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
    try {
        std::string response = resp.toString();
        sendResponse(client_fd, response);
    } catch (const std::exception& e) {
   
        close(client_fd);
    }
}

void HTTPResponse::sendResponse(int client_fd, const std::string& response_data) {
    size_t total_sent = 0;
    size_t len = response_data.length();
    
    while (total_sent < len) {
        ssize_t sent = send(client_fd, response_data.c_str() + total_sent, len - total_sent, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              
                return;
            }
          
            close(client_fd);
            return;
        }
        total_sent += sent;
    }
}

bool HTTPResponse::isMethodAllowed(const std::string& uri, const std::string& method) {
    if (!_config) return false;

  
    std::string best_match;
    const LocationConfig* matching_location = NULL;
    
  
    for (std::map<std::string, LocationConfig>::const_iterator it = _config->locations.begin(); 
         it != _config->locations.end(); ++it) {
        if (uri.find(it->first) == 0 && it->first.length() > best_match.length()) {
            best_match = it->first;
            matching_location = &it->second;
        }
    }

  
    if (!matching_location) {
        matching_location = &_config->locations.at("/");
    }

   
    if (matching_location->allowed_methods.empty() || 
        (matching_location->allowed_methods.size() == 1 && matching_location->allowed_methods[0] == "NONE")) {
        return false;
    }

  
    return (std::find(matching_location->allowed_methods.begin(), 
                    matching_location->allowed_methods.end(), 
                    method) != matching_location->allowed_methods.end());
}

void HTTPResponse::handle_request(HTTPRequest& req, int client_fd) {
    try {
        std::string uri = req.getURI();
        std::string method = req.getMethod();
        
    
        std::string host = req.getHeader("Host");
        if (host.empty()) {
            sendErrorPage(client_fd, 400, req);
            return;
        }

    
        size_t colon_pos = host.find(':');
        std::string hostname = (colon_pos != std::string::npos) ? host.substr(0, colon_pos) : host;

   
        if (hostname != _config->host) {
            sendErrorPage(client_fd, 400, req);
            return;
        }

   
        if (!isMethodAllowed(uri, method)) {
            if ((uri == "/forbidden") || (uri.find("/forbidden/") == 0)) {
                sendErrorPage(client_fd, 403, req);
            } else {
                sendErrorPage(client_fd, 405, req);
            }
            return;
        }

  
        std::string location = findLocationForURI(uri, _config->locations);
        const LocationConfig& loc_config = _config->locations.at(location);

   
        std::string root = loc_config.root;
    
        if (!root.empty() && root[root.length() - 1] == '/') {
            root = root.substr(0, root.length() - 1);
        }

        std::string full_path = root + uri;

        size_t pos = 0;
        while ((pos = full_path.find("//", pos)) != std::string::npos) {
            full_path.replace(pos, 2, "/");
        }

        struct stat path_stat;
        if ((method == "GET")) {
            if (isCGI(uri)) {
                executeCGI(full_path, req, client_fd);
                return;
            }

       
            if (stat(full_path.c_str(), &path_stat) != 0) {
                sendErrorPage(client_fd, 404, req);
                return;
            }

       
            if (S_ISDIR(path_stat.st_mode) || uri == "/" || uri[uri.length() - 1] == '/') {
           
                if (loc_config.autoindex) {
                    generateDirectoryListing(full_path, client_fd);
                    return;
                }
                
             
                std::string index_path;
                if (!loc_config.index.empty()) {
                    index_path = full_path + (uri == "/" ? "" : "/") + loc_config.index;
                } else {
                    index_path = full_path + (uri == "/" ? "" : "/") + "index.html";
                }
                
                if (stat(index_path.c_str(), &path_stat) == 0) {
                    serveFile(index_path, client_fd);
                } else {
                    sendErrorPage(client_fd, 403, req);
                }
                return;
            }

      
            serveFile(full_path, client_fd);
        }
        else if ((method == "POST")) {
       
            if (isCGI(uri)) {
                executeCGI(full_path, req, client_fd);
                return;
            }

            if (((stat(full_path.c_str(), &path_stat) == 0) && (S_ISDIR(path_stat.st_mode))) || 
                (full_path[full_path.length() - 1] == '/')) {
                
                char buffer[20];
                sprintf(buffer, "%ld", time(NULL));
                full_path = full_path + (full_path[full_path.length() - 1] == '/' ? "" : "/") + 
                           "post_" + std::string(buffer) + ".txt";
            }

       
            size_t last_slash = full_path.find_last_of('/');
            if ((last_slash != std::string::npos)) {
                std::string dir = full_path.substr(0, last_slash);
                struct stat dir_stat;
                if (stat(dir.c_str(), &dir_stat) != 0) {
                    if (mkdir(dir.c_str(), 0755) != 0) {
                        sendErrorPage(client_fd, 500, req);
                        return;
                    }
                }
            }

           
            std::ofstream file(full_path.c_str(), std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                sendErrorPage(client_fd, 500, req);
                return;
            }

            file.write(req.getBody().c_str(), req.getBody().length());
            file.close();

            if (file.fail()) {
                sendErrorPage(client_fd, 500, req);
                return;
            }

            HTTPResponse resp(201, "text/plain", "Resource created successfully");
            try {
                std::string response = resp.toString();
                sendResponse(client_fd, response);
            } catch (const std::exception& e) {
             
                close(client_fd);
            }
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
            try {
                std::string response = resp.toString();
                sendResponse(client_fd, response);
            } catch (const std::exception& e) {
               
                close(client_fd);
            }
        }
        else {
            sendErrorPage(client_fd, 501, req);
        }
    }
    catch (const std::exception& e) {
        sendErrorPage(client_fd, 500, req);
    }
}

void HTTPResponse::serveFile(const std::string& path, int client_fd) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        HTTPResponse resp(500, "text/plain", "Internal server error");
        try {
            std::string response = resp.toString();
            sendResponse(client_fd, response);
        } catch (const std::exception& e) {
          
            close(client_fd);
        }
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
    try {
        std::string response = resp.toString();
        sendResponse(client_fd, response);
    } catch (const std::exception& e) {

        close(client_fd);
    }
}

void HTTPResponse::generateDirectoryListing(const std::string& path, int client_fd) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        HTTPResponse resp(500, "text/plain", "Internal server error");
        try {
            std::string response = resp.toString();
            sendResponse(client_fd, response);
        } catch (const std::exception& e) {

            close(client_fd);
        }
        return;
    }


    std::string location_root;
    std::string current_uri;
    
    std::map<std::string, LocationConfig>::const_iterator it;
    for (it = _config->locations.begin(); it != _config->locations.end(); ++it) {
        const std::string& loc_path = it->first;
        const LocationConfig& loc = it->second;
        
        if (path.find(loc.root) == 0) {
            location_root = loc.root;

            current_uri = loc_path;

            if (path.length() > loc.root.length()) {
                std::string relative_path = path.substr(loc.root.length());

                while (relative_path[0] == '/') {
                    relative_path = relative_path.substr(1);
                }
                if (!relative_path.empty()) {
                    current_uri += "/" + relative_path;
                }
            }
            break;
        }
    }

    if (current_uri[current_uri.length() - 1] != '/') {
        current_uri += "/";
    }

    std::stringstream html;
    html << "<html><head><title>Index of " << current_uri << "</title>";
    html << "<style>";
    html << "body { ";
    html << "    background-color: #1a1a1a; ";
    html << "    color: #c0c0c0; ";
    html << "    font-family: 'Cinzel', serif; ";
    html << "    margin: 0; ";
    html << "    padding: 20px; ";
    html << "    background-image: linear-gradient(to bottom, #1a1a1a, #2d2d2d); ";
    html << "    min-height: 100vh; ";
    html << "}";
    html << "h1 { ";
    html << "    color: #8b0000; ";
    html << "    text-align: center; ";
    html << "    font-size: 2.5em; ";
    html << "    text-shadow: 2px 2px 4px #000; ";
    html << "    margin-bottom: 30px; ";
    html << "    font-family: 'MedievalSharp', cursive; ";
    html << "}";
    html << ".container { ";
    html << "    background-color: rgba(0, 0, 0, 0.7); ";
    html << "    border: 2px solid #4a0000; ";
    html << "    border-radius: 8px; ";
    html << "    padding: 20px; ";
    html << "    box-shadow: 0 0 15px rgba(139, 0, 0, 0.3); ";
    html << "}";
    html << ".file-list { ";
    html << "    list-style: none; ";
    html << "    padding: 0; ";
    html << "}";
    html << ".file-item { ";
    html << "    display: flex; ";
    html << "    justify-content: space-between; ";
    html << "    align-items: center; ";
    html << "    padding: 10px 15px; ";
    html << "    margin: 5px 0; ";
    html << "    background-color: rgba(30, 30, 30, 0.9); ";
    html << "    border: 1px solid #4a0000; ";
    html << "    border-radius: 4px; ";
    html << "}";
    html << ".file-name { ";
    html << "    color: #d4af37; ";
    html << "    font-weight: bold; ";
    html << "}";
    html << ".file-info { ";
    html << "    color: #808080; ";
    html << "    font-size: 0.9em; ";
    html << "}";
    html << ".directory { ";
    html << "    color: #cd7f32; ";
    html << "}";
    html << "@import url('https://fonts.googleapis.com/css2?family=MedievalSharp&display=swap');";
    html << "</style>";
    html << "</head><body>";
    html << "<h1>Directory of " << current_uri << "</h1>";
    html << "<div class='container'><ul class='file-list'>";

    if (current_uri != "/") {
        html << "<li class='file-item'>";
        html << "<span class='file-name directory'>..</span>";
        html << "<span class='file-info'>Parent Directory</span>";
        html << "</li>";
    }

    struct dirent* entry;
    std::vector<std::string> entries;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    }

    std::sort(entries.begin(), entries.end());

    std::vector<std::string>::const_iterator name_it;
    for (name_it = entries.begin(); name_it != entries.end(); ++name_it) {
        const std::string& name = *name_it;
        struct stat st;
        std::string full_path = path + "/" + name;
        if (stat(full_path.c_str(), &st) == 0) {
            char time_str[26];
            ctime_r(&st.st_mtime, time_str);
            time_str[24] = '\0';
            
            html << "<li class='file-item'>";
            html << "<span class='file-name";
            if (S_ISDIR(st.st_mode)) html << " directory";
            html << "'>" << name;
            if (S_ISDIR(st.st_mode)) html << "/";
            html << "</span>";
            
            std::stringstream info;
            info << time_str << " - ";
            if (S_ISDIR(st.st_mode)) 
                info << "Directory";
            else 
                info << st.st_size << " bytes";
            
            html << "<span class='file-info'>" << info.str() << "</span>";
            html << "</li>";
        }
    }
    
    html << "</ul></div></body></html>";
    
    HTTPResponse resp(200, "text/html", html.str());
    try {
        std::string response = resp.toString();
        sendResponse(client_fd, response);
    } catch (const std::exception& e) {

        close(client_fd);
    }
    
    closedir(dir); 
}

std::string HTTPResponse::findLocationForURI(const std::string& uri, const std::map<std::string, LocationConfig>& locations) {
    std::string best_match = "/";
    std::map<std::string, LocationConfig>::const_iterator it;
    for (it = locations.begin(); it != locations.end(); ++it) {
        if ((uri.find(it->first) == 0) && (it->first.length() > best_match.length())) {
            best_match = it->first;
        }
    }
    return best_match;
}