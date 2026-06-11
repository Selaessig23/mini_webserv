#basics to compile the code
NAME = miniserv
CC = cc
CFLAGS = -Wall -Werror -Wextra 

#arguments to test:
ARGS = "8080"

#styling
RESET =			\033[0m
BOLD =			\033[1m
GREEN =			\033[32m
YELLOW =		\033[33m
RED :=			\033[91m

#sources
SRCS = main.c \
       mini_webserv.c \
       mini_webserv_helpers.c

OBJS = $(SRCS:%.c=obj/%.o)

all: $(NAME)

#to create a program:
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo -- prog created, try it by using ./bsq

obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf obj
	@echo -- Deleting All .o

fclean: clean
	@rm -f $(NAME)
	@echo -- Deleting executables

re: fclean all

run: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) $(ARGS)

debug: CFLAGS += -DDEBUG -g
debug: fclean run 
	@echo "$(BOLD)$(YELLOW)Debug build complete.$(RESET)"
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes $(NAME) $(ARGS)

.PHONY: all run debug clean fclean re
