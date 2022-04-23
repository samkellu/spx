#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"

#define LOG_PREFIX "[SPX]"

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

#endif
