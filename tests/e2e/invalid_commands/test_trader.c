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

  FILE* f = fopen("tests/E2E/invalid_commands/trader.test", "w");

  sprintf(path, "/tmp/spx_exchange_%d", id);
  int exchange_fd = open(path, O_RDONLY);
  sprintf(path, "/tmp/spx_trader_%d", id);
  int trader_fd = open(path, O_WRONLY);

  char orders[11][MAX_INPUT] = {"INVALID;", "SELL -1 GPU 10 10;", "SELL 1 GPU 30 20;",\
                  "SELL 0 DOG 100 100;", "SELL 0 GPU -1 100;", "SELL 0 GPU 10 -100;",\
                  "SELL 0 GPU 1000000 100;", "SELL 0 GPU 100 1000000;", "SELL 0 GPU 1000000 100",\
                "CANCEL 12;", "AMEND 22 22 22;"};
  int order_id = 0;
  int counter = 0;

  while (running) {

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 10000;
    nanosleep(&tim , &tim2);
    counter++;

    if (read_flag) {

      read_flag = 0;
      char buf[MAX_INPUT] = "";

      read(exchange_fd, buf, MAX_INPUT);

      fprintf(f,"[Trader %d] Received from SPX: %s\n", id, buf);

      if (!market_open) {
        if (strcmp(buf, "MARKET OPEN;") == 0) {
          market_open = 1;
        }
      }
    }

    if (order_id == 11) {
      struct timespec tim, tim2;
      tim.tv_sec = 1;
      tim.tv_nsec = 0;
      nanosleep(&tim , &tim2);

      int read_timer = 0;
      while (read_timer < 100) {
        struct timespec tim, tim2;
        tim.tv_sec = 0;
        tim.tv_nsec = 100000;
        nanosleep(&tim , &tim2);

        if (read_flag) {
          read_timer = 0;
          read_flag = 0;
          char buf[MAX_INPUT] = "";

          read(exchange_fd, buf, MAX_INPUT);

          fprintf(f,"[Trader %d] Received from SPX: %s\n", id, buf);
        }
        read_timer++;
      }

      fclose(f);
      return 0;
    }
    if (counter >= 1000 && market_open && !read_flag) {
      counter = 0;
      char* msg = malloc(MAX_INPUT);
      snprintf(msg, strlen(orders[order_id]) + 1, "%s", orders[order_id]);
      write(trader_fd, msg, strlen(msg));
      kill(ppid, SIGUSR1);
      fprintf(f, "[Trader %d] Event: %s\n", id, msg);
      order_id++;
      free(msg);
    }

  }
  return 0;
}
