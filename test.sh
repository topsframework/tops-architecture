#!/usr/bin/env bash

RES="\033[0m"
RED="\033[1;31m"
GREEN="\033[1;32m"

CXX=${CXX:-g++} 
CFLAGS=${CFLAGS:- -Wall -Wextra -Werror -pedantic -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow } 

# Compile file
if [ test.sh -nt architecture ] || [ architecture.cpp -nt architecture ];
    then valgrind -q ${CXX} -std=c++14 ${CFLAGS} architecture.cpp -o architecture || exit 1
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
