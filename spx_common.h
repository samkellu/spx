#ifndef SPX_COMMON_H
#define SPX_COMMON_H

#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>

#define FIFO_EXCHANGE "/tmp/spx_exchange_%d"
#define FIFO_TRADER "/tmp/spx_trader_%d"
#define FEE_PERCENTAGE 1
#define PRODUCT_LENGTH (16)
#define PATH_LENGTH (32)
#define MAX_INPUT (64)
#define MAX_TRADERS_BYTES (4)
#define MAX_PID (6)

#endif
