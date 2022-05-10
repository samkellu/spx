#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

make

for test in tests/e2e/*
do
  test_name=$(echo $test | cut -d / -f3)

  echo "--------------------------------------------------"
  dos2unix $test/$test_name.sh
  echo -e "${NC}Running Test: ${YELLOW}$test${NC}"
  bash $test/$test_name.sh
  echo "--------------------------------------------------"
done
