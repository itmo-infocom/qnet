#!/bin/bash

g++ -c -std=c++11 alice_main.cpp
g++ -pthread alice_main.o -o ralice


g++ -c -g -std=c++11 bob_main.cpp
g++ -pthread -g bob_main.o -o rbob

rm *.o
#g++ -std=c++11 -Wall -lpthread alice_main.cpp -o build/alice.o 
#g++ -Wall -pthread bob_main.cpp -o build/bob.o -std=c++11
