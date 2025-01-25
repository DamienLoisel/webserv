/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/25 17:18:18 by dloisel           #+#    #+#             */
/*   Updated: 2025/01/25 17:53:02 by dloisel          ###   ########.fr       */
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
//# include <sys/event.h> Erreur sur cette bibliothèque

bool parse(int argc, char **argv);

#endif