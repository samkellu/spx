#include "spx_trader.h"
#include "spx_common.h"

int read_flag = 0;
int running = 1;
int market_open = 0;

void sig_read(int errno) {
  read_flag = 1;
  printf("reading");
  fflush(stdout);
  return;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    int id = strtol(argv[0], NULL, 10);
    pid_t ppid = strtol(argv[1], NULL, 10);
    signal(SIGUSR1, sig_read);
    int this_fd = open("/tmp/spx_trader_0", O_RDWR | O_NONBLOCK);
    int fd = open("/tmp/spx_exchange_0", O_RDWR | O_NONBLOCK);

    int debug_count = 0;

    while (running) {
      sleep(1);
      debug_count++;
      if (debug_count == 10) {
        return 0;
      }

      if (market_open) {
        write(fd, "SELL what the fuck", strlen("SELL what the fuck") + 1);
        kill(ppid, SIGUSR1);
      }

      if (read_flag && !market_open) {
        char buf[MAX_INPUT] = "";
        read(this_fd, buf, MAX_INPUT);
        printf("\nread from pipe %s\n", buf);
        if (strcmp(buf, "MARKET OPEN;") == 0) {
          market_open = 1;
        }
        read_flag = 0;
      }
    }
    printf("This is trader %d\n", id);
    return 0;

    // register signal handler

    // connect to named pipes

    // event loop:

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)

}
