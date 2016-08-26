g++\
    -g\
    -std=c++11\
    -pthread\
    -c\
    -I./driverAnB/devel\
    bob_main.cpp
ld -r bob_main.o ./driverAnB/devel/libAnB.a -o bob.o
g++ -pthread bob.o -o bob.out
rm *.o
