/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigTypes.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/17 03:41:00 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/17 03:41:00 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_TYPES_HPP
# define CONFIG_TYPES_HPP

# include <string>
# include <vector>
# include <map>

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

#endif
