CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L
LDFLAGS=-lm
BINARIES=spx_exchange spx_trader spx_auto_trader

.PHONY: generate
generate: clean spx_exchange.c spx_trader.c spx_common.h spx_exchange.h spx_trader.h

	$(CC) -o spx_exchange spx_exchange.c $(CFLAGS) $(LDFLAGS)
	$(CC) -o spx_trader spx_trader.c $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(BINARIES)
