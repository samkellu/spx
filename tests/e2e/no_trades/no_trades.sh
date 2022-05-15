#! /bin/bash
RED='\033[0;31m'
GREEN='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

test_name="no_trades"

gcc tests/e2e/$test_name/test_trader.c -o tests/e2e/$test_name/test_trader -Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L -lm

./spx_exchange "products.txt" "tests/e2e/$test_name/test_trader" | tee tests/e2e/$test_name/$test_name.test

diff tests/e2e/$test_name/$test_name.out tests/e2e/$test_name/$test_name.test && echo -e "${GREEN}Test $test_name passed.${NC}" || echo -e "${RED}Test $test_name failed.${NC}"

gcov ./spx_exchange

rm tests/e2e/$test_name/$test_name.test
