#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include "cmocka.h"

#define TESTING

#include "../spx_exchange.c"

// To check coverage, gcov ./unit-tests.c

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

static void test_read_products_file_not_exist(void **state) {

  char** result;
  result = read_products_file("tests/products_not_exist.txt");
  assert_true(result == NULL);
}

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

static void test_read_products_file_invalid_length(void **state) {

  char** result;

  result = read_products_file("tests/products_test_invalid_length.txt");

  assert_true(result == NULL);
}

static void test_create_buy_valid_no_match(void **state) {

  struct trader* new_trader = malloc(sizeof(struct trader));
  new_trader->id = 0;
  new_trader->position_qty = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->position_cost = calloc(sizeof(int), sizeof(int) * 2);
  new_trader->active = 0;
  new_trader->current_order_id = 0;

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




int main() {


  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_read_products_file_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_not_exist, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_missing_lines, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_invalid_length, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_create_buy_valid_no_match, NULL, NULL)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
