CC := gcc
CFLAGS = -lpthread
files := server client


all: $(files)

$(files): % : %.c
	$(CC) -o ./build/$@ $^ $(CFLAGS)

# server: server.c
# 	$(CC) -o $@ $^ $(CFLAGS)

# client: client.c
# 	$(CC) -o $@ $^ $(CFLAGS)