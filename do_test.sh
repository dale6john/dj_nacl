#!/bin/bash

echo "recommended to compile with PARANOID=1, see types.h"
g++ -g test3.cpp -DPARANOID=1 -DVERBOSE=0 -I. view.cpp dj_two.cc rectangle.cpp
./a.out
