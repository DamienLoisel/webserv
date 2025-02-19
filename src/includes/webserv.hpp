/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/25 17:18:18 by dloisel           #+#    #+#             */
/*   Updated: 2025/02/19 02:20:53 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <iomanip>
# include <stdexcept>
# include <cstring>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <stdlib.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <signal.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netdb.h>
# include <poll.h>
# include <sys/epoll.h>
# include <sys/select.h>
# include <dirent.h>
# include <sstream>
# include <map>
# include <vector>
# include <fstream>
# include "ConfigTypes.hpp"

bool parse(int argc, char **argv);
bool socket(const ServerConfig& config);

#endif