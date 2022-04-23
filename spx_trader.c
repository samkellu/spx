#include "spx_trader.h"
#include "spx_common.h"

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    int id = strtol(argv[0], NULL, 10);
    pid_t ppid = strtol(argv[1], NULL, 10);
    int fd = open("/tmp/spx_exchange_0", O_NONBLOCK);
    sleep(5);
    write(fd, "SELL what the fuck", strlen("SELL what the fuck") + 1);
    close(fd);
    printf("This is trader %d\n", id);
    return 0;

    // register signal handler

    // connect to named pipes

    // event loop:

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)

}
