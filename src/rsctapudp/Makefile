all:
	g++ -std=c++11 -fopenmp -fpermissive -Wall -Wextra -Wl,--no-as-needed ctapudp.c sha.c keyqueue.c post_curl.c thpool.c LeopardCommon.cpp LeopardFF16.cpp LeopardFF8.cpp leopard.cpp -lmcrypt -lcrypto -lcurl -lz -pthread -o ctapudp  -lpthread -msse4.1
clean:
	rm ctapudp