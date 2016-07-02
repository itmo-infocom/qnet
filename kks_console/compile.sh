#!/bin/bash

#mkdir build
g++ -Wall -std=c++1y -pthread -l AnB -L ./driverAnB/devel/ alice_main.cpp -o build/alice.o
g++ -Wall -std=c++1y -pthread -l AnB -L ./driverAnB/devel/ bob_main.cpp -o build/bob
