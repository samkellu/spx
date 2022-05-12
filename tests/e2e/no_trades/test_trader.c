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

  int id = strtol(argv[1], NULL, 10);
  char path[PATH_LENGTH];

  sprintf(path, "/tmp/spx_exchange_%d", id);
  int exchange_fd = open(path, O_RDONLY);
  sprintf(path, "/tmp/spx_trader_%d", id);
  int trader_fd = open(path, O_WRONLY);

  write(trader_fd, "unused var", strlen("unused var"));

  while (running) {

    if (read_flag) {

      read_flag = 0;
      char buf[MAX_INPUT] = "";

      read(exchange_fd, buf, MAX_INPUT);

      if (strcmp(buf, "MARKET OPEN;") == 0) {
        break;
      }
    }
  }
  return 0;
}
