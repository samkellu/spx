#! /bin/bash
RED='\033[0;31m'
GREEN='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

test_name="no_args"

./spx_exchange | tee tests/E2E/$test_name/$test_name.test

diff tests/E2E/$test_name/$test_name.out tests/E2E/$test_name/$test_name.test && echo -e "${GREEN}Test $test_name passed.${NC}" || echo -e "${RED}Test $test_name failed.${NC}"

gcov ./spx_exchange

rm tests/E2E/$test_name/$test_name.test
