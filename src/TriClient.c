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

void turnHandle();
void imWaiting();
void botTurnHandle(int segnale);

int semid;               // semaphores id
Game *game;              // game's shared memory
int player;              // indicates whether we are player 1 or 2
char symbol;             // our symbol
char name[32];           // our name
int bot = 0;             // is the bot active?
pthread_t threads[3];    // 0 printTimer, 1 move, 2 notYourTurn
sigset_t mySet, defSet;  // set of signals

/// @brief received end of turn signal, wait for another signal to start the new
/// round
/// @param segnale
void imWaiting(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  if (game->turn != player && bot == 0) {
    pthread_cancel(threads[2]);  // cancel notYourTurn thread
  } else if (game->turn == player) {
    pthread_cancel(threads[0]);  // cancel timer thread
    pthread_cancel(threads[1]);  // cancel move thread
  }
  if (bot == 0) {
    printf("\033[?25h");  // show cursor
    fflush(stdout);
  }
  signal(SIGUSR1, turnHandle);  // next SIGUSR1 = next turn
  if (game->pidP1 == getpid()) {
    semOp(semid, 1, 1);  // unlock server
  } else {
    semOp(semid, 2, 1);  // unlock server
  }
  sigprocmask(SIG_SETMASK, &defSet, NULL);
}

/// @brief thread function to handle player's move
/// @return
void *move() {
  int x, y;            // move's coordinates
  int is_integer;      // how many integers did we read?
  int garbage;         // is this garbage?
  int isCellFree = 0;  // is this cell free?
  printf("\n");        // make space
  printf(MOVE_UP);     // go back
  fflush(stdout);
  is_integer = scanf("%d %d", &x, &y);  // ask for input
  if (x >= 0 && x <= 2 && y >= 0 && y <= 2) {
    if (game->table[x][y] == 0) {
      isCellFree = 1;
    }
  }
  while (!is_integer || isCellFree == 0) {
    while ((garbage = getchar()) != EOF && garbage != '\n');
    printf("%s\r%s", MOVE_UP, CLEAR_LINE);
    fflush(stdout);
    is_integer = scanf("%d %d", &x, &y);  // ask for input

    if (x >= 0 && x <= 2 && y >= 0 && y <= 2) {
      if (game->table[x][y] == 0) {
        isCellFree = 1;
      }
    }
  }

  game->table[x][y] = player;      // place your move
  kill(game->serverPid, SIGUSR2);  // tell the server we are done
  pthread_exit(NULL);
}

void *notYourTurn() {
  char garbage;
  printf("\n%s%s%s%s%s\n%s%s%c%s\n", ORANGE, "Non è il tuo turno ", name, ", attendi", RESET,
         "Il tuo simbolo è: ", GREEEEEN, symbol, RESET);
  printTable(player, game);
  printf("\n\n\n\n\033M\033[?25l");  // align table and hide cursor
  fflush(stdout);
  while ((garbage = getchar()) != -1) {  // eat garbage input
    if (garbage == '\n') {
      moveUpLine(1);
    }
    clearLine();
  };
  pthread_exit(NULL);
}

void *turn(void *) {
  clearScreen();
  // tell the server we are ready

  if (game->pidP1 == getpid()) {
    semOp(semid, 1, 1);  // unlock server
  } else {
    semOp(semid, 2, 1);  // unlock server
  }
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  semOp(semid, 0, -1);  // wait for all the clients to be ready
  sigprocmask(SIG_SETMASK, &defSet, NULL);

  if (game->turn == player) {
    printf("\n%sÈ il tuo turno %s!%s\nIl tuo simbolo è: %s%c%s\n", CYAN, name, RESET, GREEEEEN, symbol, RESET);
    printTable(player, game);
    printf("\nFai la tua mossa: x y (intervallo ammesso 0 - 2)\n\n");
    fflush(stdout);

    if (pthread_create(&threads[1], NULL, move, NULL) != 0) {
      printf("ERRORE: thread create move");
    }
    if (pthread_create(&threads[0], NULL, printTimer, game) != 0) {
      printf("ERRORE: thread create printTimer");
    }

  } else if (game->turn != player && bot == 0) {
    if (pthread_create(&threads[2], NULL, notYourTurn, NULL) != 0) {
      printf("ERRORE: thread create notYourTurn");
    }
  }

  pthread_exit(NULL);
}

/// @brief upon reciving of signal 10 start the game's turn
/// @param segnale
void turnHandle(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  signal(SIGUSR1, imWaiting);              // next SIGUSR1 = turn end
  pthread_t thread[1];
  pthread_create(&thread[0], NULL, turn, NULL);
  sigprocmask(SIG_SETMASK, &defSet, NULL);  // we unblock all signals
}

void endGame(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  clearScreen();
  printTable(player, game);
  printVictory(game->victory, player);
  fflush(stdout);
  signal(SIGINT, SIG_DFL);  // reset SIGINT to it's default behaviour
  semOp(semid, 0, 1);
  semOp(semid, 4, 1);  // unlock server
  sigprocmask(SIG_SETMASK, &defSet, NULL);
  kill(getpid(), SIGKILL);  // terminate process
}

/// @brief alert server that this client is leaving the game
/// @param segnale
void exitHandler(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  kill(game->serverPid, SIGUSR2);          // tell server to check who's left
  if (player == 1) {
    game->pidP1 = 0;     // this means player 1 left
    semOp(semid, 1, 1);  // unlock server
    semOp(semid, 2, 1);  // unlcok server
  } else {
    game->pidP2 = 0;     // this means player 2 left
    semOp(semid, 2, 1);  // unlcok server
    semOp(semid, 1, 1);  // unlock server
  }

  printf("\033[?25h");  // reset cursor to visible
  fflush(stdout);
  kill(getpid(), SIGKILL);                  // kill process
  sigprocmask(SIG_SETMASK, &defSet, NULL);  // we unblock all signals
}

void sigIntHandler(int segnale) {
  signal(SIGINT, exitHandler);
  printf("\033[2D\033[0K"); // delete ctrl+c character
 
  for (int i = 10; i > 0; i--) {
    printf(SAVE_CURSOR_POSITION);
    printf("\n");
    clearLine();
    printf(
      "\033[1;33mSei sicuro di voler uscire? \033[0m");
    if (i < 4 && i != 1) {
      printf("%s%s%d%s", ORANGE, "Tempo rimanente: ", i, " secondi");
    } else if (i == 1) {
      printf("%s%s%d%s", RED, "Tempo rimanente: ", i, " secondo");
    } else {
      printf("%s%s%d%s", YELLOW, "Tempo rimanente: ", i, " secondi");
    }
    printf(RESTORE_CURSOR_POSITION);
    fflush(stdout);
    sleep(1);
  }
  printf(SAVE_CURSOR_POSITION);
  printf("\033[1E");
  clearLine();
  //printf("%s%s%s", GREEN, "Uscita annullata", RESET);
  printf(RESTORE_CURSOR_POSITION);
  fflush(stdout);
  signal(SIGINT, sigIntHandler);
}

void serverTermination(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  printf(
      "\n\r\033[?25h%s%sIl server è stato terminato, partita "
      "abortita%s\n",
      CLEAR_LINE, RED, RESET);
  fflush(stdout);
  signal(SIGTERM, SIG_DFL);
  kill(getpid(), SIGTERM);
  sigprocmask(SIG_SETMASK, &defSet, NULL);  // we unblock all signals
}

int main(int argc, char const *argv[]) {
  // Argument check
  if (argc < 2) {  // not enough arguments
    clearLine();
    printf("%s%s%s\n", RED, "Errore: argomenti insufficienti", RESET);
    printf("%s%s%s\n", RED, "Uso corretto: ./TriClient nome_utente o ./TriClient nome_utente \\*", RESET);
    fflush(stdout);
    exit(1);
  } else {                                         // 2 or more arguments
    strncpy(name, argv[1], 31);                    // second argument is player name
    name[31] = '\0';                               // add string terminator in case argv[1] > 31
    if (argc == 3 && strcmp("*", argv[2]) == 0) {  // 3 arguments and 3rd one is bot call
      bot = 1;                                     // bot enabled
    } else if (argc != 2) {                        // more than 2 arguments && not bot
      printf("%s%s%s\n", RED, "Errore: troppi argomenti", RESET);
      printf("%s%s%s\n", RED, "Uso corretto: ./TriClient nome_utente o ./TriClient nome_utente \\*", RESET);
      fflush(stdout);
      exit(1);
    }
  }

  // Check whether a server is running
  int pipeFd[2];
  char buff[64] = {0};
  if (pipe(pipeFd) < 0) {
    perror("pipe failed");
    exit(1);
  }
  switch (fork()) {
    case -1:
      perror("fork failed");
      exit(1);
      break;

    case 0:
      close(pipeFd[0]);                             // close pipe read
      close(1);                                     // close stdout
      dup(pipeFd[1]);                               // copy piper write to stdout's default fd
      close(pipeFd[1]);                             // close old pipe write
      execlp("pgrep", "pgrep", "TriServer", NULL);  // get the list of pids of processes called TriServer
      exit(0);
      break;

    default:
      close(pipeFd[1]);  // close pipe write
      wait(NULL);        // wait for child
      // read data drom child
      if (read(pipeFd[0], &buff, sizeof(buff)) == -1) {
        perror("read failed");
        exit(1);
      }
      if (atoi(buff) == 0) {  // there is not an active server
        printf("%sErrore: nessun server attivo%s\n", RED, RESET);
        exit(1);
      }
      break;
  }

  // IPC Section
  key_t key;
  if ((key = ftok(FILENO, 0)) == -1) {
    perror("ftok");
    exit(1);
  }
  if ((semid = semget(key, 5, IPC_CREAT | 0600)) == -1) {
    perror("semget");
    exit(1);
  }

  // we fill the set with all signals that will be blocked
  sigfillset(&mySet);

  signal(SIGTERM, serverTermination);  // handle server termination signal
  signal(SIGINT, sigIntHandler);         // manage exit interrupt signal
  signal(SIGQUIT, exitHandler);        //
  signal(SIGHUP, exitHandler);         // closing terminal

  game = attach_shm();  // connect to the game

  sigprocmask(SIG_SETMASK, &mySet, NULL);

  semOp(semid, 4, -1);  // lock the semaphore so only one client can start the joining process

  if (game->players > 0 && bot == 1) {  // there is no place for the bot to play
    semOp(semid, 4, 1);                 // unlock semaphore so other players can join
    shmdt(&game->shmid);                // detach from shm
    printf("%s%s%s\n", RED, "Errore BOT: troppi giocatori connessi", RESET);
    fflush(stdout);
    exit(1);
  } else if (game->players > 1) {  // 2 players alrady connected
    semOp(semid, 4, 1);            // unlock semaphore even if server if full
    shmdt(&game->shmid);           // detach from shm
    printf("%sCi sono già 2 giocatori collegati\033[0m\n", RED);
    fflush(stdout);
    exit(1);
  }
  // we know the server is not full and the counter is 1 player behind because we didn't tell the server we joined yet
  if (game->players == 0) {
    game->pidP1 = getpid();  // set player 1 pid to our pid
    player = 1;              // we are player 1
    symbol = game->symbol1;  // get our symbol
  } else {
    game->pidP2 = getpid();  // set player 2 pid to our pid
    player = 2;              // we are player 2
    symbol = game->symbol2;  // get our symbol
  }
  if (bot == 1) {
    game->bot = 1;  // to inform the client we request bot start
  }

  signal(SIGUSR1, turnHandle);  // signal to start the turn
  signal(SIGUSR2, endGame);     // signal to know the winner when the game is done

  kill(game->serverPid, SIGUSR1);  // tell the server we are joining, and reset semaphore number 1
  semOp(semid, 2, -1);             // wait until the server has handled the signal
  semOp(semid, 4, 1);              // release the semaphore so other players can join

  sigprocmask(SIG_SETMASK, &defSet, NULL);

  int firstRound = 1;  // we need to know if it's the first round of play
  while (1) {
    if (firstRound) {
      firstRound = 0;
      if (game->pidP1 == getpid()) {
        semOp(semid, 0, 1);  // unlock the server
      } else {
        semOp(semid, 3, 1);  // unlock the server
      }
    }
    sleep(1);
  }
  return 0;
}
