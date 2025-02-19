# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dmathis <dmathis@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/01/25 17:18:15 by dloisel           #+#    #+#              #
#    Updated: 2025/02/19 02:28:46 by dmathis          ###   ########.fr        #
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
CFLAGS = -Wall -Werror -Wextra -std=c++98 -I src/includes

SRC_DIR = src
SRC_FILES = main.cpp parse.cpp socket.cpp ConfigParser.cpp HTTPRequest.cpp HTTPResponse.cpp CGIHandler.cpp
SRC = $(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

RM = rm -rf

all: $(NAME)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	@$(CC) $(OBJ) $(CFLAGS) -o $(NAME)
	@echo "$(GREEN)webserv compiled!$(RESET)"

clean:
	@echo "$(RED)make clean...$(RESET)"
	@$(RM) $(OBJ_DIR)

fclean: clean
	@echo "$(RED)make fclean...$(RESET)"
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
