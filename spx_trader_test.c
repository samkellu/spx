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
    // if (argc < 2) {
    //     printf("Not enough arguments\n");
    //     return 1;
    // }
    signal(SIGUSR1, sig_read);

    pid_t ppid = getppid();
    int id = strtol(argv[1], NULL, 10);
    char path[PATH_LENGTH];

    sprintf(path, "/tmp/spx_exchange_%d", id);
    int exchange_fd = open(path, O_RDONLY);
    sprintf(path, "/tmp/spx_trader_%d", id);
    int trader_fd = open(path, O_WRONLY);

    int order_id = 0;

    while (running) {
      sleep(1);
      if (read_flag) {

        read_flag = 0;
        char buf[MAX_INPUT] = "";
        // char* token;
        // char** args = malloc(0);
        // int arg_counter = 0;

        read(exchange_fd, buf, MAX_INPUT);

        if (!market_open) {
          printf("%s",buf);
          if (strcmp(buf, "MARKET OPEN;") == 0) {
            printf("market open\n");
            market_open = 1;
            continue;
          }
        }
      }


      if (market_open) {
        char* msg = malloc(MAX_INPUT);
        snprintf(msg, MAX_INPUT, "BUY %d GPU 10 11;", order_id++);
        write(trader_fd, msg, strlen(msg) + 1);
        kill(ppid, SIGUSR1);
        free(msg);
      }

      if (order_id == 5) {
        return 0;
      }
    //     token = strtok(buf, " ");
    //     while (token != NULL) {
    //       args = realloc(args, sizeof(char*) * ++arg_counter);
    //       char* arg = malloc(PRODUCT_LENGTH);
    //       memcpy(arg, token, PRODUCT_LENGTH);
    //       args[arg_counter - 1] = arg;
    //       token = strtok(NULL, " ");
    //     }
    //
    //     if (strcmp(args[1], "SELL") == 0) {
    //       if (strtol(args[3], NULL, 10) >= QTY_LIMIT) {
    //         for (int arg_num = 0; arg_num < arg_counter; arg_num++) {
    //           free(args[arg_num]);
    //         }
    //         free(args);
    //         return 0;
    //       }
    //       char* msg = malloc(MAX_INPUT);
    //       snprintf(msg, MAX_INPUT, "BUY %d %s %s %s;", order_id++, args[2], args[3], args[4]);
    //       write(trader_fd, msg, strlen(msg) + 1);
    //       kill(ppid, SIGUSR1);
    //       free(msg);
    //     }
    //
    //     for (int arg_num = 0; arg_num < arg_counter; arg_num++) {
    //       free(args[arg_num]);
    //     }
    //     free(args);
    //   }
    // }}
  }
  return 0;
}