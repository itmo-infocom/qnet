all:
	${CC} ${CFLAGS}  -Wall ctapudp.c sha.c keyqueue.c post_curl.c -lmcrypt -lcrypto -lcurl -lz ${LDFLAGS} -o ctapudp
clean:
	rm ctapudp