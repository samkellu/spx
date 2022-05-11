#! /bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

test_name="invalid_binary"

./spx_exchange "products.txt" "invalid_binary" | tee tests/e2e/$test_name/$test_name.test

diff tests/e2e/$test_name/$test_name.out tests/e2e/$test_name/$test_name.test && echo -e "${GREEN}Test $test_name passed.${NC}" || echo -e "${RED}Test $test_name failed.${NC}"

rm tests/e2e/$test_name/$test_name.test
