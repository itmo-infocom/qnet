all:
	${CC} ${CFLAGS}  -Wall ctapudp.c sha3.c keyqueue.c post_curl.c -lmcrypt -lcrypto -lcurl -lz ${LDFLAGS} -o ctapudp
clean:
	rm ctapudp