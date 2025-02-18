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

# include <string>
# include <sstream>
# include "ConfigTypes.hpp"

class ConfigParser 
{
    public:
        bool parse(const std::string& filename);
        const ServerConfig& getServerConfig() const;
    
    private:
        ServerConfig server;
        void trim(std::string& str);
        std::vector<std::string> split(const std::string& s, char delimiter);
        void parseLocationBlock(std::istringstream& block_stream, LocationConfig& loc);
};

#endif