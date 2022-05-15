#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

make e2etesting

for test in tests/E2E/*
do
  test_name=$(echo $test | cut -d / -f3)

  echo "--------------------------------------------------"
  dos2unix $test/$test_name.sh
  echo -e "${NC}Running Test: ${YELLOW}$test${NC}"
  bash $test/$test_name.sh
  echo "--------------------------------------------------"
done

make clean
make testing

tests/unit-tests

gcov ./unit-tests.c

make clean
make
