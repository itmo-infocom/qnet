#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
#include "sha3.h"
#include "keyqueue.h"
#include <microhttpd.h>
#include <db.h>
#include <libgen.h>
/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000
#define CLIENT 0
#define SERVER 1
#define PORT 55555
#define PORTC 55554
#define NUMBER_OF_THREADS 2
DB *dbhandle; // DB handle
DB_ENV *dbenv = NULL;
char *progname;
volatile int toclose = 0;

volatile int debug = 0;

typedef struct {
    uint8_t key[32];
    unsigned int time;
    uint8_t usage;
} KEY_IN_DB;

KEY_IN_DB *getLastKey(DB *db);
KEY_IN_DB *getByKey(uint8_t *sha, DB *db);
int putKey(KEY *k, DB *db);

struct postStatus {
    bool status;
    char *buff;
    int count;
};
const char * response_data = "OK\0";

static int ahc_echo(void * cls,
        struct MHD_Connection * connection,
        const char * url,
        const char * method,
        const char * version,
        const char * upload_data,
        size_t * upload_data_size,
        void ** ptr) {

    char response_buffer[64];

    DB_TXN *t;
    const char * page = cls;
    struct MHD_Response * response;
    int ret;
    int toclose_local = 0;
    struct postStatus *post = NULL;
    post = (struct postStatus*) *ptr;
    int to_sync = 0;

    if (post == NULL) {
        post = malloc(sizeof (struct postStatus));
        post->status = false;
        post->count = 0;
        *ptr = post;
    }

    if (!post->status) {
        post->status = true;
        return MHD_YES;
    } else {
        if (*upload_data_size != 0) {
            post->buff = malloc(*upload_data_size + 1);
            strncpy(post->buff, upload_data, *upload_data_size);
            post->count += *upload_data_size;
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            if (strncmp(post->buff, "quit", 4) == 0) {
                toclose_local = 1;
                response = MHD_create_response_from_buffer(strlen(response_data),
                        (void*) response_data,
                        MHD_RESPMEM_PERSISTENT);
            } else
                if (strncmp(post->buff, "last", 4) == 0) {
                KEY_IN_DB *lastKey = getLastKey(dbhandle);
                if (lastKey == NULL)
                    return MHD_NO;
                if (debug) {
                    PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                }
                int i;
                for (i = 0; i < 32; i++) {
                    sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                }
                response = MHD_create_response_from_buffer(sizeof (response_buffer),
                        (void *) response_buffer,
                        MHD_RESPMEM_MUST_COPY);
                to_sync = 1;
                //free(lastKey);
            } else
                if (strncmp(post->buff, "key", 3) == 0) {
                //curl --data-ascii key5D56F45ED57EA8DC9CF62322849A36E8D563AE8C7E5A265CFD994213834B73A4 127.0.0.1:55554
                uint8_t sha[32];
                int i;
                for (i = 0; i < 32; i++) {
                    sscanf(post->buff + 3 + i * 2, "%02X", &(sha[i]));
                }
                if (debug) {
                    for (i = 0; i < 32; i++) {
                        printf("%02X", sha[i]);
                    }
                }
                KEY_IN_DB *lastKey = getByKey(sha, dbhandle);
                if (lastKey != NULL) {
                    if (debug) {
                        PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                    }
                    for (i = 0; i < 32; i++) {
                        sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                    }
                    free(lastKey);
                    response = MHD_create_response_from_buffer(sizeof (response_buffer),
                            (void *) response_buffer,
                            MHD_RESPMEM_MUST_COPY);
                } else {
                    response = NULL;
                }
            } else {
                uint8_t key[32];
                int i, j;
                bool isadded = false;
                for (j = 0; j <= post->count - 64; j += 64) {
                    for (i = 0; i < 32; i++) {
                        sscanf((post->buff + j + i * 2), "%02X", &(key[i]));
                    }
                    if (debug) {
                        printf("\n j=%d \n", j);
                    }
                    KEY *k1 = ConstructKey(key);
                    if (debug) {
                        PrintKey(k1);
                    }
                    int ret = putKey(k1, dbhandle);
                    free(k1);
                    if (ret == 0) {
                        isadded = true;
                    }
                }
                if (isadded) {
                    if (debug) {
                        fprintf(stdout, "NEW KEYS\n");
                        fflush(stdout);
                    }
                    response = MHD_create_response_from_buffer(strlen(response_data),
                            (void*) response_data,
                            MHD_RESPMEM_PERSISTENT);
                    to_sync = 1;
                } else {
                    response = NULL;
                }

            }
            free(post->buff);
        }
    }

    if (post != NULL)
        free(post);
    if (to_sync == 1) {
        //dbhandle->sync(dbhandle, 0);
    }
    ret = MHD_queue_response(connection,
            MHD_HTTP_OK,
            response);
    if (response)
        MHD_destroy_response(response);

    if (toclose_local == 1) {
        toclose = 1;
    }
    return ret;
}

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)  *
 **************************************************************************/
void do_debug(char *msg, ...) {
    va_list argp;
    if (debug) {
        va_start(argp, msg);
        vfprintf(stderr, msg, argp);
        va_end(argp);
    }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.  *
 **************************************************************************/
void my_err(char *msg, ...) {
    va_list argp;
    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

/**************************************************************************
 * usage: prints usage and exits.  *
 **************************************************************************/
void usage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -p <port> \n", progname);
    fprintf(stderr, "%s -h\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "-p <port>: port to listen on 55554\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

int putKey(KEY *k, DB *db) {
    DB_TXN *t;
    size_t len;
    DBT db_key, db_data;
    KEY_IN_DB *key_in = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    if (key_in == NULL) {
        return 1;
    }
    uint8_t key_buffer[32];
    memset(&db_key, 0, sizeof (DBT));
    memset(&db_data, 0, sizeof (DBT));
    memcpy(&(key_in->key), &(k->key), sizeof (k->key));
    unsigned int curt = (unsigned int) time(NULL);
    memcpy(&(key_in->time), &curt, sizeof (unsigned int));
    uint8_t was_usage = 0;
    memcpy(&(key_in->usage), &was_usage, sizeof (uint8_t));
    memcpy(&key_buffer, &(k->sha), sizeof (key_buffer));
    db_key.data = &key_buffer;
    db_key.size = sizeof (key_buffer);
    db_data.data = key_in;
    db_data.size = sizeof (uint8_t)*37;
    int ret = 0;
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        ret = db->put(db, NULL, &db_key, &db_data, DB_NOOVERWRITE);
        if (t->commit(t, 0) != 0) {
            printf("ERROR IN COMMIT!!!\n");
        }
    } else {
        if (t != NULL)
            (void)t->abort(t);
        printf("ERROR IN TRANSACTION!!!\n");
        return -1;
    }
    if (ret != 0) {
        my_err("Key ID exists\n");
    }
    free(key_in);
    return ret;
}

KEY_IN_DB *getByKey(uint8_t *sha, DB *db) {
    DB_TXN *t;
    KEY_IN_DB *keyret = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    if (keyret == NULL) {
        return NULL;
    }
    DBT key, data;
    memset(&key, 0, sizeof (key));
    memset(&data, 0, sizeof (data));
    data.flags = DB_DBT_MALLOC;
    key.data = sha;
    key.size = sizeof (uint8_t)*32;
    //data.data = &key_in;
    //data.ulen = sizeof(KEY_IN_DB);
    //data.flags = DB_DBT_USERMEM;
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        int ret = db->get(db, t, &key, &data, 0);

        if (t->commit(t, 0) != 0) {
            printf("ERROR IN COMMIT!!!\n");
        }
        if (ret == 0) {
            memcpy(keyret, data.data, sizeof (KEY_IN_DB));
            free(data.data);
            return keyret;
        } else {
            my_err("NOT FOUND\n");
            return NULL;
        }
    } else {
        if (t != NULL)
            (void)t->abort(t);
        printf("ERROR IN TRANSACTION!!!\n");
        return NULL;
    }
}

KEY_IN_DB *getLastKey(DB *db) {
    DBT key_dbt, data_dbt;
    DBC *dbc;
    DB_TXN *t;
    KEY_IN_DB *k = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    memset(&key_dbt, 0, sizeof (DBT));
    memset(&data_dbt, 0, sizeof (DBT));
    data_dbt.flags = DB_DBT_MALLOC;
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        db->cursor(db, t, &dbc, 0);
        int ret;
        int flag = 0;
        int isok = 0;
        for (ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_LAST);
                ret == 0;
                ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_PREV)) {
            if (((KEY_IN_DB*) (data_dbt.data))->usage != 0) {
                if (flag) {
                    ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_NEXT);
                    memcpy(k, data_dbt.data, sizeof (KEY_IN_DB));
                    isok = 1;
                    break;
                } else {
                    break;
                }
            } else {
                flag = 1;
            }
        }
        if (!isok) {
            ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_NEXT);
            memcpy(k, data_dbt.data, sizeof (KEY_IN_DB));
        }
        if (k->usage < 250) {
            k->usage++;
            ((KEY_IN_DB*) data_dbt.data)->usage = k->usage;
            dbc->put(dbc, &key_dbt, &data_dbt, DB_CURRENT);
        }
        free(data_dbt.data);
        if (t->commit(t, 0) != 0) {
            printf("ERROR IN COMMIT!!!\n");
        }
    } else {
        if (t != NULL)
            (void)t->abort(t);
        printf("ERROR IN TRANSACTION!!!\n");
        return NULL;
    }
    return k;
}

struct MHD_Daemon *my_daemon;

void intHandler(int dummy) {
    printf("\nEXIT\n");
    if (my_daemon) {
        MHD_stop_daemon(my_daemon);
    }
    if (dbhandle) {
        dbhandle->sync(dbhandle, 0);
        dbhandle->close(dbhandle, 0);
        dbenv->close(dbenv, 0);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    fprintf(stdout, "Starting\n");
    fflush(stdout);
    int tap_fd, option;
    int flags = IFF_TUN;
    char if_name[IFNAMSIZ] = "";
    int maxfd;
    uint16_t nread, nwrite, plength;
    char buffer[BUFSIZE];
    struct sockaddr_in local, remote;
    char remote_ip[16] = ""; /* dotted quad IP string */
    unsigned short int port = PORT;
    unsigned int portCtrl = 55554;
    int sock_fd, net_fd, optval = 1;
    socklen_t remotelen;
    int cliserv = -1; /* must be specified on cmd line */
    unsigned long int tap2net = 0, net2tap = 0;
    progname = argv[0];
    uint8_t key[1][32] = {
        {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4}
    };
    u_int32_t envFlags;
    int ret = 0;
    char dbname[255] = "keys.db";

    while ((option = getopt(argc, argv, "p:h:n:d:")) > 0) {
        switch (option) {
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                break;
            case 'p':
                portCtrl = atoi(optarg);
                break;
            case 'n':
                strncpy(dbname, optarg, 255);
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        printf("%s: %s\n", progname, db_strerror(ret));
        return (ret);
    }
    if ((ret =
            dbenv->open(dbenv, dirname(dbname), DB_CREATE | DB_RECOVER | DB_INIT_LOCK |
            DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_THREAD, 0)) != 0) {
        dbenv->err(dbenv, ret, "environment open: %s", dirname(dbname));
        dbenv->close(dbenv, 0);
        return (ret);
    }
    // Initialize our DB handle
    ret = db_create(&dbhandle, dbenv, 0);
    if (debug) {
        fprintf(stdout, "Creating\n");
        fflush(stdout);
    }
    if (ret != 0) {
        fprintf(stdout, "Failed to initialize the database handle: %s\n", db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    if (debug) {
        fprintf(stdout, "Created\n");
        fflush(stdout);
    }
    ret = dbhandle->open(dbhandle, NULL, dbname, NULL, DB_BTREE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to open database file %s: %s\n", dbname, db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    if (debug) {
        fprintf(stdout, "Opened\n");
        fflush(stdout);
    }

    //sleep(1);

    KEY *newkey = ConstructKey(key[0]);

    if (debug) {
        PrintKey(newkey);
    }

    putKey(newkey, dbhandle);

    KEY_IN_DB *retKey = getByKey(newkey->sha, dbhandle);

    free(newkey);

    if (retKey != NULL) {
        if (debug) {
            printf("1111");
        }
        KEY *nextKey = ConstructKeyUsage(retKey->key, retKey->usage);

        if (debug) {
            PrintKey(nextKey);
        }
        //PrintKey(ConstructKeyUsage(nextKey->key, nextKey->usage));
        free(retKey);
    }

    if (debug) {
        KEY_IN_DB *lastKey = getLastKey(dbhandle);
        if (lastKey != NULL) {
            printf("2222");
            PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
        }
    }

    int pass = 1;

    /* Check command line options */

#if EPOLL_SUPPORT
    fprintf("EPOLL\n");
#endif
    fflush(stdout);
    /*argv += optind;
    argc -= optind;
    if (argc > 0) {
        my_err("Too many options!\n");
        usage();
    }*/
    my_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_SUPPRESS_DATE_NO_CLOCK
            | MHD_USE_EPOLL_LINUX_ONLY | MHD_USE_EPOLL_TURBO
            ,
            portCtrl,
            NULL,
            NULL,
            &ahc_echo,
            NULL,
            MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
            MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) NUMBER_OF_THREADS,
            MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 4,
            MHD_OPTION_END);
    if (my_daemon == NULL) {
        my_err("Error in libmicrohttpd\n");
        return 1;
    }
    fflush(stdout);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fileno(stderr), &set);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    while (1) {
        if (select(FD_SETSIZE, NULL, NULL, &set, &timeout) != 1) {
        }
    }
    intHandler(0);
    //DestructQueue(q1);
    //DestructQueue(q2);
    return (0);
}

