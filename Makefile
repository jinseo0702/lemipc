CC = gcc
CFLAG = -g -Wall -Wextra -Werror -Iinclude -Iprintf -Ilibft
RM = rm -rf

SRC = src/system.c \
	src/logic.c \
	src/main.c \
	src/player.c \
	src/utils.c \
	src/game_algorithm.c

TESTSRC = src/testSystem.c \
	src/system.c \
	src/logic.c \
	src/player.c \
	src/utils.c \
	src/game_algorithm.c

TESTOBJ = $(TESTSRC:.c=.o)

OBJS = $(SRC:.c=.o)

NAME = lemipc
TESTSYSTEM = testSystem

HEADER = include/data.h \
	include/system.h \
	include/logic.h

LIBFT_A = libft/libft.a
PRINTF_A = printf/libftprintf.a

all : $(NAME)

$(NAME): $(OBJS) $(LIBFT_A) $(PRINTF_A)
	@$(CC) -o $@ $^


$(TESTSYSTEM): $(TESTOBJ) $(LIBFT_A) $(PRINTF_A)
	@$(CC) -o $@ $^

$(LIBFT_A):
	@make -C libft

$(PRINTF_A):
	@make -C printf

%.o : %.c $(HEADER)
	@$(CC) $(CFLAG) -c $< -o $@


test : $(TESTSYSTEM)

clean :
	@make clean -C libft/
	@make clean -C printf/
	@$(RM) $(OBJS) $(TESTOBJ)

fclean :
	@make fclean -C libft/
	@make fclean -C printf/
	@make clean
	@$(RM) $(NAME) $(TESTSYSTEM)

re : 
	@make fclean
	@make all

.PHONY: all clean fclean re test