CC=gcc
SRC=./src
BIN=./bin

gatling:
	mkdir -p $(BIN)
	$(CC) -Wall -o $(BIN)/gatling \
		$(SRC)/gatling.c \
		$(SRC)/hashmap.c \
		$(SRC)/subscriptions.c \
		$(SRC)/protocol.c \
		$(SRC)/chan.c \
		$(SRC)/queue.c

clean:
	rm -rf $(BIN)
