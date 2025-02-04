/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 13:47:21 by dmathis           #+#    #+#             */
/*   Updated: 2025/01/26 15:44:32 by dmathis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "webserv.hpp"
# include "HTTPRequest.hpp"

class HTTPResponse {
private:
    std::string response;
    
public:
    HTTPResponse(int status, std::string content_type, std::string body);
    std::string toString();

    static void handle_request(HTTPRequest& req, int client_fd);
};

#endif