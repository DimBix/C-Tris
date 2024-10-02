#include "headers/ipclib.h"

int create_shm() {
  key_t key = ftok(FILENO, 0);
  if (key == IPC_ERR) {
    return IPC_ERR;
  }
  return shmget(key, sizeof(Game), IPC_CREAT | IPC_EXCL | 0600);
}

Game* attach_shm() {
  key_t key = ftok(FILENO, 0);
  if (key == IPC_ERR) {
    return NULL;
  }
  int shmid = shmget(key, sizeof(Game), IPC_CREAT);
  return shmat(shmid, NULL, 0600);
}

void semOp(int semid, unsigned short semnum, unsigned short op) {
  struct sembuf sop = {.sem_num = semnum, .sem_flg = 0, .sem_op = op};

  if (semop(semid, &sop, 1) == -1) {
    perror("semop: ");
    exit(1);
  }

  return;
}

void semCtl(int semid, int semnum, int cmd, int value, char* semname) {
  if (semctl(semid, semnum, cmd, value) == -1) {
    perror("semctl ");
    perror(semname);
    semctl(semid, IPC_RMID, 0);  // remove semaphore
    exit(1);
  }
}
