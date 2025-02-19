/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 17:27:08 by dloisel           #+#    #+#             */
/*   Updated: 2025/01/26 17:42:46 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

void ConfigParser::trim(std::string& str)
{
    size_t semicolon_pos = str.find(';');
    if (semicolon_pos != std::string::npos) {
        str = str.substr(0, semicolon_pos);
    }
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    str = (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::vector<std::string> ConfigParser::split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		trim(token);
		if (!token.empty()) tokens.push_back(token);
	}
	return tokens;
}

void ConfigParser::parseLocationBlock(std::istringstream& block_stream, LocationConfig& loc)
{
	std::string line, key;
	while (std::getline(block_stream, line) && line.find("}") == std::string::npos)
	{
		trim(line);
		if (line.empty()) continue;

		std::istringstream line_stream(line);
		line_stream >> key;

		if (key == "root") line_stream >> loc.root;
		else if (key == "autoindex") line_stream >> loc.autoindex;
		else if (key == "allow_methods") 
		{
			std::string method;
			while (line_stream >> method)
				loc.allowed_methods.push_back(method);
		}
		else if (key == "index") line_stream >> loc.index;
		else if (key == "return") line_stream >> loc.return_path;
		else if (key == "alias") line_stream >> loc.alias;
		else if (key == "cgi_path") 
		{
			std::string path;
			while (line_stream >> path)
				loc.cgi_path += path + " ";
		}
		else if (key == "cgi_ext") 
		{
			std::string ext;
			while (line_stream >> ext)
				loc.cgi_ext.push_back(ext);
		}
	}
}

bool ConfigParser::parse(const std::string& filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file" << std::endl;
        return false;
    }

    std::string line;
    ServerConfig current_server;
    bool in_server_block = false;
    std::string block_content;

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.find("server {") != std::string::npos) {
            if (in_server_block) {
                std::cerr << "Error: Nested server blocks not allowed" << std::endl;
                return false;
            }
            in_server_block = true;
            current_server = ServerConfig();
            continue;
        }

        if (line == "}") {
            if (in_server_block) {
                servers.push_back(current_server);
                if (servers.size() > MAX_SERVERS) {
                    std::cerr << "Error: Maximum number of servers (" << MAX_SERVERS << ") exceeded" << std::endl;
                    return false;
                }
                in_server_block = false;
            }
            continue;
        }

        if (in_server_block) {
            if (line.find("location") == 0) {
                std::string location_path = line.substr(9);
                trim(location_path);
                size_t brace_pos = location_path.find("{");
                if (brace_pos != std::string::npos) {
                    location_path = location_path.substr(0, brace_pos);
                    trim(location_path);
                }
                
                LocationConfig loc;
                std::string location_block;
                while (std::getline(file, line) && line.find("}") == std::string::npos) {
                    location_block += line + "\n";
                }
                std::istringstream block_stream(location_block);
                parseLocationBlock(block_stream, loc);
                current_server.locations[location_path] = loc;
            } else {
                std::istringstream line_stream(line);
                std::string key;
                line_stream >> key;

                if (key == "listen") {
                    line_stream >> current_server.listen_port;
                }
                else if (key == "host") {
                    line_stream >> current_server.host;
                }
                else if (key == "server_name") {
                    line_stream >> current_server.server_name;
                }
                else if (key == "error_page") {
                    line_stream >> current_server.error_page;
                }
                else if (key == "client_max_body_size") {
                    line_stream >> current_server.client_max_body_size;
                }
                else if (key == "root") {
                    line_stream >> current_server.root;
                }
                else if (key == "index") {
                    line_stream >> current_server.index;
                }
            }
        }
    }

    if (servers.empty()) {
        std::cerr << "Error: No server configuration found" << std::endl;
        return false;
    }

    if (!checkDuplicateServers()) {
        std::cerr << "Error: Duplicate server configurations (same IP:port) found" << std::endl;
        return false;
    }

    return true;
}

bool ConfigParser::checkDuplicateServers() const {
    for (size_t i = 0; i < servers.size(); ++i) {
        for (size_t j = i + 1; j < servers.size(); ++j) {
            if (servers[i].host == servers[j].host && 
                servers[i].listen_port == servers[j].listen_port) {
                return false;
            }
        }
    }
    return true;
}