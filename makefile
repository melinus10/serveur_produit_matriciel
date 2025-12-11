CC = gcc

CFLAGS = -std=c2x -D_XOPEN_SOURCE -Wpedantic -Wall -Wextra -Wconversion -Werror -fstack-protector-all -fpie -pie -O2 -D_FORTIFY_SOURCE=2 -MMD

CLIENT_TARGET = client
SERVER_TARGET = serveur

CLIENT_SRCS = client.c
SERVER_SRCS = serveur.c

CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_DEPS = $(CLIENT_OBJS:.o=.d)
SERVER_DEPS = $(SERVER_OBJS:.o=.d)

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(CLIENT_DEPS) $(SERVER_DEPS)

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) $(CLIENT_OBJS) $(SERVER_OBJS) $(CLIENT_DEPS) $(SERVER_DEPS)

distclean: clean
	rm -f *~

.PHONY: all clean distclean