CC      := cc
CFLAGS  := -Wall -Wextra -Werror -std=c99 -O2

NAME    := my_tar
SRC     := my_tar.c
OBJ     := $(SRC:.c=.o)
HDR     := my_tar.h

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ) $(HDR)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all