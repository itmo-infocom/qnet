all:
	rm -rf ./build
	mkdir ./build
	cc -o ./build/ctap aes.c sha3.c keyqueue.c ctap.c -lmicrohttpd
clean:
	rm -rf ./build