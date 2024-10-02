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
#include <unistd.h>

#include "headers/ipclib.h"
#include "headers/printable.h"

void *waitingForPlayers(void *);
void exitHandler(int);
void killClients();

int shmid;                // shared memory id
int semid;                // semaphores id
Game *game;               // Game table pointer
int timer;                // turn timer
bool exit_ = false;       // player has made a move flag
pthread_t waitThread[1];  // waiting for plaers thread
char path[256];           // relative path of execution
sigset_t mySet, defSet;   // set of signals

/// @brief checks if someone has won the game and returns the result
/// @return -1 = game is not over yet, 0 = draw, 1 = player1 won, 2 = player2
/// won
int checkResult() {
  for (int i = 0; i < 3; i++) {
    if (game->table[i][0] != 0) {
      // check if someone won in a row
      if (game->table[i][0] == game->table[i][1] && game->table[i][0] == game->table[i][2]) {
        return game->table[i][0];
      }
    }
    if (game->table[0][i] != 0) {
      // check if someone won in a column
      if (game->table[0][i] == game->table[1][i] && game->table[0][i] == game->table[2][i]) {
        return game->table[0][i];
      }
    }
  }

  if (game->table[1][1] != 0) {
    // check the main diagonal
    if (game->table[0][0] == game->table[1][1] && game->table[0][0] == game->table[2][2]) {
      return game->table[1][1];
    }
    // check the anti-diagonal
    else if (game->table[0][2] == game->table[1][1] && game->table[0][2] == game->table[2][0]) {
      return game->table[1][1];
    }
  }

  int found = 0;
  for (int i = 0; i < 3 && !found; i++) {
    for (int j = 0; j < 3; j++) {
      if (game->table[i][j] == 0) {
        found = 1;
        break;
      }
    }
  }
  if (!found)  // no spaces for the next move, it's a draw
    return 0;
  else
    return -1;  // game is still to end yet
}

/// @brief it tells the clients that the game has started and handles rounds
void startGame() {
  if (checkResult() == -1) {
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    semCtl(semid, 0, SETVAL, 0, "START");   // lock semaphore start
    semCtl(semid, 1, SETVAL, 0, "JOIN 1");  // lock semaphore join 1
    semCtl(semid, 2, SETVAL, 0, "JOIN 2");  // lock semaphore join 2

    kill(game->pidP1, 10);  // tell client 1 that the game has started
    kill(game->pidP2, 10);  // tell client 2 that the game has started

    semOp(semid, 1, -1);                   // wait for client to unlock join 1
    semOp(semid, 2, -1);                   // wait for client to unlock join 2
    semctl(semid, 0, SETVAL, 2, "START");  // set start to 2

    sigprocmask(SIG_SETMASK, &defSet, NULL);

    if (game->timer != 0) {
      int timeRem = game->timer;
      while ((timeRem = sleep(timeRem)) != 0 && exit_ == false);
    } else {
      while (!exit_) {
        sleep(1);
      }  // wait for player's signal telling that he has made his move
    }

    exit_ = false;

    exit_ = false;

    if (game->pidP1 == 0 || game->pidP2 == 0) {
      return;
    }
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    semCtl(semid, 1, SETVAL, 0, "END 1");  // lock semaphore end 1
    semCtl(semid, 2, SETVAL, 0, "END 2");  // lock semaphore end 2
    kill(game->pidP1, 10);                 // end turn for client 1
    kill(game->pidP2, 10);                 // end turn for client 2
    semOp(semid, 1, -1);                   // wait for client to unlock end 1
    semOp(semid, 2, -1);                   // wait for client to unlock end 2

    sigprocmask(SIG_SETMASK, &defSet, NULL);
    game->turn = (game->turn == 1) ? (2) : (1);  // change turn

  } else {
    game->victory = checkResult();  // used by players to know who won
    semCtl(semid, 0, SETVAL, 0, "ENDGAME");
    kill(game->pidP1, 12);  // tell the result to client 1
    kill(game->pidP2, 12);  // tell the result to client 2
    sigprocmask(SIG_SETMASK, &mySet, NULL);
    semOp(semid, 0, -1);  // wait for clients
    semOp(semid, 0, -1);  // wait for clients
    sigprocmask(SIG_SETMASK, &defSet, NULL);
    printf("%s%s%s\n", YELLOW, "Partita finita", RESET);
    fflush(stdout);
  }
}

/// @brief handle player count
/// @param segnale
void playerHandle(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  if (game->players < 2) {                 // if it's possible to join
    game->players++;
    semOp(semid, 2, 1);  // unlock requesting client
  }
  sigprocmask(SIG_SETMASK, &defSet, NULL);  // unblock all signals
}

void initializeGame(int timer, char symbolp1, char symbolp2) {
  game->bot = 0;
  game->shmid = shmid;                    // shared memeory id
  game->serverPid = getpid();             // server's pid
  game->victory = -1;                     // no player has won yet
  game->players = 0;                      // set player count to 0
  int tmp[9] = {0};                       // temp array of all 0
  memcpy(game->table, tmp, sizeof(tmp));  // initialize table to 0
  game->symbol1 = symbolp1;               // assign symbol1 to player 1
  game->symbol2 = symbolp2;               // assign symbol2 to player 2
  game->turn = 1;                         // player 1 is the first to play
  game->timer = timer;                    // turn timer
}

void killClients() {
  if (game->pidP1 != 0)          // if player 1 is connected
    kill(game->pidP1, SIGTERM);  // disconnect player 1
  if (game->pidP2 != 0)          // if player 2 is connected
    kill(game->pidP2, SIGTERM);  // disconnect player 2
}

void shutdown(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals

  char msg[] = "\033[2K\n\033[1;46m Server chiuso \033[0m\n";
  write(1, msg, sizeof(msg));
  shmdt(&shmid);  // detach from shared memory
  killClients();
  shmctl(shmid, IPC_RMID, NULL);  // remove shared memory
  semctl(semid, IPC_RMID, 0);     // remove semaphore
  exit(0);                        // exit program

  sigprocmask(SIG_SETMASK, &defSet, NULL);  // we unblock all signals
}

/// @brief thread function, wait 5 seconds, if the user doesn't press CTRL+C
/// in these 5 seconds reset signal 10
/// @return

void *exitFunction() {
  pthread_cancel(waitThread[0]);  // stop waiting for players thread (print)
  clearLine();
  printf(
      "\n\033[1;33mSei sicuro di voler uscire?\033[0m\nPremi "
      "\033[3;1;33mCTRL+C\033[0m "
      "entro 5 secondi "
      "per chiudere il programma\033[0m\n");

  for (int i = 5; i > 0; i--) {
    printf(SAVE_CURSOR_POSITION);
    printf("\r");
    printf(MOVE_UP);
    clearLine();
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
  printf("\r");
  printf(MOVE_UP);
  clearLine();
  printf("%s%s%s\n", GREEN, "Ritorno al programma", RESET);

  signal(SIGINT, exitHandler);  // reset SIGINT to this function

  if (game->players < 2) {
    pthread_create(&waitThread[0], NULL, waitingForPlayers, game);  // restart waiting for players thread
  }
  pthread_exit(NULL);
}

void exitHandler(int segnale) {
  sigprocmask(SIG_SETMASK, &mySet, NULL);  // we block all signals
  signal(SIGINT, shutdown);                // next CTRL+C = shutdown
  pthread_t thread[1];
  pthread_create(&thread[0], NULL, exitFunction, NULL);  // start timer for exit confirmation
  sigprocmask(SIG_SETMASK, &defSet, NULL);               // we block all signals
}

void playerMove() {
  sigprocmask(SIG_SETMASK, &mySet, NULL);
  semOp(semid, 3, -1);
  if (game->victory == -1) {
    if (game->pidP1 == 0) {  // player 1 has disconnected
      semCtl(semid, 4, SETVAL, 0, "wait for client exit");
      game->players--;
      printf("%sIl Giocatore 1 ha lasciato la partita%s\n", RED, RESET);
      printf("%sPartita finita%s\n", YELLOW, RESET);
      fflush(stdout);
      game->victory = 2;             // player 2 has disconnected, player 1 wins
      if (game->pidP2 != 0) {        // player 2 is connected
        kill(game->pidP2, SIGUSR2);  // tell player 2 he is the winner
        semOp(semid, 4, -1);         // wait for client to exit
      }
      shmctl(shmid, IPC_RMID, NULL);  // remove shared memory
      semctl(semid, IPC_RMID, 0);     // remove semaphore
      exit(0);
    } else if (game->pidP2 == 0) {  // player 2 has disconnected
      semCtl(semid, 4, SETVAL, 0, "wait for client exit");
      game->players--;
      printf("%sIl Giocatore 2 ha lasciato la partita%s\n", RED, RESET);
      printf("%sPartita finita%s\n", YELLOW, RESET);
      fflush(stdout);
      game->victory = 1;             // player 2 has disonnected, player 1 wins
      if (game->pidP1 != 0) {        // if player 1 is connected
        kill(game->pidP1, SIGUSR2);  // tell player 1 he is the winner
        semOp(semid, 4, -1);         // wait for client to exit
      }
      shmctl(shmid, IPC_RMID, NULL);  // remove shared memory
      semctl(semid, IPC_RMID, 0);     // remove semaphore
      exit(0);
    }
    exit_ = true;
  }
  semOp(semid, 3, 1);
  sigprocmask(SIG_SETMASK, &defSet, NULL);
}

int main(int argc, char const *argv[]) {
  // arguments check
  if (argc != 4) {
    if (argc > 4) {
      printf("%s%s%s\n", RED, "Errore: troppi argomenti", RESET);
    } else {
      printf("%s%s%s\n", RED, "Errore: argomenti insufficienti", RESET);
    }
    printf("%s%s%s\n", RED, "Uso corretto: ./TriServer durata_turno simbolo_P1 simbolo_P2", RESET);
    exit(1);
  }
  // turn timer check
  for (int i = 0; argv[1][i] != '\0'; i++) {
    if (argv[1][i] < '0' || argv[1][i] > '9') {
      printf("%s%s%s%s%s\n", RED, "Errore: \033[3m", argv[1], "\033[23m non è un numero valido per il timer del turno",
             RESET);
      printf("%s%s%s\n", RED, "Uso corretto: ./TriServer durata_turno simbolo_P1 simbolo_P2", RESET);
      fflush(stdout);
      exit(1);
    }
  }
  timer = atoi(argv[1]);

  // players symbols check
  if (strlen(argv[2]) != 1 || strlen(argv[3]) != 1) {
    printf("%s%s%s\n", RED, "Errore: solo 1 carattere ammesso per ogni giocatore", RESET);
    printf("%s%s%s\n", RED, "Uso corretto: ./TriServer durata_turno simbolo_P1 simbolo_P2", RESET);
    fflush(stdout);
    exit(1);
  }
  char p1Symbol = argv[2][0];
  char p2Symbol = argv[3][0];

  strncpy(path, argv[0], sizeof(path));

  int fd[2];
  if (pipe(fd) == -1) {
    perror("Pipe failed\n");
    exit(1);
  }

  char pidBuff[64];
  switch (fork()) {
    case 0:
      close(fd[0]);  // close pipe's read
      close(1);      // close stdout
      dup(fd[1]);    // put pipe's write onto stdout fd
      close(fd[1]);  // close old pipe write
      execlp("pgrep", "pgrep", "TriServer", NULL);
      perror("exec failed");
      exit(1);
      break;

    case -1:
      perror("fork failed");
      exit(1);
      break;

    default:
      close(fd[1]);
      if (read(fd[0], pidBuff, sizeof(pidBuff)) == -1) {
        perror("read failed");
        exit(1);
      }

      // check if another server is running
      for (int i = 0; i < sizeof(pidBuff); i++) {
        if (pidBuff[i] == '\n') {
          if (atoi(&pidBuff[i]) != 0) {
            printf("%sUn altro server è gia in funzione%s\n", ORANGE, RESET);
            exit(1);
          } else {
            break;
          }
        }
      }
  }

  key_t key;

  if ((key = ftok(FILENO, 0)) == -1) {
    perror("ftok");
    exit(1);
  }
  // create seamphore
  if ((semid = semget(key, 5, IPC_CREAT | 0600)) == -1) {
    perror("semget");
    exit(1);
  }

  // initialize semaphores
  semCtl(semid, 0, SETVAL, 0, "INITIALIZE");
  semCtl(semid, 1, SETVAL, 1, "INITIALIZE");
  semCtl(semid, 2, SETVAL, 0, "INITIALIZE");
  semCtl(semid, 3, SETVAL, 1, "INITIALIZE");
  semCtl(semid, 4, SETVAL, 1, "INITIALIZE");

  // manage existing shm
  shmid = shmget(key, sizeof(Game), IPC_CREAT | IPC_EXCL | 0600);  // try creating a new shared memory segment
  if (shmid == -1) {
    write(1, "Esiste già un segmento di memoria condivisa con questa chiave\n", 64);
    int shmid_old = shmget(key, sizeof(Game), IPC_CREAT);            // get old shared memory id
    shmctl(shmid_old, IPC_RMID, NULL);                               // remove old shared memory id
    shmid = shmget(key, sizeof(Game), IPC_CREAT | IPC_EXCL | 0600);  // create new shared memory segment
    write(1, "Vecchio segmento di memoria rimosso, ne è stato creato uno nuovo\n\n", 68);
  }
  game = shmat(shmid, NULL, 0600);  // attach to shm

  initializeGame(timer, p1Symbol, p2Symbol);

  // initialize the set with all the signals
  sigfillset(&mySet);

  signal(SIGUSR1, playerHandle);  // start listening for players joining
  signal(SIGUSR2, playerMove);    // player move handler
  signal(SIGINT, exitHandler);    // set CTRL+C to double confirmation
  signal(SIGTERM, shutdown);      //
  signal(SIGQUIT, shutdown);      //
  signal(SIGHUP, shutdown);       // closing terminal

  printf("\r%s\033[1;46m%s%s\n", CLEAR_LINE, " Inizializzazione server completata! ", RESET);
  fflush(stdout);
  pthread_create(&waitThread[0], NULL, waitingForPlayers, game);
  int firstRound = 1;  // is this the first round?

  while (1) {
    if (game->victory != -1) {
      write(1, "\033[1;46m Server chiuso \033[0m\n", 28);
      shmdt(&shmid);                  // detach from shared memory
      shmctl(shmid, IPC_RMID, NULL);  // remove shared memory
      semctl(semid, 0, IPC_RMID);     // remove semaphore
      exit(0);
    } else if (game->players == 2) {
      if (firstRound) {
        sigprocmask(SIG_SETMASK, &mySet, NULL);
        semOp(semid, 0, -1);                    // wait for client to unlock join 1
        semOp(semid, 3, -1);                    // wait for client to unlock join 2
        semCtl(semid, 0, SETVAL, 0, "JOIN 1");  // lock semaphore join 1
        semCtl(semid, 3, SETVAL, 0, "JOIN 2");  // lock semaphore join 2
        sigprocmask(SIG_SETMASK, &defSet, NULL);

        pthread_cancel(waitThread[0]);
        printf("\n");
        moveUpLine(3);
        printf("\r");
        clearLine();
        printf("%s\n\r%s%s\n\n\r%s%s\n", "Giocatore 1 \033[32m✔\033[0m", CLEAR_LINE, "Giocatore 2 \033[32m✔\033[0m",
               CLEAR_LINE, "\033[1;46m Tutti i giocatori pronti, che la partita abbia inizio! \033[0m");
        fflush(stdout);
        firstRound = 0;
      }
      startGame();

      // exit program
    } else if (game->bot == 1 && game->players == 1) {
      switch (fork()) {
        case -1:
          perror("bot fork: ");
          exit(1);
          break;

        case 0:
          path[strlen(path) - 9] = '\0';
          strcat(path, "TriBot");
          char *args[] = {path, NULL};
          if (execv(args[0], args) == -1) exit(1);
          break;

        default:
          game->bot = 2;
          break;
      }
    } else {
      pause();
    }
  }
  return 0;
}
