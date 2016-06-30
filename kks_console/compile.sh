#!/bin/bash

g++ -Wall -pthread alice_main.cpp -o build/alice.o2 -std=c++11
g++ -Wall -pthread bob_main.cpp -o build/bob.o -std=c++11
