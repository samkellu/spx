#include "spx_trader.h"
#include "spx_common.h"

int read_flag = 0;

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
    int id = strtol(argv[1], NULL, 10);
    char path[PATH_LENGTH];

    int order_id = 0;
    int valid = 1;
    int exponent = 1;

    sprintf(path, "/tmp/spx_exchange_%d", id);
    int exchange_fd = open(path, O_RDONLY);
    sprintf(path, "/tmp/spx_trader_%d", id);
    int trader_fd = open(path, O_WRONLY);


    while (1) {

      if (valid) {
        exponent = 0;
        pause();

      } else {

        // Exponential back-off function helps minimise the effect of lost signals,
        // as they are re-sent at exponentially increasing intervals until they are
        // received
        kill(ppid, SIGUSR1);
        double nsec = 1000000000 * (pow(1.3, exponent++)/4);

        struct timespec tim, tim2;
        tim.tv_sec = 0;
        tim.tv_nsec = nsec;
        nanosleep(&tim , &tim2);

        // Break, in the event that the exponent has become excessively large
        if (exponent == 10) {
          exponent = 0;
          return -1;
        }
      }

      if (read_flag) {

        read_flag = 0;
        char buf[MAX_INPUT] = "";
        char* token;
        char** args = malloc(0);
        int arg_counter = 0;

        if (read(exchange_fd, buf, MAX_INPUT) < MIN_READ) {
          continue;
        }

        token = strtok(buf, " ");
        while (token != NULL) {
          args = realloc(args, sizeof(char*) * ++arg_counter);
          char* arg = malloc(PRODUCT_LENGTH);
          memcpy(arg, token, PRODUCT_LENGTH);
          args[arg_counter - 1] = arg;
          token = strtok(NULL, " ");
        }

        if (strcmp(args[0], "ACCEPTED") == 0) {
          char* tmp = malloc(MAX_TRADERS_BYTES);
          snprintf(tmp, MAX_TRADERS_BYTES, "%d;", order_id);
          if (strcmp(args[1],tmp) == 0) {
            valid = 1;
            order_id++;
          }
        }

        if (strcmp(args[1], "SELL") == 0) {
          if (strtol(args[3], NULL, 10) >= QTY_LIMIT) {
            for (int arg_num = 0; arg_num < arg_counter; arg_num++) {
              free(args[arg_num]);
            }
            free(args);
            return 0;
          }

          if (valid) {

            valid = 0;

            char* msg = malloc(MAX_INPUT);
            snprintf(msg, MAX_INPUT, "BUY %d %s %s %s", order_id, args[2], args[3], args[4]);
            write(trader_fd, msg, strlen(msg));
            kill(ppid, SIGUSR1);
            free(msg);
          }
        }

        for (int arg_num = 0; arg_num < arg_counter; arg_num++) {
          free(args[arg_num]);
        }
        free(args);
      }
    }
    return 0;
}
