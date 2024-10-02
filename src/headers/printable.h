#ifndef PRINTABLE_H_
#define PRINTABLE_H_
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "ipclib.h"

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;38;5;6m"
#define ORANGE "\033[1;38;5;214m"
#define GREEEEEN "\033[1;38;5;46m"
#define LIGHT_YELLOW "\033[1;38;5;227m"
#define SAVE_CURSOR_POSITION "\033[s"
#define RESTORE_CURSOR_POSITION "\033[u"
#define CLEAR_LINE "\033[2K"
#define MOVE_UP "\033M"

// void colorText(int color, int style, int type);
void moveUpLine(int lines);
void moveDownLine(int lines);
void clearScreen();
void clearLine();
void printVictory(int winner, int player);
void printTable(int player, Game *game);
void *printTimer(void *args);
void *waitingForPlayers(void *args);

#endif