#include "spx_trader.h"
#include "spx_common.h"

int read_flag = 0;
int running = 1;
int market_open = 0;

void sig_read(int errno) {
  read_flag = 1;
  return;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    signal(SIGUSR1, sig_read);

    pid_t ppid = getppid();
    int id = strtol(argv[0], NULL, 10);
    char path[PATH_LENGTH];

    sprintf(path, "/tmp/spx_exchange_%d", id);
    int exchange_fd = open(path, O_RDWR);
    sprintf(path, "/tmp/spx_trader_%d", id);
    int trader_fd = open(path, O_RDWR);

    int debug_count = 0;

    while (running) {
      sleep(1);
      debug_count++;
      if (debug_count == 20) {
        return 0;
      }

      if (market_open) {
        write(trader_fd, "BUY 0 GPU 1 12;\n", strlen("BUY 0 GPU 1 12;\n") + 1);
        kill(ppid, SIGUSR1);
        sleep(2);
        return 0;
      }

      if (read_flag) {
        char buf[MAX_INPUT] = "";
        read(exchange_fd, buf, MAX_INPUT);
        printf("[Trader %d] [t=%d] Received from SPX: %s\n", id, 0, buf);

        if (!market_open) {
          if (strcmp(buf, "MARKET OPEN;") == 0) {
            market_open = 1;
          }
        } else {

        }
        read_flag = 0;
      }
    }
    return 0;


    // event loop:

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)

}
