#! /bin/bash
RED='\033[0;31m'
GREEN='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

test_name="one_trader_all_functions"

gcc tests/e2e/$test_name/test_trader.c -o tests/e2e/$test_name/test_trader -Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L -lm

./spx_exchange "products.txt" "tests/e2e/$test_name/test_trader" | tee tests/e2e/$test_name/$test_name.test

diff tests/e2e/$test_name/$test_name.out tests/e2e/$test_name/$test_name.test && echo -e "${GREEN} $test_name Exchange output correct.${NC}" || echo -e "${RED} $test_name Exchange output incorrect.${NC}"

diff tests/e2e/$test_name/trader.out tests/e2e/$test_name/trader.test && echo -e "${GREEN} $test_name Trader output correct.${NC}" || echo -e "${RED} $test_name Trader output incorrect.${NC}"

gcov ./spx_exchange

rm tests/e2e/$test_name/$test_name.test
rm tests/e2e/$test_name/trader.test
