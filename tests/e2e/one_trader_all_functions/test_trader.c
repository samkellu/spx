#include "../../../spx_trader.h"
#include "../../../spx_common.h"

int read_flag = 0;
int running = 1;
int market_open = 0;

void sig_read(int errno) {
  read_flag = 1;
  return;
}

int main(int argc, char ** argv) {
  signal(SIGUSR1, sig_read);

  pid_t ppid = getppid();
  int id = strtol(argv[1], NULL, 10);
  char path[PATH_LENGTH];

  FILE* f = fopen("tests/e2e/one_trader_all_functions/trader.test", "w");

  sprintf(path, "/tmp/spx_exchange_%d", id);
  int exchange_fd = open(path, O_RDONLY);
  sprintf(path, "/tmp/spx_trader_%d", id);
  int trader_fd = open(path, O_WRONLY);

  char orders[10][MAX_INPUT] = {"BUY 0 GPU 30 50;", "SELL 1 Router 20 30;", "SELL 2 GPU 30 20;",\
                  "SELL 3 GPU 100 100;", "AMEND 3 30 30;", "CANCEL 1;", "CANCEL 12;",\
                  "AMEND 1 2 3;", "SELL 4 Router 20 30;", "BUY 5 Router 20 30;"};
  int order_id = 0;
  while (running) {

    if (read_flag) {

      read_flag = 0;
      char buf[MAX_INPUT] = "";
      char* token;

      read(exchange_fd, buf, MAX_INPUT);

      token = strtok(buf, ";");
      while (token != NULL) {
        fprintf(f,"[Trader %d] Received from SPX: %s;\n", id, token);
        token = strtok(NULL, ";");
      }

      token = strtok(buf, " ");
      if (!market_open) {
        if (strcmp(buf, "MARKET") == 0) {
          market_open = 1;
        }
      }

      if (market_open && (strcmp(token, "FILL") == 0 || strcmp(token, "MARKET") == 0 || strcmp(token, "ACCEPTED") == 0 || strcmp(token, "INVALID") == 0 || strcmp(token, "AMENDED") == 0 || strcmp(token, "CANCELLED") == 0)) {

        char* msg = malloc(MAX_INPUT);
        snprintf(msg, MAX_INPUT, "%s", orders[order_id]);
        write(trader_fd, msg, strlen(msg));
        kill(ppid, SIGUSR1);
        fprintf(f, "[Trader %d] Event: %s\n", id, msg);
        order_id++;
        free(msg);
      }
    }

    if (order_id == 10) {
      int counter = 0;

      while (counter++ < 100) {

        int* stay_awake = malloc(0);
        struct timespec tim, tim2;
        tim.tv_sec = 0;
        tim.tv_nsec = 10000;
        nanosleep(&tim , &tim2);

        if (read_flag) {
          read_flag = 0;
          counter = 0;
          char buf[MAX_INPUT] = "";
          char* token;

          read(exchange_fd, buf, MAX_INPUT);

          token = strtok(buf, ";");
          while (token != NULL) {
            fprintf(f,"[Trader %d] Received from SPX: %s;\n", id, token);
            token = strtok(NULL, ";");
          }
        }
        free(stay_awake);
      }
      fclose(f);
      return 0;
    }
  }
  fclose(f);
  return 0;
}
