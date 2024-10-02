CC = gcc
CFLAGS = -o -Wall -g
SRC = ./src/
BIN = ./bin/

all: TriServer

TriServer: TriClient
	@echo compiling server
	@gcc -o ./bin/TriServer ./src/TriServer.c ./src/ipclib.c ./src/printable.c -Wall -g -pthread

TriClient: TriBot
	@echo compiling client
	@gcc -o ./bin/TriClient ./src/TriClient.c ./src/ipclib.c ./src/printable.c -Wall -g -pthread


TriBot: bin
	@echo compiling bot
	@gcc -o ./bin/TriBot ./src/TriBot.c ./src/ipclib.c ./src/printable.c -Wall -g -pthread

bin:
	@mkdir -p $@