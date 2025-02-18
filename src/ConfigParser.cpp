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
	std::ifstream config_file(filename.c_str());
	if (!config_file.is_open()) 
	{
		std::cerr << "Cannot open configuration file" << std::endl;
		return false;
	}
	std::string line, key;
	std::string current_block;
	while (std::getline(config_file, line)) 
	{
		trim(line);
		if (line.empty() || line[0] == '#') continue;
		if (line == "server {") 
		{
			current_block = "server";
			continue;
		}
		if (current_block == "server") {
			if (line.find("location") != std::string::npos) {
				std::string location_name = line.substr(9, line.length() - 11);
				LocationConfig loc;
				
				std::istringstream location_stream(line);
				std::string block_line;
				std::string block_content;
				while (std::getline(config_file, block_line) && block_line.find("}") == std::string::npos)
					block_content += block_line + "\n";
				std::istringstream block_stream(block_content);
				parseLocationBlock(block_stream, loc);
				server.locations[location_name] = loc;
				continue;
			}

			std::istringstream line_stream(line);
			line_stream >> key;

			if (key == "listen") line_stream >> server.listen_port;
			else if (key == "host") line_stream >> server.host;
			else if (key == "server_name") line_stream >> server.server_name;
			else if (key == "error_page") line_stream >> server.error_page;
			else if (key == "client_max_body_size") line_stream >> server.client_max_body_size;
			else if (key == "root") line_stream >> server.root;
			else if (key == "index") line_stream >> server.index;
		}
		if (line == "}") current_block = "";
	}
	return true;
}

const ServerConfig& ConfigParser::getServerConfig() const 
{ 
	return server; 
}