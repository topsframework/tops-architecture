#!/usr/bin/env bash

RES="\033[0m"
RED="\033[1;31m"
GREEN="\033[1;32m"

# Compile file
if [ architecture.cpp -nt architecture ];
    then ${CXX:-g++} -std=c++11 architecture.cpp -o architecture || exit 1
fi

# Run tests
if [ -n $(./architecture | diff test.txt -) ];
then
    echo -e "${GREEN}[OK]${RES}"
else
    echo -e "${RED}[ERROR]${RES}"
    ./architecture
    exit 1
fi
