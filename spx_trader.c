#include "spx_trader.h"

int main(int argc, char ** argv) {
    if (argc < 1) {
        printf("Not enough arguments\n");
        return 1;
    }
    int id = strtol(argv[0], NULL, 10);
    printf("This is trader %d\n", id);
    return 0;

    // register signal handler

    // connect to named pipes

    // event loop:

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)

}
