#!/bin/bash

g++ alice_main.cpp -o -pthread build/alice -std=c++11
g++ bob_main.cpp -o -pthread build/bob -std=c++11
