#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include "cmocka.h"

#define TESTING

#include "../spx_exchange.c"

// static int create_orders(void **state) {
//
// }

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
  free(result);
}




int main() {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_read_products_file_valid, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_not_exist, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_missing_lines, NULL, NULL),
    cmocka_unit_test_setup_teardown(test_read_products_file_invalid_length, NULL, NULL),

      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
