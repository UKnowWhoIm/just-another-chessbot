#!/bin/sh
set -e
g++ -g -pthread -std=c++20 /app/engines/test.cpp -o /app/engines/test.out
/app/engines/test.out
