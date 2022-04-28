.DEFAULT_GOAL := generate

CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm
BINARIES=spx_exchange spx_trader spx_auto_trader

generate: spx_exchange.c spx_trader.c spx_auto_trader.c spx_common.h spx_exchange.h spx_trader.h

	$(CC) -o spx_echange spx_exchange.c $(CFLAGS)
	$(CC) -o spx_trader spx_trader.c $(CFLAGS)
	$(CC) -o spx_auto_trader spx_auto_trader.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(BINARIES)
