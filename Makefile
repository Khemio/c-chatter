CC := gcc
CFLAGS = -lpthread -Wall

files := server client
path := utils/
utils := $(path)queue.c $(path)list.c $(path)servUtils.c

all: $(files)

# $(files): % : %.c
# 	$(CC) -o ./build/$@ $^ $(CFLAGS)

# server: server.c $(path)queue.c utils/list.c utils/servUtils.c
# 	$(CC) -o ./build/$@ $^ $(CFLAGS)
server: server.c $(utils)
	$(CC) -o ./build/$@ $^ $(CFLAGS)

client: client.c
	$(CC) -o ./build/$@ $^ $(CFLAGS)

