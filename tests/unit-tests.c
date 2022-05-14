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

static void test_read_products_file(void **state) {

  char** result;
  char* expected[PRODUCT_LENGTH] = {
    "6","Burger","Burrito","Sandwich","Pie","Fish","Fruit"
  };
  result = read_products_file("tests/products_test.txt");
  for (int i = 0; i < 7; i++) {
    result[i] = strtok(result[i], "\r");
    assert_string_equal(expected[i], result[i]);
    free(result[i]);
  }
  free(result);
}




int main() {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_read_products_file, NULL, NULL)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
