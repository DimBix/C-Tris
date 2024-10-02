#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int table[3][3] = {0};
int moves = 0;

void printVictory(int hasWon) {
  if (hasWon == 1) {
    printf(
        " _   _   ___   _____   _   _  _____  _   _  _____  _____ \n"
        "| | | | / _ \\ |_   _| | | | ||_   _|| \\ | ||_   _||  _  |\n"
        "| |_| |/ /_\\ \\  | |   | | | |  | |  |  \\| |  | |  | | | |\n"
        "|  _  ||  _  |  | |   | | | |  | |  | . ` |  | |  | | | |\n"
        "| | | || | | | _| |_  \\ \\_/ / _| |_ | |\\  |  | |  \\ \\_/ /\n"
        "\\_| |_/\\_| |_/ \\___/   \\___/  \\___/ \\_| \\_/  \\_/   \\___/ \n");
  } else if (hasWon == 2) {
    printf(
        " _   _   ___   _____  ______  _____ ______  _____  _____ \n"
        "| | | | / _ \\ |_   _| | ___ \\|  ___|| ___ \\/  ___||  _  |\n"
        "| |_| |/ /_\\ \\  | |   | |_/ /| |__  | |_/ /\\ `--. | | | |\n"
        "|  _  ||  _  |  | |   |  __/ |  __| |    /  `--. \\| | | |\n"
        "| | | || | | | _| |_  | |    | |___ | |\\ \\ /\\__/ /\\ \\_/ /\n"
        "\\_| |_/\\_| |_/ \\___/  \\_|    \\____/ \\_| \\_|\\____/  \\___/ \n");
  } else {
    printf(
        "______   ___  ______  _____ \n"
        "| ___ \\ / _ \\ | ___ \\|_   _|\n"
        "| |_/ // /_\\ \\| |_/ /  | |  \n"
        "|  __/ |  _  ||    /   | |  \n"
        "| |    | | | || |\\ \\  _| |_ \n"
        "\\_|    \\_| |_/\\_| \\_| \\___/ \n");
  }
}

int result(int a[3][3]) {
  if (moves < 3) {  // can't win if you don't make 3 moves
    return -1;
  }
  for (int i = 0; i < 3; i++) {
    if (table[i][0] != 0) {
      if (table[i][0] == table[i][1] && table[i][0] == table[i][2]) {
        return table[i][0];
      }
    }
    if (table[0][i] != 0) {
      if (table[0][i] == table[1][i] && table[0][i] == table[2][i]) {
        return table[0][i];
      }
    }
  }

  if (table[1][1] != 0) {
    if (table[0][0] == table[1][1] && table[0][0] == table[2][2]) {
      return table[1][1];
    } else if (table[0][2] == table[1][1] && table[0][2] == table[2][0]) {
      return table[1][1];
    }
  }

  int found = 0;
  for (int i = 0; i < 3 && !found; i++) {
    for (int j = 0; j < 3; j++) {
      if (table[i][j] == 0) {
        found = 1;
        break;
      }
    }
  }
  if (!found)
    return 0;
  else
    return -1;
}

int ask() {
  int x = -1;
  do {
    scanf("%d", &x);
  } while (x < 0 && x > 2);
  return x;
}

int main(int argc, char const *argv[]) {
  int turn = 1;
  int winner;
  do {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        printf("%d ", table[i][j]);
      }
      printf("\n");
    }

    int x, y;
    do {
      printf("\n\rFai una mossa X Y\n");
      x = ask();
      y = ask();
    } while (table[x][y] != 0);
    table[x][y] = turn;
    turn = turn == 1 ? 2 : 1;
    moves++;
  } while ((winner = result(table)) == -1);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("%d ", table[i][j]);
    }
    printf("\n");
  }
  printVictory(winner);
  return 0;
}
