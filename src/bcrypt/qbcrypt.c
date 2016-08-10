#include <inttypes.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <arpa/inet.h>

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

int fd;
int reread = 0;
int toclose = 0;
char *arg1;
char *arg2;

struct key *keys = NULL;

struct key *curkey;

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

void print_key(struct key * key) {
    int i;

    fprintf(stdout, "%d ", key->size + 8);
    for (i = 0; i < 8; i++)
        fprintf(stdout, "%x ", key->num[i]);
    fprintf(stdout, "%d", key->curpos);
    fprintf(stdout, "\n");
}

size_t crypt(char *data, int length, int socket) {
    char out[BUF_SIZE];
    size_t readsize;
    int count = 0;
    
    while (count < length) {
        if (toclose) {
            break;
        } else {
            if (reread || curkey == NULL) {
                fprintf(stdout, "REREAD KEYS\n");
                reread = 0;
                get_files(arg1, arg2);
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

        for (int i = count; i < readsize + count; i++) {
            out[i - count] = data[i] ^ curkey->key[i + curkey->curpos];
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
            for (int i = ready; i < nbytes + ready; i++) {
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
            get_files(arg1, arg2);
        }
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

static int
make_socket_non_blocking(int sfd) {
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

static int
create_and_bind(char *port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE; /* All interfaces */

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
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
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(result);

    return sfd;
}

int coder = 0;

int
main(int argc, char *argv[]) {
    struct sockaddr_in server, controlserver;
    int sfd, s, controlsfd;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    int result;

    if (argc != 8) {
        fprintf(stderr, "Usage: %s [port] [connectip] [connectport] [controlport] [coder/decoder] [keydir] [keytail]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    arg1 = malloc(strlen(argv[6]) + 1);
    strcpy(arg1, argv[6]);
    arg2 = malloc(strlen(argv[7]) + 1);
    strcpy(arg2, argv[7]);
    result = get_files(arg1, arg2);
    if (result)
        fprintf(stderr, "%d keys\n", result);
    else {
        fprintf(stderr, "Keys not found\n", result);
        exit(-2);
    }


    if (atoi(argv[5]) == 1) {
        coder = 1;
        printf("CODER\n");
    } else {
        coder = 0;
        printf("DECODER\n");
    }

    fflush(stdout);

    server.sin_addr.s_addr = inet_addr(argv[2]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[3]));

    controlsfd = create_and_bind(argv[4]);
    if (controlsfd == -1)
        abort();

    s = make_socket_non_blocking(controlsfd);
    if (s == -1)
        abort();

    s = listen(controlsfd, SOMAXCONN);
    if (s == -1) {
        perror("listen");
        abort();
    }

    sfd = create_and_bind(argv[1]);
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
    event.data.u64 = controlsfd;
    event.data.u64 = (event.data.u64 << (sizeof (int)*8));
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, controlsfd, &event);
    if (s == -1) {
        perror("epoll_ctl");
        abort();
    }

    /* Buffer where events are returned */
    events = calloc(MAXEVENTS, sizeof event);

    /* The event loop */
    while (1) {
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) &&
                    !(events[i].events & EPOLLOUT))) {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (controlsfd == (events[i].data.u64 >> (sizeof (int)*8))) {
                if (events[i].data.fd == 0) {
                    /* We have a notification on the listening socket, which
                       means one or more incoming connections. */
                    while (1) {
                        struct sockaddr in_addr;
                        socklen_t in_len;
                        int infd;
                        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                        in_len = sizeof in_addr;
                        infd = accept(controlsfd, &in_addr, &in_len);
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
                            printf("Accepted control connection on descriptor %d "
                                    "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                        }

                        /* Make the incoming socket non-blocking and add it to the
                           list of fds to monitor. */
                        s = make_socket_non_blocking(infd);
                        if (s == -1) {
                            printf("Could not make nonblock forward");
                            abort();
                        }

                        event.data.u64 = controlsfd;
                        event.data.u64 = (event.data.u64 << (sizeof (int)*8) | infd);

                        event.events = EPOLLIN | EPOLLET;
                        s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1) {
                            perror("epoll_ctl");
                            abort();
                        }
                    }
                    continue;
                } else {
                    int done = 0;
                    int count = 0;
                    int inp;
                    char buf[BLOCK_SIZE];
                    while (1) {
                        inp = events[i].data.u64 & UINT32_MAX;
                        count = read(inp, buf, sizeof buf);
                        if (count == -1) {
                            if (errno != EAGAIN) {
                                perror("read");
                                done = 1;
                            }
                            break;
                        } else if (count == 0) {
                            done = 1;
                            break;
                        }
                        if (strncmp(buf, "r", 1) == 0) {
                            fprintf(stderr, "REREAD KEYS\n");
                            reread = 1;
                            break;
                        } else
                            if (strncmp(buf, "q", 1) == 0) {
                            fprintf(stderr, "EXIT\n");
                            free(events);
                            close(sfd);
                            return EXIT_SUCCESS;
                        }
                    }

                    if (done) {
                        printf("Closed control connection on descriptor %d\n",
                                inp);
                        close(inp);
                    }
                }
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
                        printf("Accepted connection on descriptor %d "
                                "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                    /* Make the incoming socket non-blocking and add it to the
                       list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if (s == -1) {
                        printf("Could not make nonblock forward");
                        abort();
                    }

                    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_desc == -1) {
                        printf("Could not create socket");
                    }

                    if (connect(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
                        printf("Could not connect socket");
                        return 1;
                    }

                    s = make_socket_non_blocking(socket_desc);
                    if (s == -1) {
                        printf("Could not make nonblock backward");
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
                    char buf[BLOCK_SIZE];
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
                        printf("Closed connection on descriptor %d\n",
                                outp);
                        close(outp);
                        printf("Closed connection on descriptor %d\n",
                                inp);
                        close(inp);
                    } else {
                        printf("Closed connection on descriptor %d\n",
                                inp);
                        close(inp);
                        printf("Closed connection on descriptor %d\n",
                                outp);
                        close(outp);
                    }
                }
            }
        }
    }

    free(events);
    close(sfd);
    return EXIT_SUCCESS;
}
