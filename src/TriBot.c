#include <asm-generic/signal-defs.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "headers/ipclib.h"
#include "headers/printable.h"

void botTurnHandle(int segnale);

int semid;
int timeRemainig;  // [n] time until the turn ends, [0] turn is over, [-1]
                   // infinite time
Game *game;
int player;
char symbol;
int bot = 1;
sigset_t mySet, defSet;

void serverTermination(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  printf(
      "\n\r\x1b[?25h\x1b[2K\x1b[1;31mIl server è stato terminato, partita "
      "abortita%s\n",
      RESET);
  fflush(stdout);
  signal(SIGTERM, SIG_DFL);
  kill(getpid(), SIGTERM);
  sigprocmask(SIG_SETMASK, &defSet, NULL);
}

void exitHandler(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  if (player == 1) {
    game->pidP1 = 0;
    semOp(semid, 1, 1);
    semOp(semid, 2, 1);
  } else {
    game->pidP2 = 0;
    semOp(semid, 2, 1);
    semOp(semid, 1, 1);
  }

  kill(game->serverPid, SIGUSR2);
  kill(getpid(), SIGKILL);
  sigprocmask(SIG_SETMASK, &defSet, NULL);
}

void botWaiting(int segnale) {
  if (segnale == 10) {
    signal(10, botTurnHandle);
  }

  if (game->pidP1 == getpid()) {
    semOp(semid, 1, 1);
  } else {
    semOp(semid, 2, 1);
  }
}

void *botTurn(void *arg) {
  if (game->victory == -1) {
    // tell the server we joined

    if (game->pidP1 == getpid())
      semOp(semid, 1, 1);
    else
      semOp(semid, 2, 1);

    sigprocmask(SIG_SETMASK, &mySet, NULL);
    semOp(semid, 0, -1);  // wait for all the clients to be ready
    sigprocmask(SIG_SETMASK, &defSet, NULL);

    if (game->turn == player) {
      srand(time(NULL));
      int x, y;
      do {
        x = rand() % 3;
        y = rand() % 3;
      } while (game->table[x][y] != 0);
      game->table[x][y] = player;
      kill(game->serverPid, SIGUSR2);
    }
  }
  pthread_exit(NULL);
}

void botTurnHandle(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  write(1, "handler", 8);
  signal(10, botWaiting);
  pthread_t thread[1];
  if (pthread_create(&thread[0], NULL, botTurn, NULL) != 0) {
    printf("Errore creazione thread");
    fflush(stdout);
  };
  sigprocmask(SIG_SETMASK, &defSet, NULL);
}

void endGame(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  signal(2, SIG_DFL);
  semOp(semid, 0, 1);
  semOp(semid, 4, 1);
  sigprocmask(SIG_SETMASK, &defSet, NULL);
  kill(getpid(), 2);
}

int main(int argc, char const *argv[]) {
  key_t key;

  if ((key = ftok(FILENO, 0)) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((semid = semget(key, 5, IPC_CREAT | 0600)) == -1) {
    perror("semget");
    exit(1);
  }

  sigfillset(&mySet);

  signal(SIGTERM, serverTermination);
  signal(SIGINT, SIG_IGN);  // ignore sigint to avoid problems with server
  signal(SIGQUIT, exitHandler);
  signal(SIGHUP, exitHandler);  // closing terminal

  game = attach_shm();  // connect to the game

  sigprocmask(SIG_SETMASK, &mySet, NULL);
  semOp(semid, 4, -1);  // lock the semaphore join 1 so one client can start the joining process
  sigprocmask(SIG_SETMASK, &defSet, NULL);

  if (game->players != 1) {  // if there are already 2 players
    semOp(semid, 4, 1);
    printf("\x1b[1;31mbot: errore giocatori\x1b[0m\n");
    fflush(stdout);
    shmdt(&game->shmid);  // detach from shm
    exit(1);
  }

  close(1);

  // we know the server is not full and the counter is 1 player behind
  if (game->players == 0) {
    game->pidP1 = getpid();  // because we didn't tell the server we joined yet
    player = 1;
    symbol = game->symbol1;
  } else {
    game->pidP2 = getpid();
    player = 2;
    symbol = game->symbol2;
  }
  signal(SIGUSR1, botTurnHandle);
  signal(SIGUSR2, endGame);

  kill(game->serverPid, 10);  // tell the server we are joining, and reset semaphore number 1
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  semOp(semid, 2, -1);  // wait until the server has handled the signal
  sigprocmask(SIG_SETMASK, &defSet, NULL);
  semOp(semid, 4, 1);

  printf("pid p1: %d\npid p2: %d\n\n\n\n", game->pidP1, game->pidP2);
  fflush(stdout);
  semOp(semid, 1, 1);  // release the semaphore number 1
  int firstRound = 1;
  while (1) {
    if (firstRound) {
      firstRound = 0;
      if (game->pidP1 == getpid()) {
        semOp(semid, 0, 1);
        printf("SONO PLAYER 1");
        fflush(stdout);
      } else {
        semOp(semid, 3, 1);
        printf("SONO PLAYER 2");
        fflush(stdout);
      }
    }

    sleep(1);
  }

  return 0;
}
