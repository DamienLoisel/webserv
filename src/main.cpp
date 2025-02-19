/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/25 17:18:12 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/19 15:08:05 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "ConfigParser.hpp"
#include "HTTPResponse.hpp"

void	display(char *conf)
{
	ConfigParser parser;
	if (parser.parse(conf)) {
		const ServerConfig& config = parser.getServerConfig();
		
		std::cout << "Server Configuration Details:" << std::endl;
		std::cout << "-----------------------------" << std::endl;
		std::cout << "Listen Port: " << config.listen_port << std::endl;
		std::cout << "Host: " << config.host << std::endl;
		std::cout << "Server Name: " << config.server_name << std::endl;
		std::cout << "Error Page: " << config.error_page << std::endl;
		std::cout << "Client Max Body Size: " << config.client_max_body_size << " bytes" << std::endl;
		std::cout << "Root Directory: " << config.root << std::endl;
		std::cout << "Default Index: " << config.index << std::endl;
		
		std::cout << "\nLocation Configurations:" << std::endl;
		std::cout << "----------------------" << std::endl;
		
		for (std::map<std::string, LocationConfig>::const_iterator it = config.locations.begin(); 
			 it != config.locations.end(); ++it) {
			std::cout << "Location Path: " << it->first << std::endl;
			const LocationConfig& loc = it->second;
			
			std::cout << "  Root: " << loc.root << std::endl;
			std::cout << "  Autoindex: " << (loc.autoindex ? "On" : "Off") << std::endl;
			
			std::cout << "  Allowed Methods: ";
			for (std::vector<std::string>::const_iterator mit = loc.allowed_methods.begin(); 
				 mit != loc.allowed_methods.end(); ++mit) {
				std::cout << *mit << " ";
			}
			std::cout << std::endl;
			
			std::cout << "  Index: " << loc.index << std::endl;
			std::cout << "  Return Path: " << loc.return_path << std::endl;
			std::cout << "  Alias: " << loc.alias << std::endl;
			
			std::cout << "  CGI Path: " << loc.cgi_path << std::endl;
			
			std::cout << "  CGI Extensions: ";
			for (std::vector<std::string>::const_iterator eit = loc.cgi_ext.begin(); 
				 eit != loc.cgi_ext.end(); ++eit) {
				std::cout << *eit << " ";
			}
			std::cout << std::endl << std::endl;
		}
	}
}

int main(int argc, char **argv)
{
    (void)argv;
    if (!parse(argc, argv))
        return (1);
    display(argv[1]);
	ConfigParser parser;
	if (parser.parse(argv[1])) {
	    static const ServerConfig config = parser.getServerConfig();
	    HTTPResponse::setConfig(&config);
	    socket(config);
	}
    return (0);
}