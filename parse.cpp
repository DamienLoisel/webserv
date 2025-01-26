/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dloisel <dloisel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/25 17:26:02 by dloisel           #+#    #+#             */
/*   Updated: 2025/01/26 10:38:02 by dloisel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

bool	endsWithConf(char *str)
{
	size_t	len = std::strlen(str);
	
	if (!str)
		return (false);
	if (len < 5)
		return (false);
	return (std::strcmp(str + len - 5, ".conf") == 0);
}

bool	parse(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cout << "The program must be called like this :" << std::endl;
		std::cout << "./webserv [configuration file]" << std::endl;
		return (false);
	}
	if (!endsWithConf(argv[1]))
	{
		std::cout << "The configuration file must be a .conf file." << std::endl;
		return (false);
	}
	return (true);
}