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

  char orders[20][MAX_INPUT] = {"SELL 0 GPU 30 500;", "SELL 1 GPU 30 501;", "SELL 2 GPU 30 501;", \
                  "SELL 3 GPU 30 502;", "SELL 4 Router 22 51;", "CANCEL 3;", "AMEND 1 30 502;", \
                  "AMEND 2 30 503;","AMEND 3 30 504;", "AMEND 4 22 505;", \
                  "SELL 5 Router 10 100;", "BUY 6 GPU 30 50;", "SELL 7 Router 20 30;", "SELL 8 Router 1 1;", \
                  "SELL 9 GPU 100 100;", "AMEND 6 30 30;", "CANCEL 1;", "CANCEL 12;", \
                  "BUY 10 Router 1000 1;", "BUY 11 Router 100 1000;"};

  char* all_recv = malloc(MAX_INPUT*MAX_INPUT*MAX_INPUT);
  int cursor = 0;

  int order_id = 0;
  int counter = 0;
  while (counter++ < 2000) {

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 10000;
    nanosleep(&tim , &tim2);

    if (read_flag && counter > 200) {

      read_flag = 0;
      counter = 0;
      char* line_tok;
      char buf[2*MAX_INPUT];

      if (read(exchange_fd, buf, MAX_INPUT) <= 0) {
        continue;
      }

      line_tok = strtok(buf, ";");
      while (line_tok != NULL) {
        char* tmp = malloc(4*MAX_INPUT);
        snprintf(tmp, 4*MAX_INPUT, "[Trader %d] Received from SPX: %s;\n", id, line_tok);

        if (strlen(tmp) > 38) {
          snprintf(all_recv + cursor, 4*MAX_INPUT, "%s", tmp);
          cursor += strlen(tmp);
        }

        free(tmp);
        line_tok = strtok(NULL, ";");
      }
    }

    if (counter > 800 && order_id < 20) {
      char* msg = malloc(MAX_INPUT);
      snprintf(msg, MAX_INPUT, "%s", orders[order_id]);
      write(trader_fd, msg, strlen(msg));
      kill(ppid, SIGUSR1);
      fprintf(f, "[Trader %d] Event: %s\n", id, msg);

      order_id++;
      free(msg);
      counter = 0;
    }
  }
  fprintf(f, "\n\nSPX ---> TRADER\n\n");
  for (int x = 0; x < 8192; x++) {
    if (all_recv[x] >= 0 && all_recv[x] <= 177){
      if (all_recv[x] == '\t') {
        continue;
      }
      fprintf(f, "%c", all_recv[x]);
    }
  }
  free(all_recv);
  sleep(4);
  fclose(f);
  return 0;
}
