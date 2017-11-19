/**************************************************************************
 * simpletun.c *
 * *
 * A simplistic, simple-minded, naive tunnelling program using tun/tap *
 * interfaces and TCP. DO NOT USE THIS PROGRAM FOR SERIOUS PURPOSES.  *
 * *
 * You have been warned.  *
 * *
 * (C) 2010 Davide Brini.  *
 * *
 * DISCLAIMER AND WARNING: this is all work in progress. The code is *
 * ugly, the algorithms are naive, error checking and input validation *
 * are very basic, and of course there can be bugs. If that's not enough, *
 * the program has not been thoroughly tested, so it might even fail at *
 * the few simple things it should be supposed to do right.  *
 * Needless to say, I take no responsibility whatsoever for what the *
 * program might do. The program has been written mostly for learning *
 * purposes, and can be used in the hope that is useful, but everything *
 * is to be taken "as is" and without any kind of warranty, implicit or *
 * explicit. See the file LICENSE for further details.  *
 *************************************************************************/
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <net/if.h> 
#include <linux/if_tun.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <arpa/inet.h> 
#include <sys/select.h> 
#include <sys/time.h> 
#include <errno.h> 
#include <stdarg.h>
#include "aes.h"
/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000 
#define CLIENT 0 
#define SERVER 1 
#define PORT 55555
int debug; char *progname; /**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller *
 * must reserve enough space in *dev.  *
 **************************************************************************/ int tun_alloc(char *dev, int flags) {
  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";
  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = flags;
  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }
  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}
void print_hex(BYTE str[], int len)
{
	int idx;

	for(idx = 0; idx < len; idx++)
		printf("%02x", str[idx]);
}
int aes_cbc_test()
{
	WORD key_schedule[60];
	BYTE enc_buf[128];
	BYTE plaintext[1][32] = {
		{0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51}
	};
	BYTE ciphertext[1][32] = {
		{0xf5,0x8c,0x4c,0x04,0xd6,0xe5,0xf1,0xba,0x77,0x9e,0xab,0xfb,0x5f,0x7b,0xfb,0xd6,0x9c,0xfc,0x4e,0x96,0x7e,0xdb,0x80,0x8d,0x67,0x9f,0x77,0x7b,0xc6,0x70,0x2c,0x7d}
	};
	BYTE iv[1][16] = {
		{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}
	};
	BYTE key[1][32] = {
		{0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4}
	};
	int pass = 1;

	printf("* CBC mode:\n");
	aes_key_setup(key[0], key_schedule, 256);

	printf(  "Key          : ");
	print_hex(key[0], 32);
	printf("\nIV           : ");
	print_hex(iv[0], 16);

	aes_encrypt_cbc(plaintext[0], 32, enc_buf, key_schedule, 256, iv[0]);
	printf("\nPlaintext    : ");
	print_hex(plaintext[0], 32);
	printf("\n-encrypted to: ");
	print_hex(enc_buf, 32);
	printf("\nCiphertext   : ");
	print_hex(ciphertext[0], 32);
	pass = pass && !memcmp(enc_buf, ciphertext[0], 32);

	aes_decrypt_cbc(ciphertext[0], 32, enc_buf, key_schedule, 256, iv[0]);
	printf("\nCiphertext   : ");
	print_hex(ciphertext[0], 32);
	printf("\n-decrypted to: ");
	print_hex(enc_buf, 32);
	printf("\nPlaintext   : ");
	print_hex(plaintext[0], 32);
	pass = pass && !memcmp(enc_buf, plaintext[0], 32);

	printf("\n\n");
	return(pass);
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is *
 * returned.  *
 **************************************************************************/ int cread(int fd, char *buf, int n){
  
  int nread;
  if((nread=read(fd, buf, n)) < 0){
    perror("Reading data");
    exit(1);
  }
  return nread;
}
/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is *
 * returned.  *
 **************************************************************************/ int cwrite(int fd, char *buf, int n){
  
  int nwrite;
  if((nwrite=write(fd, buf, n)) < 0){
    perror("Writing data");
    exit(1);
  }
  return nwrite;
}
/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts them into "buf".  *
 * (unless EOF, of course) *
 **************************************************************************/ int read_n(int fd, char *buf, int n) {
  int nread, left = n;
  while(left > 0) {
    if ((nread = cread(fd, buf, left)) == 0){
      return 0 ;
    }else {
      left -= nread;
      buf += nread;
    }
  }
  return n;
}
/**************************************************************************
 * do_debug: prints debugging stuff (doh!)  *
 **************************************************************************/ void do_debug(char *msg, ...){
  
  va_list argp;
  
  if(debug) {
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
  }
}
/**************************************************************************
 * my_err: prints custom error messages on stderr.  *
 **************************************************************************/ void my_err(char *msg, ...) {
  va_list argp;
  
  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}
/**************************************************************************
 * usage: prints usage and exits.  *
 **************************************************************************/ void usage(void) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
  fprintf(stderr, "%s -h\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
  fprintf(stderr, "-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)\n");
  fprintf(stderr, "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555\n");
  fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
  fprintf(stderr, "-d: outputs debug information while running\n");
  fprintf(stderr, "-h: prints this help text\n");
  exit(1);
}
int main(int argc, char *argv[]) {
  
  int tap_fd, option;
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  int maxfd;
  uint16_t nread, nwrite, plength;
  char buffer[BUFSIZE];
  struct sockaddr_in local, remote;
  char remote_ip[16] = ""; /* dotted quad IP string */
  unsigned short int port = PORT;
  int sock_fd, net_fd, optval = 1;
  socklen_t remotelen;
  int cliserv = -1; /* must be specified on cmd line */
  unsigned long int tap2net = 0, net2tap = 0;
  progname = argv[0];
  	WORD key_schedule[60];
	BYTE enc_buf[128];
	BYTE plaintext[1][32] = {
		{0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51}
	};
	BYTE ciphertext[1][32] = {
		{0xf5,0x8c,0x4c,0x04,0xd6,0xe5,0xf1,0xba,0x77,0x9e,0xab,0xfb,0x5f,0x7b,0xfb,0xd6,0x9c,0xfc,0x4e,0x96,0x7e,0xdb,0x80,0x8d,0x67,0x9f,0x77,0x7b,0xc6,0x70,0x2c,0x7d}
	};
	BYTE iv[1][16] = {
		{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}
	};
	BYTE key[1][32] = {
		{0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4}
	};
	int pass = 1;

  //aes_cbc_test();
  /* Check command line options */
  while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
    switch(option) {
      case 'd':
        debug = 1;
        break;
      case 'h':
        usage();
        break;
      case 'i':
        strncpy(if_name,optarg, IFNAMSIZ-1);
        break;
      case 's':
        cliserv = SERVER;
        break;
      case 'c':
        cliserv = CLIENT;
        strncpy(remote_ip,optarg,15);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'u':
        flags = IFF_TUN;
        break;
      case 'a':
        flags = IFF_TAP;
        break;
      default:
        my_err("Unknown option %c\n", option);
        usage();
    }
  }
  argv += optind;
  argc -= optind;
  if(argc > 0) {
    my_err("Too many options!\n");
    usage();
  }
  if(*if_name == '\0') {
    my_err("Must specify interface name!\n");
    usage();
  } else if(cliserv < 0) {
    my_err("Must specify client or server mode!\n");
    usage();
  } else if((cliserv == CLIENT)&&(*remote_ip == '\0')) {
    my_err("Must specify server address!\n");
    usage();
  }
  /* initialize tun/tap interface */
  if ( (tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }
  do_debug("Successfully connected to interface %s\n", if_name);
  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }
  if(cliserv == CLIENT) {
    /* Client, try to connect to server */
    /* assign the destination address */
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip);
    remote.sin_port = htons(port);
    /* connection request */
    if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
      perror("connect()");
      exit(1);
    }
    net_fd = sock_fd;
    do_debug("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
    
  } else {
    /* Server, wait for connections */
    /* avoid EADDRINUSE error on bind() */
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("setsockopt()");
      exit(1);
    }
    
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
      perror("bind()");
      exit(1);
    }
    
    if (listen(sock_fd, 5) < 0) {
      perror("listen()");
      exit(1);
    }
    
    /* wait for connection request */
    remotelen = sizeof(remote);
    memset(&remote, 0, remotelen);
    if ((net_fd = accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) < 0) {
      perror("accept()");
      exit(1);
    }
    do_debug("SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));
  }
  
  /* use select() to handle two descriptors at once */
  maxfd = (tap_fd > net_fd)?tap_fd:net_fd;
  while(1) {
    int ret;
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tap_fd, &rd_set); FD_SET(net_fd, &rd_set);
    ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
    if (ret < 0 && errno == EINTR){
      continue;
    }
    if (ret < 0) {
      perror("select()");
      exit(1);
    }
    if(FD_ISSET(tap_fd, &rd_set)) {
      /* data from tun/tap: just read it and write it to the network */
      nread = cread(tap_fd, buffer, BUFSIZE);
      tap2net++;
      do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);
      /* write length + packet */
      plength = htons(nread);
      int diff = nread%128;
      if(diff!=0){
         diff = nread+(128-diff);
      }else{
         diff = nread;
      }
	aes_key_setup(key[0], key_schedule, 256);

      aes_encrypt_cbc(buffer, diff/8, buffer, key_schedule, 256, iv[0]);
      nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));
      nwrite = cwrite(net_fd, buffer, diff);
      do_debug("TAP2NET %lu: Written %d bytes to the network\n", tap2net, nwrite);
    }
    if(FD_ISSET(net_fd, &rd_set)) {
      /* data from the network: read it, and write it to the tun/tap interface.
       * We need to read the length first, and then the packet */
      /* Read length */
      nread = read_n(net_fd, (char *)&plength, sizeof(plength));
      if(nread == 0) {
        /* ctrl-c at the other end */
        break;
      }
      net2tap++;
      /* read packet */
      plength = ntohs(plength);
      int diff = plength%128;
      if(diff!=0){
         diff = plength+(128-diff);
      }else{
         diff = plength;
      }
      nread = read_n(net_fd, buffer, diff);
	aes_key_setup(key[0], key_schedule, 256);

      aes_decrypt_cbc(buffer, diff/8, buffer, key_schedule, 256, iv[0]);
      do_debug("NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);
      /* now buffer[] contains a full packet or frame, write it into the tun/tap interface */
      nwrite = cwrite(tap_fd, buffer, plength);
      do_debug("NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, nwrite);
    }
  }
  
  return(0);
}
