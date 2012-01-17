#!/bin/bash

g++ -g test3.cpp -DPARANOID=1 -DVERSION=\"test\" -I. view.cpp dj_two.cc rectangle.cpp
./a.out
