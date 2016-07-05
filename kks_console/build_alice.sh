g++ -g -std=c++11 -pthread -c -I./driverAnB/devel alice_main.cpp
ld -r alice_main.o ./driverAnB/devel/libAnB.a -o alice.o
g++ -pthread alice.o -o alice.out
rm alice_main.o
