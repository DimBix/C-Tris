#include "headers/printable.h"

#include "headers/ipclib.h"

void printVictory(int winner, int player) {
  if (winner == player) {
    printf(
        "\033[1;32m _   _   ___   _____   _   _  _____  _   _  _____  _____ \n"
        "| | | | / _ \\ |_   _| | | | ||_   _|| \\ | ||_   _||  _  |\n"
        "| |_| |/ /_\\ \\  | |   | | | |  | |  |  \\| |  | |  | | | |\n"
        "|  _  ||  _  |  | |   | | | |  | |  | . ` |  | |  | | | |\n"
        "| | | || | | | _| |_  \\ \\_/ / _| |_ | |\\  |  | |  \\ \\_/ /\n"
        "\\_| |_/\\_| |_/ \\___/   \\___/  \\___/ \\_| \\_/  \\_/   \\___/ "
        "\033[0m\033[?25h\n");
  } else if (winner == 0) {
    printf(
        "\033[1;33m______   ___  ______  _____ \n"
        "| ___ \\ / _ \\ | ___ \\|_   _|\n"
        "| |_/ // /_\\ \\| |_/ /  | |  \n"
        "|  __/ |  _  ||    /   | |  \n"
        "| |    | | | || |\\ \\  _| |_ \n"
        "\\_|    \\_| |_/\\_| \\_| \\___/ \033[0m\033[?25h\n");
  } else {
    printf(
        "%s _   _   ___   _____  ______  _____ ______  _____  _____ \n"
        "| | | | / _ \\ |_   _| | ___ \\|  ___|| ___ \\/  ___||  _  |\n"
        "| |_| |/ /_\\ \\  | |   | |_/ /| |__  | |_/ /\\ `--. | | | |\n"
        "|  _  ||  _  |  | |   |  __/ |  __| |    /  `--. \\| | | |\n"
        "| | | || | | | _| |_  | |    | |___ | |\\ \\ /\\__/ /\\ \\_/ /\n"
        "\\_| |_/\\_| |_/ \\___/  \\_|    \\____/ \\_| \\_|\\____/  \\___/ "
        "\033[0m\033[?25h\n",
        ORANGE);
  }
}

void printTable(int player, Game *game) {
  char mySymbol, oppSymbol;
  if (player == 1) {
    mySymbol = game->symbol1;
    oppSymbol = game->symbol2;
  } else {
    mySymbol = game->symbol2;
    oppSymbol = game->symbol1;
  }
  for (int i = 0; i < 3; i++) {  // print game table
    if (i == 0) {
      printf("\n\t╔═════╦═════╦═════╗\n");
    } else {
      printf("\n\t╠═════╬═════╬═════╣\n");
    }

    for (int j = 0; j < 3; j++) {
      if (j == 0) {
        printf("\t║");
      }
      if (game->table[i][j] == player) {
        printf("  %s%c%s  ║", GREEEEEN, mySymbol, RESET);
      } else if (game->table[i][j] == 0) {
        printf("     ║");
      } else {
        printf("  %s%c%s  ║", "\033[1m", oppSymbol, RESET);
      }
    }
  }
  printf("\n\t╚═════╩═════╩═════╝\n");
  fflush(stdout);
}

void *printTimer(void *args) {
  Game *game = (Game *)args;
  for (int i = game->timer; i > 0; i--) {
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
  if (game->timer == 0) {
    printf(SAVE_CURSOR_POSITION);
    printf("\r");
    printf(MOVE_UP);
    clearLine();
    printf("\033[1;33m%s%s%s", "Nessun limite, prenditi il tuo tempo!", RESTORE_CURSOR_POSITION, MOVE_UP);
    fflush(stdout);
  }

  printf("\n");
  fflush(stdout);

  pthread_exit(NULL);
}

void *waitingForPlayers(void *args) {
  Game *game = (Game *)args;
  printf("\n\n\n\n");
  fflush(stdout);
  for (int i = 0; i < 4; i++) {
    moveUpLine(3);
    clearLine();
    switch (i) {
      case 0:
        printf("\r%s%s\033[0m", LIGHT_YELLOW, "In attesa di giocatori");
        fflush(stdout);
        break;
      case 1:
        printf("\r%s%s\033[0m", LIGHT_YELLOW, "In attesa di giocatori.");
        fflush(stdout);
        break;
      case 2:
        printf("\r%s%s\033[0m", LIGHT_YELLOW, "In attesa di giocatori..");
        fflush(stdout);
        break;
      case 3:
        printf("\r%s%s\033[0m", LIGHT_YELLOW, "In attesa di giocatori...");
        fflush(stdout);
        break;
    }
    switch (game->players) {
      case 0:
        printf("\n\r%s%s\n\r%s%s\n", CLEAR_LINE, "Giocatore 1 \033[31m✖\033[0m", CLEAR_LINE,
               "Giocatore 2 \033[31m✖\033[0m");
        fflush(stdout);
        break;
      case 1:
        printf("\n\r%s%s\n\r%s%s\n", CLEAR_LINE, "Giocatore 1 \033[32m✔\033[0m", CLEAR_LINE,
               "Giocatore 2 \033[31m✖\033[0m");
        fflush(stdout);
        break;
      case 2:
        printf("\n\r%s%s\n\r%s%s\n", CLEAR_LINE, "Giocatore 1 \033[32m✔\033[0m", CLEAR_LINE,
               "Giocatore 2 \033[32m✔\033[0m");
        fflush(stdout);
        break;
    }
    i = i == 3 ? -1 : i;
    sleep(1);
  }
  pthread_exit(NULL);
}

void clearScreen() {
  printf("\033[2J");
  fflush(stdout);
}

void moveUpLine(int lines) {
  printf("\033[%dA", lines);
  fflush(stdout);
}

void moveDownLine(int lines) {
  printf("\033[%dB", lines);
  fflush(stdout);
}

void clearLine() {
  printf(CLEAR_LINE);
  fflush(stdout);
}