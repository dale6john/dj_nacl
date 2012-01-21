#!/bin/bash

g++ -g test3.cpp -DPARANOID=1 -DVERBOSE=0 -DVERSION=\"test\" -I. view.cpp game.cpp rectangle.cpp \
 && ./a.out
