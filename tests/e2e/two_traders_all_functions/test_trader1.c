#include "../../../spx_trader.h"
#include "../../../spx_common.h"

int read_flag = 0;
int running = 1;

void sig_read(int errno) {
  read_flag = 1;
  return;
}

int main(int argc, char ** argv) {
  signal(SIGUSR1, sig_read);

  pid_t ppid = getppid();
  int id = strtol(argv[1], NULL, 10);
  char path[PATH_LENGTH];

  FILE* f = fopen("tests/e2e/two_traders_all_functions/trader1.test", "w");

  sprintf(path, "/tmp/spx_exchange_%d", id);
  int exchange_fd = open(path, O_RDONLY);
  sprintf(path, "/tmp/spx_trader_%d", id);
  int trader_fd = open(path, O_WRONLY);

  char orders[11][MAX_INPUT] = {"SELL 0 GPU 30 500;", "SELL 1 GPU 30 501;", "SELL 2 GPU 30 501;"\
                  ,"SELL 3 GPU 30 502;", "SELL 4 Router 22 51;", "CANCEL 0;", "AMEND 1 30 502;", \
                  "AMEND 2 30 503;","AMEND 3 30 504;", "AMEND 4 22 505;",
                  "SELL 5 Router 100000 100000;"};

  int order_id = 0;
  int market_open = 0;

  while (running) {

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 1000;
    nanosleep(&tim , &tim2);

    if (read_flag) {

      read_flag = 0;
      char buf[MAX_INPUT] = "";
      char* token;

      read(exchange_fd, buf, MAX_INPUT);

      fprintf(f,"[Trader %d] Received from SPX: %s\n", id, buf);

      token = strtok(buf, " ");
      if (!market_open) {
        if (strcmp(buf, "MARKET") == 0) {
          market_open = 1;
        }
      }

      if (market_open && (strcmp(token, "MARKET") == 0 || strcmp(token, "ACCEPTED") == 0 || strcmp(token, "INVALID;") == 0 || strcmp(token, "AMENDED") == 0 || strcmp(token, "CANCELLED") == 0)) {
        if (strcmp(token, "MARKET") != 0) {
          struct timespec tim, tim2;
          tim.tv_sec = 0;
          tim.tv_nsec = 100;
          nanosleep(&tim , &tim2);
        }

        char* msg = malloc(MAX_INPUT);
        snprintf(msg, strlen(orders[order_id]) + 1, "%s", orders[order_id]);
        fflush(stdout);
        write(trader_fd, msg, strlen(msg));
        kill(ppid, SIGUSR1);
        fprintf(f, "[Trader %d] Event: %s\n", id, msg);
        order_id++;
        free(msg);
      }
    }

    if (order_id >= 11) {
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

          read(exchange_fd, buf, MAX_INPUT);

          fprintf(f,"[Trader %d] Received from SPX: %s\n", id, buf);
        }
        free(stay_awake);
      }
      fclose(f);
      close(exchange_fd);
      close(trader_fd);
      return 0;
    }
  }
  fclose(f);
  close(exchange_fd);
  close(trader_fd);
  return 0;
}
