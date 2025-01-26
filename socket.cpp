/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 10:35:54 by dloisel           #+#    #+#             */
/*   Updated: 2025/01/26 11:11:07 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

bool socket(void)
{
	int         socketfd;
	sockaddr_in sockaddr;
	int         connection;
	char 		buffer[100];

	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd == -1)
	{
		std::cout << "Failed to create socket." << std::endl;
		return (false);
	}
	
	int opt = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) //Cette partie n'est pas nécessaire si on attend suffisament longtemps avant de relancer le programme.
	{
		std::cout << "Failed to set socket options." << std::endl;
		close(socketfd);
		return false;
	}

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(9999);
	if (bind(socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
	{
		std::cout << "Failed to bind to port 9999." << std::endl;
		return (false);
	}
	
	if (listen(socketfd, 10) < 0)
	{
		std::cout << "Failed to listen on socket." << std::endl;
		return (false);
	}
	
	socklen_t addrlen = sizeof(sockaddr);
	connection = accept(socketfd, (struct sockaddr*)&sockaddr, (socklen_t *)&addrlen);
	if (connection < 0)
	{
		std::cout << "Failed to grab connection." << std::endl;
		return (false);
	}

	ssize_t bytesRead = read(connection, buffer, 100);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		std::cout << "The message was: " << buffer << std::endl;
	}

	std::string response = "Good talking to you\n";
	send(connection, response.c_str(), response.size(), 0);

	close(connection);
  	close(socketfd);
	return (true);
}