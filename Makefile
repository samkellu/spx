CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm
BINARIES=spx_exchange spx_trader spx_auto_trader

main: spx_exchange.c spx_trader.c spx_auto_trader.c spx_common.h spx_exchange.h spx_trader.h
	$(CC) spx_exchange.c -o spx_echange $(CFLAGS)
	$(CC)	spx_trader.c -o spx_trader $(CFLAGS)
	$(CC) spx_auto_trader.c -o spx_auto_trader $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(BINARIES)
