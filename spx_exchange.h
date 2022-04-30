#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"

#define LOG_PREFIX "[SPX]"
#define EXCHANGE_PATH "/tmp/spx_exchange_%d"
#define TRADER_PATH "/tmp/spx_trader_%d"

enum type {
  BUY=0,
  SELL=1,
  AMEND=2,
  CANCEL=3
};

struct order {
  int type;
  int order_id;
  char product[PRODUCT_LENGTH];
  int qty;
  int price;
  int trader_id;
};

struct level {
  int price;
  int num;
  int qty;
  int type;
};

#endif
