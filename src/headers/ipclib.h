#ifndef SHMF_H_
#define SHMF_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

#define FILENO "/"
#define IPC_ERR (-1)

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

typedef struct game_t {
  pid_t serverPid;  // server's pid
  int table[3][3];  // game table
  pid_t pidP1;      // player 1 pid
  pid_t pidP2;      // player 2 pid
  char symbol1;     // symbol of player 1
  char symbol2;     // symbol of player 2
  int victory;      // 0 = draw, 1 = p1 win, 2 = p2 win
  int players;      // number of player connected
  int shmid;        // shmid
  int timer;
  int turn;  // 1 = p1, 2 = p2
  int bot;
} Game;

int create_shm();
Game *attach_shm();
void semOp(int semid, unsigned short semnum, unsigned short op);
void semCtl(int semid, int semnum, int cmd, int value, char *semname);

#endif