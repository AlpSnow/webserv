NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 -O0

SRC = main.cpp \
config_loader/Config.cpp \
config_loader/ConfigParser.cpp \
srcs/Server.cpp \
srcs/Client.cpp \
srcs/Response.cpp \
srcs/CgiHandler.cpp \
srcs/Request.cpp \
srcs/RequestCookies.cpp \
srcs/SessionManager.cpp

OBJ = $(SRC:.cpp=.o)

RED		:= \033[0;31m
GREEN	:= \033[0;32m
YELLOW	:= \033[0;33m
BLUE	:= \033[0;34m
NC		:= \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX)	$(CXXFLAGS)	$(OBJ) -o $(NAME)
	@echo "$(GREEN)FINISHED COMPILING $(NAME)!$(NC)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	@echo "$(YELLOW)OBJECTS FILES DELETED!$(NC)"

fclean: clean
	rm -f $(NAME)
	@echo "$(RED)ALL FILES CLEAN!$(NC)"

re: fclean all

.PHONY: all clean fclean re