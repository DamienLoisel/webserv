/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 17:27:06 by dloisel           #+#    #+#             */
/*   Updated: 2025/01/26 17:28:50 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include "webserv.hpp"

class ConfigParser 
{
    public:
        struct LocationConfig {
            std::string root;
            bool autoindex;
            std::vector<std::string> allowed_methods;
            std::string index;
            std::string return_path;
            std::string alias;
            std::string cgi_path;
            std::vector<std::string> cgi_ext;
        };
    
        struct ServerConfig {
            int listen_port;
            std::string host;
            std::string server_name;
            std::string error_page;
            size_t client_max_body_size;
            std::string root;
            std::string index;
            std::map<std::string, LocationConfig> locations;
        };
    
        bool parse(const std::string& filename);
        const ServerConfig& getServerConfig() const;
    
    private:
        ServerConfig server;
        void trim(std::string& str);
        std::vector<std::string> split(const std::string& s, char delimiter);
        void parseLocationBlock(std::istringstream& block_stream, LocationConfig& loc);
};

#endif