#! /bin/bash
RED='\033[0;31m'
GREEN='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

test_name="two_traders_all_functions"

gcc tests/E2E/$test_name/test_trader1.c -o tests/E2E/$test_name/test_trader1 -Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L -lm
gcc tests/E2E/$test_name/test_trader2.c -o tests/E2E/$test_name/test_trader2 -Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L -lm

./spx_exchange "products.txt" "tests/E2E/$test_name/test_trader1" "tests/E2E/$test_name/test_trader2" | tee tests/E2E/$test_name/$test_name.test

diff tests/E2E/$test_name/$test_name.out tests/E2E/$test_name/$test_name.test && echo -e "${GREEN} $test_name Exchange output correct.${NC}" || echo -e "${RED} $test_name Exchange output incorrect.${NC}"

diff tests/E2E/$test_name/trader1.out tests/E2E/$test_name/trader1.test && echo -e "${GREEN} $test_name Trader1 output correct.${NC}" || echo -e "${RED} $test_name Trader1 output incorrect.${NC}"
diff tests/E2E/$test_name/trader2.out tests/E2E/$test_name/trader2.test && echo -e "${GREEN} $test_name Trader2 output correct.${NC}" || echo -e "${RED} $test_name Trader2 output incorrect.${NC}"

gcov ./spx_exchange

rm tests/E2E/$test_name/$test_name.test
rm tests/E2E/$test_name/trader1.test
rm tests/E2E/$test_name/trader2.test
