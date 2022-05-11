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
                  ,"SELL 3 GPU 30 502;", "SELL 4 Router 22 51;", "CANCEL 3;", "AMEND 1 30 502;", \
                  "AMEND 2 30 503;","AMEND 3 30 504;", "AMEND 4 22 505;",
                  "SELL 5 Router 100000 100000;"};

  int order_id = 0;
  char* all_recv = malloc(8192);
  int cursor = 0;
  int counter = 0;

  while (counter++ < 8000) {

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 100000;
    nanosleep(&tim , &tim2);

    if (read_flag) {

      read_flag = 0;
      counter = 0;
      char* line_tok;
      char buf[MAX_INPUT];

      read(exchange_fd, buf, MAX_INPUT);

      line_tok = strtok(buf, ";");
      while (line_tok != NULL) {
        char* tmp = malloc(2*MAX_INPUT);
        snprintf(tmp, 2*MAX_INPUT, "[Trader %d] Received from SPX: %s;\n", id, buf);

        snprintf(all_recv + cursor, strlen(tmp) + 1, "%s", tmp);
        cursor += strlen(tmp) + 1;
        free(tmp);
        line_tok = strtok(NULL, ";");
      }
    }

    if (counter > 800 && order_id < 11) {
      char* msg = malloc(MAX_INPUT);
      snprintf(msg, MAX_INPUT, "%s", orders[order_id]);
      write(trader_fd, msg, strlen(msg));
      kill(ppid, SIGUSR1);
      fprintf(f, "[Trader %d] Event: %s\n", id, msg);

      order_id++;
      fflush(stdout);
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
  sleep(5);
  fclose(f);
  return 0;
  }
