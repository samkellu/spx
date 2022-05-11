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
    FILE* f = fopen("tests/e2e/two_traders_all_functions/trader2.test", "w");

    char* all_recv = malloc(8192);
    int cursor = 0;

    int order_id = 0;

    while (running) {
      if (read_flag) {

        read_flag = 0;
        char buf[MAX_INPUT] = "";
        char* token;
        char args[10][PRODUCT_LENGTH];
        int arg_counter = 0;

        read(exchange_fd, buf, MAX_INPUT);
        token = strtok(buf, ";");
        while (token != NULL) {
          char* tmp = malloc(2*MAX_INPUT);
          snprintf(tmp, 2*MAX_INPUT, "[Trader %d] Received from SPX: %s;\n", id, buf);

          snprintf(all_recv + cursor, strlen(tmp) + 1, "%s", tmp);
          cursor += strlen(tmp) + 1;
          free(tmp);
          token = strtok(NULL, ";");
        }

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
