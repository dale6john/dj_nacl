#!/bin/bash

VERBOSE=0
g++ -Wall -O0 -g test3.cpp -DPARANOID=1 -DVERBOSE=$VERBOSE -DVERSION=\"test\" -I. \
    view.cpp game.cpp rectangle.cpp log.cpp \
 && ./a.out
