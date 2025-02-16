# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/01/25 17:18:15 by dloisel           #+#    #+#              #
#    Updated: 2025/02/17 00:31:01 by dmathis          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

GRAY = \033[30m
RED = \033[31m
GREEN = \033[32m
YELLOW = \033[33m
BLUE = \033[34m
RESET = \033[0m

NAME = webserv

CC = c++
CFLAGS = -Wall -Werror -Wextra -std=c++98

SRC = main.cpp webserv.cpp parse.cpp socket.cpp ConfigParser.cpp HTTPRequest.cpp HTTPResponse.cpp

RM = rm -f
OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(OBJ) $(CFLAGS) -o $(NAME)
	@echo "$(GREEN)webserv compiled!$(RESET)"

%.o: %.cpp
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "$(RED)make clean...$(RESET)"
	@$(RM) $(OBJ)

fclean: clean
	@echo "$(RED)make fclean...$(RESET)"
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
