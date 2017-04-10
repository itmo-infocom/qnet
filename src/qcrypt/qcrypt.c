#include <inttypes.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <arpa/inet.h>

#include <microhttpd.h>
#include <libconfig.h>

#define MAXEVENTS 64

#define BUF_SIZE 1024
#define BLOCK_SIZE BUF_SIZE/32

struct key {
    char num[8];
    char key[BUF_SIZE];
    int curpos;
    size_t size;
    struct key *next;
};

config_t cfg;

int fd;
int reread = 0;
static int waitforread = 0;
static int toclose = 0;
char *arg1;
char *arg2;
int mode = 0;
int coder = 0;
char port[50];
char portDest[50];
char portCtrl[50];
char *ip;

struct key *keys = NULL;

struct key *curkey;

int doesFileExist(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}
const char * response_data = "OK";

struct postStatus {
    bool status;
    char *buff;
    int count;
};


static int ahc_echo(void * cls,
            struct MHD_Connection * connection,
            const char * url,
            const char * method,
                    const char * version,
            const char * upload_data,
            size_t * upload_data_size,
                    void ** ptr) {
  const char * page = cls;
  struct MHD_Response * response;
  int ret;
  int toclose_local = 0;
  struct postStatus *post = NULL;
  post = (struct postStatus*)*ptr;

  if(post == NULL) {
    post = malloc(sizeof(struct postStatus));
    post->status = false;
    post->count = 0;
    *ptr = post;
  }

  if(!post->status) {
    post->status = true;
    return MHD_YES;
  } else {
    if(*upload_data_size != 0) {
        post->buff = malloc(*upload_data_size + 1);
        strncpy(post->buff, upload_data, *upload_data_size);
    	post->count += *upload_data_size;
        *upload_data_size = 0;
        return MHD_YES;
    } else {
        if (strncmp(post->buff, "quit", 4) == 0) {
		toclose_local = 1;
        }else if (mode > 0&&strncmp(post->buff, "r", 1) == 0) {
		fprintf(stdout, "REREAD KEYS\n");
		fflush(stdout);
		reread = 1;
        }else{
		if(mode == 0){
			waitforread = 1;
			get_keys_buffer(post->buff,post->count);
			waitforread = 0;
		}
	}
        free(post->buff);
    }
  }

  if(post != NULL)
    free(post);

  response = MHD_create_response_from_buffer (strlen(page),
                                              (void*) page,
                          MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response(connection,
               MHD_HTTP_OK,
               response);
  MHD_destroy_response(response);
  if(toclose_local==1){
	toclose = 1;
  }
  return ret;
}


int read_cfg(char *fname){
        int result = 1;
	config_setting_t *root;	
	
	if(doesFileExist(fname==NULL?"/etc/qcrypt.cfg":fname)==0){
    	fprintf(stdout, "File does not exists in '%s'\n",fname==NULL?"/etc/qcrypt.cfg":fname);
		fflush(stdout);
	    return -1;		
	}	

	config_init(&cfg);
	
	if(!config_read_file(&cfg, (fname==NULL?"/etc/qcrypt.cfg":fname)))
	{
	    fprintf(stdout, "%s:%d - %s\n", config_error_file(&cfg),
		    config_error_line(&cfg), config_error_text(&cfg));
		fflush(stdout);
	    config_destroy(&cfg);
	    return -1;
	}
	
  	root = config_root_setting(&cfg);

	if(config_lookup_int(&cfg, "mode", &mode))
	    printf("Mode: %d\n\n", mode);
	  else{
	    fprintf(stdout, "No 'mode' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }

	if(config_lookup_int(&cfg, "coder", &coder))
	    printf("Coder: %d\n\n", coder);
	  else{
	    fprintf(stdout, "No 'coder' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }
	int tmp = 0;
	if(config_lookup_int(&cfg, "port", &tmp)){
	    printf("port: %d\n\n", tmp);
    	    sprintf( port, "%d", tmp );  
	  }else{
	    fprintf(stdout, "No 'port' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }

	if(config_lookup_int(&cfg, "portDest", &tmp)){
	    printf("portDest: %d\n\n", tmp);
    	    sprintf( portDest, "%d", tmp );  
	  }else{
	    fprintf(stdout, "No 'portDest' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }

	if(config_lookup_int(&cfg, "portCtrl", &tmp)){
	    printf("portCtrl: %d\n\n", tmp);
    	    sprintf( portCtrl, "%d", tmp );  
	  }else{
	    fprintf(stdout, "No 'portCtrl' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }

	if(config_lookup_string(&cfg, "ip", &ip))
	    printf("ip: %s\n\n", ip);
	  else{
	    fprintf(stdout, "No 'ip' setting in configuration file.\n");
		fflush(stdout);
	    result++;
	  }

        if(mode == 1){
		if(config_lookup_string(&cfg, "keyDir", &arg1))
			printf("keyDir: %s\n\n", arg1);
		  else{
			fprintf(stdout, "No 'keyDir' setting in configuration file.\n");
			fflush(stdout);
	    	result++;
		  }

		if(config_lookup_string(&cfg, "keyTail", &arg2))
		    printf("keyTail: %s\n\n", arg2);
		  else{
		    fprintf(stdout, "No 'keyTail' setting in configuration file.\n");
			fflush(stdout);
	    	result++;
		  }
	}else if(mode == 2){
		if(config_lookup_string(&cfg, "keyFile", &arg1))
		    printf("keyFile: %s\n\n", arg1);
		  else{
		    fprintf(stdout, "No 'keyFile' setting in configuration file.\n");
			fflush(stdout);
	    	result++;
		  }	
	}

	return result;
}

struct key *get_key(char *file) {
    FILE *f;
    size_t res;
    struct key *key, *k;

    f = fopen(file, "r");
    if (f == NULL) return (NULL);

    key = malloc(sizeof (struct key));
    if (key == NULL) return (NULL);
    key->curpos = 0;
    res = fread(key->num, 1, 8, f);
    if (res < 8) {
        free(key);
        return (NULL);
    }
    res = fread(key->key, 1, BUF_SIZE, f);
    if (res) {
        key->size = res;
        if (keys == NULL) {
            keys = key;
        } else {
            k = keys;
            while (k->next != keys) {
                k = k->next;
            }
            k->next = key;
        }
        // Circular list
        key->next = keys;
    } else {
        free(key);
        return (NULL);
    }
    return (key);
}

int get_files(char *dir_name, char *tail) {
    DIR *d;
    struct dirent *dir;
    int cnt = 0;
    size_t len;
    struct key *key;
    char fname[BUF_SIZE];

    if (keys != NULL) {
        free(keys);
        keys = NULL;
    }
    d = opendir(dir_name);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            len = strlen(dir->d_name);
            if (strncmp(dir->d_name + len - strlen(tail), tail, len) == 0) {
                strncpy(fname, dir_name, BUF_SIZE);
                strncat(fname, "/", BUF_SIZE);
                strncat(fname, dir->d_name, BUF_SIZE);
                key = get_key(fname);
                if (key) {
                    cnt++;
                }
            }
        }
        closedir(d);
    }
    return (cnt);
}

struct key *get_key_buffer(char *f, int size) {
    size_t res;
    struct key *key, *k;

    if (f == NULL) return (NULL);

    key = malloc(sizeof (struct key));
    if (key == NULL) return (NULL);
    key->curpos = 0;
    memcpy(key->num, f, 8);
    //res = read(f, key->num, 8);
    memcpy(key->key, f, size);
        key->size = size;
        if (keys == NULL) {
            keys = key;
        } else {
            k = keys;
            while (k->next != keys) {
                k = k->next;
            }
            k->next = key;
        }
        // Circular list
        key->next = keys;
    return (key);
}

int get_keys_buffer(char *f, int size) {
    int cnt = 0;
    int all = 0; 
    struct key *key;

    if(size<16){
	return 0;
    }
    if (keys != NULL) {
        free(keys);
        keys = NULL;
    }
    key = get_key_buffer(f,size>BUF_SIZE?BUF_SIZE:size);
    while(key!=NULL){
		print_key(key);
        	cnt++;
		all+=key->size;
		if(all>=size-16){
			return (cnt);
		}
    		key = get_key_buffer(f,(size-all>BUF_SIZE?BUF_SIZE:size-all));
    }
    return (cnt);
}

struct key *key_alloc()
{
  struct key *key, *k;

  key = malloc(sizeof(struct key));
  if (key == NULL) return(key);

  if(keys!=NULL) {
    // Find last key in chain
    k = keys;
    while(k->next != keys) {
      //fprintf(stdout, "key->next=0x%x\n", k->next);
      k=k->next;
    }
    k->next = key;
  } else keys = key;

  key->size = 0;
  // Circular list
  key->next = keys;

  return(key);
}

int get_key_plug(char *file)
{
  char in[BUF_SIZE];
  FILE *f;
  size_t res=0;
  int bit, byte=0, num=0, bcnt=0, cnt=0, ret=0, tmp;
  int i;
  int allkeys=0;
  struct key *key, *k;

    if (keys != NULL) {
        free(keys);
        keys = NULL;
    }

  fprintf(stdout, "file=%s\n", file);
	fflush(stdout);
  f = fopen(file, "r");
  if (f == NULL) return(0);

  key = key_alloc();
  if (key == NULL) return(0);
  key->curpos = 0;
  while(fgets(in, BUF_SIZE, f) != NULL) {
    ret = sscanf(in, "%d       %d", &bit, &tmp);
    if (ret) {
      if (bcnt < 8) {
	byte |= bit << bcnt; bcnt++;
      }
      else {
	// Next byte
	key->key[cnt] = byte;
	key->size = cnt++;
	bcnt = 0;
	byte = bit;
	// Next key
	if (cnt > BUF_SIZE) {
	  res += cnt;
  	  memcpy(key->num,key->key,8);
          key->curpos = 0;
	  cnt = 0;
	  print_key(key);
	  allkeys++;
	  key = key_alloc();
	  if (key == NULL) return(0);
	}
      }
    }
  }
  res += cnt;
  return allkeys;
}

void print_key(struct key * key) {
    int i;

    fprintf(stdout, "%d ", key->size);
    for (i = 0; i < 8; i++)
        fprintf(stdout, "%x ", key->num[i]);
    fprintf(stdout, "%d", key->curpos);
    fprintf(stdout, "\n");
	fflush(stdout);
}

size_t crypt(char *data, int length, int socket) {
    char out[BUF_SIZE];
    size_t readsize;
    int count = 0;
    int i; 
    
    while (count < length) {
        if (toclose == 1) {
            break;
        } else {
            if (reread || curkey == NULL) {
                fprintf(stdout, "REREAD KEYS\n");
				fflush(stdout);
                reread = 0;
		while(mode == 0&&waitforread == 1){
		}
    		if(mode == 1){
            		get_files(arg1, arg2);
		}else if(mode == 2){
	    		get_key_plug(arg1);
		}else{
			if(keys==NULL){				
  				fprintf(stdout, "NO KEYS\n");
				fflush(stdout);
				return 0;
			}
		}
                curkey = keys;
            }
        }
        readsize = ((length - count < BLOCK_SIZE) ? (length - count) : BLOCK_SIZE);
        if (readsize == 0) return (0);

        if (curkey->curpos + readsize >= curkey->size) {
            curkey->curpos = 0;
            curkey = curkey->next;
            curkey->curpos = 0;
        }

        for (i = count; i < readsize + count; i++) {
            out[i - count] = data[i] ^ curkey->key[i + curkey->curpos - count];
        }
        write(socket, curkey->num, 8);
        write(socket, &(curkey->curpos), sizeof (int));
        write(socket, &(readsize), sizeof (int));
        curkey->curpos += readsize;
        write(socket, out, readsize);
        count += readsize;
    }
    return (readsize);
}

int read_block(int stream, char *arr, int size, int toret) {
    int ready = 0;
    int nbytes;
    char ar[BLOCK_SIZE];
    int i;
    while (ready < size) {
        nbytes = read(stream, ar, size - ready);
        if (nbytes == -1) {
            if (toret == 1)
                return -1;
            /* If errno == EAGAIN, that means we have read all
               data. So go back to the main loop. */
            if (errno != EAGAIN) {
                return -1;
            }
        } else if (nbytes == 0) {
            return 0;
        } else //new data was delivered
        {
            for (i = ready; i < nbytes + ready; i++) {
                arr[i] = ar[i - ready];
            }
            ready += nbytes;
        }
    }
    return ready;
}

size_t decrypt(int inpsocket, int outsocket) {
    char out[BUF_SIZE];
    char in[BUF_SIZE];
    int size;
    int i;
    int flagToReload = 0;
    
    size = read_block(inpsocket, in, 8, 1);
    
    if (size < 8) return (size);
    if (curkey == NULL || strncmp(in, curkey->num, 8) != 0) {
        flagToReload = 1;
    }

    while (flagToReload == 1) {
        for (curkey = keys;; curkey = curkey->next) {
            if (curkey == NULL) {
                break;
            }
            if (strncmp(in, curkey->num, 8) == 0) {
                flagToReload = 0;
                break;
            }
            if (curkey->next == keys) {
                break;
            }
        }
        if (flagToReload == 1) {
    		if(mode == 1){
            		get_files(arg1, arg2);
		}else if(mode == 2){
	    		get_key_plug(arg1);
		}else{
			
		}
        }
    }
    while(mode == 0&&waitforread == 1){
    }
    read_block(inpsocket, &(curkey->curpos), sizeof (int), 0);
    read_block(inpsocket, &(size), sizeof (int), 0);
    read_block(inpsocket, in, size, 0);

    
    for (i = 0; i < size; i++) {
        out[i] = in[i] ^ curkey->key[i + curkey->curpos];
    }

    write(outsocket, out, size);
    return (size);
}

static int make_socket_non_blocking(int sfd) {
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

static int create_and_bind(char *portInp) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd; 

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE; /* All interfaces */

    s = getaddrinfo(NULL, portInp, &hints, &result);
    if (s != 0) {
        fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(s));
		fflush(stdout);
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            /* We managed to bind successfully! */
            break;
        }

        close(sfd);
    }

    if (rp == NULL) {
        fprintf(stdout, "Could not bind\n");
		fflush(stdout);
        return -1;
    }

    freeaddrinfo(result);

    return sfd;
}


int
main(int argc, char *argv[]) {
    struct MHD_Daemon *daemon;
    struct sockaddr_in server, controlserver;
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    int result = 0;
    if (argc == 1) {
    	result = read_cfg(NULL);
    }else if(argc==2){
	if((strncmp(argv[1], "h", 1) == 0 && strlen(argv[1]) == 1) || (strncmp(argv[1], "-h", 2) == 0 && strlen(argv[1]) == 2) || (strncmp(argv[1], "-help", 5) == 0 && strlen(argv[1]) == 5)){
	    	fprintf(stdout, "\nCoder/Decoder for QNET\n\n");
	    	fprintf(stdout, "Project GIT:\n https://github.com/itmo-infocom/qnet\n");
	    	fprintf(stdout, "Sample Coder cfg:\n https://github.com/itmo-infocom/qnet/blob/master/src/qcrypt/coder.cfg\n");
	    	fprintf(stdout, "Sample Decoder cfg:\n https://github.com/itmo-infocom/qnet/blob/master/src/qcrypt/decoder.cfg\n\n");
	    	fprintf(stdout, "Usage with custom cfg: %s [file]\n", argv[0]);
	    	fprintf(stdout, "Usage with cfg in /etc/qcrypt.cfg: %s\n\n", argv[0]);
			fflush(stdout);
    		return EXIT_SUCCESS;
	}else{
    		result = read_cfg(argv[1]);
	}
    }
    if(result <= 0){
    	fprintf(stdout, "Usage with custom cfg: %s [file]\n", argv[0]);
    	fprintf(stdout, "Usage with cfg in /etc/qcrypt.cfg: %s\n", argv[0]);
		fflush(stdout);
	exit(EXIT_FAILURE);	
    }else if(result>1){
    	fprintf(stdout, "Your config has %d errors.\n", result-1);
		fflush(stdout);
	exit(EXIT_FAILURE);
    }

    if(mode == 0){
	    /*if (argc != 6) {
		fprintf(stdout, "Usage: %s [port] [connectip] [connectport] [controlport] [coder/decoder]\n", argv[0]);
		exit(EXIT_FAILURE);
	    }*/
    }else if(mode == 1){
	    result = get_files(arg1, arg2);
	    if (result){
		fprintf(stdout, "%d keys\n", result);
		fflush(stdout);
	    }else {
		fprintf(stdout, "Keys not found\n", result);
		fflush(stdout);
		exit(-2);
	    }
    }else if(mode == 2){
	    result = get_key_plug(arg1);
	    if (result){
		fprintf(stdout, "%d keys\n", result);
		fflush(stdout);
	    }else {
		fprintf(stdout, "Keys not found\n", result);
		fflush(stdout);
		exit(-2);
	    }
    }


    if (coder == 1) {
        fprintf(stdout,"CODER\n");
		fflush(stdout);
    } else {
        fprintf(stdout,"DECODER\n");
		fflush(stdout);
    }


    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
               atoi(portCtrl),
               NULL,
               NULL,
               &ahc_echo,
               response_data,
               MHD_OPTION_END);

    if (NULL == daemon) return 1;

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(portDest));

    sfd = create_and_bind(port);
    if (sfd == -1)
        abort();

    s = make_socket_non_blocking(sfd);
    if (s == -1)
        abort();

    s = listen(sfd, SOMAXCONN);
    if (s == -1) {
        perror("listen");
        abort();
    }

    efd = epoll_create1(0);
    if (efd == -1) {
        perror("epoll_create");
        abort();
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1) {
        perror("epoll_ctl");
        abort();
    }
    /* Buffer where events are returned */
    events = calloc(MAXEVENTS, sizeof event);

    /* The event loop */
    while (1) {
        if (toclose == 1) {
	    fprintf(stdout,"CLOSE\n");
	    sleep(1);
	    break;
        }
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, 100);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) &&
                    !(events[i].events & EPOLLOUT))) {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                fprintf(stdout, "epoll error\n");
				fflush(stdout);
                close(events[i].data.fd);
                continue;            
            } else if (sfd == events[i].data.fd) {
                /* We have a notification on the listening socket, which
                   means one or more incoming connections. */
                while (1) {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof in_addr;
                    infd = accept(sfd, &in_addr, &in_len);
                    if (infd == -1) {
                        if ((errno == EAGAIN) ||
                                (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming
                               connections. */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    s = getnameinfo(&in_addr, in_len,
                            hbuf, sizeof hbuf,
                            sbuf, sizeof sbuf,
                            NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0) {
                        fprintf(stdout,"Accepted connection on descriptor %d "
                                "(host=%s, port=%s)\n", infd, hbuf, sbuf);
						fflush(stdout);
                    }

                    /* Make the incoming socket non-blocking and add it to the
                       list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if (s == -1) {
                        fprintf(stdout,"Could not make nonblock forward\n");
						fflush(stdout);
                        abort();
                    }

                    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_desc == -1) {
                        fprintf(stdout,"Could not create socket\n");
						fflush(stdout);
                    }

                    if (connect(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
                        fprintf(stdout,"Could not connect socket\n");                        
                        fprintf(stdout,"%s:%d\n",inet_ntoa(server.sin_addr),ntohs(server.sin_port));
						fflush(stdout);
                        return 1;
                    }

                    s = make_socket_non_blocking(socket_desc);
                    if (s == -1) {
                        fprintf(stdout,"Could not make nonblock backward\n");
						fflush(stdout);
                        abort();
                    }
                    uint64_t val = infd;
                    val = ((val << (sizeof (int)*8)) | socket_desc);
                    event.data.u64 = val;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                    val = socket_desc;
                    val = (((-val) << (sizeof (int)*8)) | infd);
                    event.data.u64 = val;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, socket_desc, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            } else {
                int done = 0;
                int inp, outp;

                inp = events[i].data.u64 >> (sizeof (int)*8);
                outp = events[i].data.u64 & UINT32_MAX;
                int direct = 0;
                if (inp < 0) {
                    direct = 1;
                    inp = -inp;
                } else {
                    direct = 0;
                }

                while (1) {
                    int count;
                    char buf[BUF_SIZE];
                    if (coder == 1) {
                        if (direct == 0) {
                            count = read(inp, buf, sizeof buf);
                            if (count > 0) {
                                crypt(buf, count, outp);
                            }
                        } else {
                            count = decrypt(inp, outp);
                        }
                    } else {
                        if (direct == 1) {
                            count = read(inp, buf, sizeof buf);
                            if (count > 0) {
                                crypt(buf, count, outp);
                            }
                        } else {
                            count = decrypt(inp, outp);
                        }
                    }
                    if (count == -1) {
                        /* If errno == EAGAIN, that means we have read all
                           data. So go back to the main loop. */
                        if (errno != EAGAIN) {
                            perror("read");
                            done = 1;
                        }
                        break;
                    } else if (count == 0) {
                        /* End of file. The remote has closed the
                           connection. */
                        done = 1;
                        break;
                    }
                }

                if (done) {
                    if (coder == 1) {
                        fprintf(stdout,"Closed connection on descriptor %d\n",
                                outp);
                        close(outp);
                        fprintf(stdout,"Closed connection on descriptor %d\n",
                                inp);
                        close(inp);
						fflush(stdout);
                    } else {
                        fprintf(stdout,"Closed connection on descriptor %d\n",
                                inp);
                        close(inp);
                        fprintf(stdout,"Closed connection on descriptor %d\n",
                                outp);
                        close(outp);
						fflush(stdout);
                    }
                }
            }
        }
    }

    MHD_stop_daemon (daemon);
    free(events);
    close(sfd);
    config_destroy(&cfg);
    return EXIT_SUCCESS;
}
