#!/usr/bin/env bash

clang++ \
    -std=c++20 \
    -Wall -Wextra -Wpedantic -Werror \
    -g \
    save-tool.cc -o save-tool

clang++ \
    -std=c++20 \
    -Wall -Wextra -Wpedantic -Werror \
    -g \
    gift-tool.cc -o gift-tool
