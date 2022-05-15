#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include "cmocka.h"

#define TESTING

#include "../spx_exchange.c"

// Reads a valid product file
static void test_read_products_file_valid(void **state) {

  char** result;
  char* expected[PRODUCT_LENGTH] = {
    "6","Burger","Burrito","Sandwich","Pie","Fish","Fruit"
  };

  printf("\nDont know why my testcase messes with the stdout buffer but it does.\
  \nThis doesnt affect the output.\n\n");

  result = read_products_file("tests/products_test.txt");
  for (int i = 0; i < 7; i++) {
    result[i] = strtok(result[i], "\r");
    assert_string_equal(expected[i], result[i]);
    free(result[i]);
  }
  free(result);
}

// Reads a product file that doesnt exist
static void test_read_products_file_not_exist(void **state) {

  char** result;
  result = read_products_file("tests/products_not_exist.txt");
  assert_true(result == NULL);
}

// Reads a product file with missing lines
static void test_read_products_file_missing_lines(void **state) {

  char** result;
  char* expected[PRODUCT_LENGTH] = {
    "6","Burger","Burrito","Sandwich","Pie","Fish","Fruit"
  };

  result = read_products_file("tests/products_test_missing_lines.txt");

  for (int i = 0; i < 7; i++) {
    result[i] = strtok(result[i], "\r");
    assert_string_equal(expected[i], result[i]);
    free(result[i]);
  }
  free(result);
}

// Reads a product file with an invalid length
static void test_read_products_file_invalid_length(void **state) {

  char** result;

  result = read_products_file("tests/products_test_invalid_length.txt");

  assert_true(result == NULL);
}

// Creates a buy order and checks it is valid
static void test_create_buy_valid_no_match(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  struct order** result;
  result = create_order(BUY, products, new_trader, 0, product, 10, 11, &buy_order, orders, traders, 0);

  assert_true(result[0] != NULL);
  assert_true(result[0]->type == BUY);
  assert_true(result[0]->order_id == 0);
  assert_true(result[0]->qty == 10);
  assert_true(result[0]->price == 11);
  assert_true(result[0]->trader == new_trader);
  assert_true(result[0]->time == 0);
  assert_true(strcmp(result[0]->product, "Burger") == 0);

  free(new_trader->position_qty);
  free(new_trader->position_cost);
  free(new_trader);
  free(result[0]->product);
  free(result[0]);
  free(result);
}

// Creates a sell order and checks it is valid
static void test_create_sell_valid_no_match(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  struct order** result;
  result = create_order(SELL, products, new_trader, 0, product, 10, 11, &sell_order, orders, traders, 0);

  assert_true(result[0] != NULL);
  assert_true(result[0]->type == SELL);
  assert_true(result[0]->order_id == 0);
  assert_true(result[0]->qty == 10);
  assert_true(result[0]->price == 11);
  assert_true(result[0]->trader == new_trader);
  assert_true(result[0]->time == 0);
  assert_true(strcmp(result[0]->product, "Burger") == 0);

  free(new_trader->position_qty);
  free(new_trader->position_cost);
  free(new_trader);
  free(result[0]->product);
  free(result[0]);
  free(result);
}

// Tests the validity of a sell order, then matches it with a new buy order
// and checks that it is processed correctly
static void test_match_sell_buy_valid(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct trader* new_trader_2 = malloc(sizeof(struct trader));
  new_trader_2->id = 1;
  new_trader_2->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->active = 1;
  new_trader_2->current_order_id = 0;
  new_trader_2->exchange_fd = 3;
  new_trader_2->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, new_trader_2, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  orders = create_order(SELL, products, new_trader, 0, product, 10, 11, &sell_order, orders, traders, 0);

  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == SELL);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 10);
  assert_true(orders[0]->price == 11);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);

  orders = create_order(BUY, products, new_trader_2, 0, product, 5, 12, &buy_order, orders, traders, 0);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == SELL);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 5);
  assert_true(orders[0]->price == 11);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);
  assert_true(total_fees == 1);
  total_fees = 0;

  int cursor = 0;
  while (traders[cursor] != NULL) {
    free(traders[cursor]->position_qty);
    free(traders[cursor]->position_cost);
    free(traders[cursor++]);
  }

  cursor = 0;
  while (orders[cursor] != NULL) {
    free(orders[cursor]->product);
    free(orders[cursor++]);
  }
  free(orders);
}

// Tests the validity of a buy order, then matches it with a new sell order
// and checks that it is processed correctly
static void test_match_buy_sell_valid(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct trader* new_trader_2 = malloc(sizeof(struct trader));
  new_trader_2->id = 1;
  new_trader_2->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->active = 1;
  new_trader_2->current_order_id = 0;
  new_trader_2->exchange_fd = 3;
  new_trader_2->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, new_trader_2, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  orders = create_order(BUY, products, new_trader_2, 0, product, 10, 12, &buy_order, orders, traders, 0);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 10);
  assert_true(orders[0]->price == 12);
  assert_true(orders[0]->trader == new_trader_2);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);

  orders = create_order(SELL, products, new_trader, 0, product, 5, 11, &sell_order, orders, traders, 0);

  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 5);
  assert_true(orders[0]->price == 12);
  assert_true(orders[0]->trader == new_trader_2);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);
  assert_true(total_fees == 1);
  total_fees = 0;

  int cursor = 0;
  while (traders[cursor] != NULL) {
    free(traders[cursor]->position_qty);
    free(traders[cursor]->position_cost);
    free(traders[cursor++]);
  }

  cursor = 0;
  while (orders[cursor] != NULL) {
    free(orders[cursor]->product);
    free(orders[cursor++]);
  }
  free(orders);
}

// Tests the validity of a buy order, then cancels it and makes sure
// it is cancelled correctly
static void test_cancel_valid(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";
  char product2[PRODUCT_LENGTH] = "Taco";

  orders = create_order(BUY, products, new_trader, 0, product, 10, 12, &buy_order, orders, traders, 0);
  orders = create_order(BUY, products, new_trader, 1, product2, 22, 333, &buy_order, orders, traders, 1);

  assert_true(orders[2] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 10);
  assert_true(orders[0]->price == 12);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);

  assert_true(orders[1] != NULL);
  assert_true(orders[1]->type == BUY);
  assert_true(orders[1]->order_id == 1);
  assert_true(orders[1]->qty == 22);
  assert_true(orders[1]->price == 333);
  assert_true(orders[1]->trader == new_trader);
  assert_true(orders[1]->time == 1);
  assert_true(strcmp(orders[1]->product, "Taco") == 0);

  orders = create_order(CANCEL, products, new_trader, 0, NULL, 0, 0, &cancel_order, orders, traders, 2);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(total_fees == 0);
  total_fees = 0;

  int cursor = 0;
  while (traders[cursor] != NULL) {
    free(traders[cursor]->position_qty);
    free(traders[cursor]->position_cost);
    free(traders[cursor++]);
  }

  cursor = 0;
  while (orders[cursor] != NULL) {
    free(orders[cursor]->product);
    free(orders[cursor++]);
  }
  free(orders);
}

// Tests the validity of a buy order, then amends it and makes sure
// it is amended correctly
static void test_amend_valid(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  orders = create_order(BUY, products, new_trader, 0, product, 10, 12, &buy_order, orders, traders, 0);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 10);
  assert_true(orders[0]->price == 12);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);

  orders = create_order(AMEND, products, new_trader, 0, NULL, 1, 22, &amend_order, orders, traders, 1);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 1);
  assert_true(orders[0]->price == 22);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 1);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);
  assert_true(total_fees == 0);
  total_fees = 0;

  int cursor = 0;
  while (traders[cursor] != NULL) {
    free(traders[cursor]->position_qty);
    free(traders[cursor]->position_cost);
    free(traders[cursor++]);
  }

  cursor = 0;
  while (orders[cursor] != NULL) {
    free(orders[cursor]->product);
    free(orders[cursor++]);
  }
  free(orders);
}

// Tests the validity of a buy order, then tests the validity of a sell order,
// amends the buy order such that the sell order is fulfilled
static void test_amend_match_valid(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 1;
  new_trader->current_order_id = 0;
  new_trader->exchange_fd = 3;
  new_trader->trader_fd = 3;

  struct trader* new_trader_2 = malloc(sizeof(struct trader));
  new_trader_2->id = 1;
  new_trader_2->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader_2->active = 1;
  new_trader_2->current_order_id = 0;
  new_trader_2->exchange_fd = 3;
  new_trader_2->exchange_fd = 3;

  struct order** orders = malloc(sizeof(struct order**));
  orders[0] = NULL;

  struct trader* traders[] = {new_trader, new_trader_2, NULL};
  char* products[] = {"2", "Burger", "Taco"};
  char product[PRODUCT_LENGTH] = "Burger";

  orders = create_order(BUY, products, new_trader, 0, product, 10, 100, &buy_order, orders, traders, 0);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 10);
  assert_true(orders[0]->price == 100);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 0);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);

  orders = create_order(SELL, products, new_trader_2, 0, product, 2, 101, &sell_order, orders, traders, 1);

  assert_true(orders[2] == NULL);
  assert_true(orders[1] != NULL);
  assert_true(orders[1]->type == SELL);
  assert_true(orders[1]->order_id == 0);
  assert_true(orders[1]->qty == 2);
  assert_true(orders[1]->price == 101);
  assert_true(orders[1]->trader == new_trader_2);
  assert_true(orders[1]->time == 1);
  assert_true(strcmp(orders[1]->product, "Burger") == 0);

  orders = create_order(AMEND, products, new_trader, 0, NULL, 3, 101, &amend_order, orders, traders, 2);

  assert_true(orders[1] == NULL);
  assert_true(orders[0] != NULL);
  assert_true(orders[0]->type == BUY);
  assert_true(orders[0]->order_id == 0);
  assert_true(orders[0]->qty == 1);
  assert_true(orders[0]->price == 101);
  assert_true(orders[0]->trader == new_trader);
  assert_true(orders[0]->time == 2);
  assert_true(strcmp(orders[0]->product, "Burger") == 0);
  assert_true(total_fees == 2);
  total_fees = 0;

  int cursor = 0;
  while (traders[cursor] != NULL) {
    free(traders[cursor]->position_qty);
    free(traders[cursor]->position_cost);
    free(traders[cursor++]);
  }

  cursor = 0;
  while (orders[cursor] != NULL) {
    free(orders[cursor]->product);
    free(orders[cursor++]);
  }
  free(orders);
}

int main() {

  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_read_products_file_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_not_exist, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_missing_lines, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_invalid_length, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_create_buy_valid_no_match, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_create_sell_valid_no_match, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_match_sell_buy_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_match_buy_sell_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_cancel_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_amend_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_amend_match_valid, NULL, NULL),
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
