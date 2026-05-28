NAME = libasm.a

NASM = nasm
NASMFLAGS = -f elf64

AR = ar
ARFLAGS = rcs

SRCS = \
	my_strlen.S \
	my_strchr.S \
	my_memset.S \
	my_memcpy.S \
	my_strcmp.S \
	my_memmove.S \
	my_strncmp.S \
	my_strcasecmp.S \
	my_index.S \
	my_read.S \
	my_write.S

OBJS = $(SRCS:.S=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(AR) $(ARFLAGS) $(NAME) $(OBJS)

%.o: %.S
	$(NASM) $(NASMFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re