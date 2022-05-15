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

    sprintf(path, "/tmp/spx_exchange_%d", id);
    int exchange_fd = open(path, O_RDONLY);
    sprintf(path, "/tmp/spx_trader_%d", id);
    int trader_fd = open(path, O_WRONLY);
    FILE* f = fopen("tests/E2E/two_traders_all_functions/trader2.test", "w");

    int order_id = 0;

    while (running) {

      if (!read_flag) {
        pause();
      }

      if (read_flag) {

        read_flag = 0;
        char buf[MAX_INPUT] = "";
        char* token;
        char args[10][PRODUCT_LENGTH];
        int arg_counter = 0;

        read(exchange_fd, buf, MAX_INPUT);
        token = strtok(buf, ";");

        if (!market_open) {
          if (strcmp(buf, "MARKET OPEN;") == 0) {
            market_open = 1;
            continue;
          }
        }

        token = strtok(buf, " ");
        while (token != NULL) {
          memcpy(args[arg_counter++], token,strlen(token) + 1);
          token = strtok(NULL, " ");
        }

        if (strcmp(args[1], "SELL") == 0) {
          if (strtol(args[3], NULL, 10) >= QTY_LIMIT) {

            fclose(f);
            return 0;
          }
          long price = strtol(args[4], NULL, 10) - 1;
          if (price <= 0) {
            continue;
          }
          char* msg = malloc(MAX_INPUT);
          snprintf(msg, MAX_INPUT, "BUY %d %s %s %d;", order_id++, args[2], args[3], (int)price);
          write(trader_fd, msg, strlen(msg));
          kill(ppid, SIGUSR1);
          fprintf(f, "[Trader %d] EVENT: %s\n", id, msg);
          free(msg);
        }
      }
    }

    fflush(stdout);
    fclose(f);
    return 0;
}
