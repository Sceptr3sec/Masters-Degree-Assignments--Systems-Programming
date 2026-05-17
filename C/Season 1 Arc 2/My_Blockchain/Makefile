CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99
TARGET = my_blockchain
OBJS = main.o blockchain.o commands.o backup.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c blockchain.h
	$(CC) $(CFLAGS) -c main.c

blockchain.o: blockchain.c blockchain.h
	$(CC) $(CFLAGS) -c blockchain.c

commands.o: commands.c blockchain.h
	$(CC) $(CFLAGS) -c commands.c

backup.o: backup.c blockchain.h
	$(CC) $(CFLAGS) -c backup.c

clean:
	rm -f $(OBJS) $(TARGET) .blockchain.bin .blockchain.txt

fclean: clean

re: fclean all

test: $(TARGET)
	rm -f .blockchain.bin .blockchain.txt
	@echo "Ready for testing - backup files cleaned"

.PHONY: all clean fclean re test