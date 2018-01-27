#include <stdio.h> 
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
/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000 
#define CLIENT 0 
#define SERVER 1 
#define PORT 55555
#define PORTC 55554
#define DATABASE "keys.db"

DB *dbhandle; // DB handle
int debug;
char *progname;
int toclose = 0;

char response_buffer[64];

const char * response_data = "OK\0";

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
    post = (struct postStatus*) *ptr;

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
                response_buffer[0] = 'O';
                response_buffer[1] = 'K';
                response = MHD_create_response_from_buffer(2,
                        (void*) response_buffer,
                        MHD_RESPMEM_PERSISTENT);
            } else
                if (strncmp(post->buff, "last", 4) == 0) {
                KEY_IN_DB *lastKey = getLastKey(dbhandle);
                PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                int i;
                for (i = 0; i < 32; i++) {
                    sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                }
                response = MHD_create_response_from_buffer(sizeof (response_buffer),
                        (void *) response_buffer,
                        MHD_RESPMEM_PERSISTENT);
                //free(lastKey);
            } else
                if (strncmp(post->buff, "key", 3) == 0) {
                //curl --data-ascii key5D56F45ED57EA8DC9CF62322849A36E8D563AE8C7E5A265CFD994213834B73A4 127.0.0.1:55554
                uint8_t sha[32];
                int i;
                for (i = 0; i < 32; i++) {
                    sscanf(post->buff + 3 + i * 2, "%02X", &(sha[i]));
                }
                for (i = 0; i < 32; i++) {
                    printf("%02X", sha[i]);
                }
                KEY_IN_DB *lastKey = getByKey(sha, dbhandle);
                if (lastKey != NULL) {
                    PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                    for (i = 0; i < 32; i++) {
                        sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                    }
                    response = MHD_create_response_from_buffer(sizeof (response_buffer),
                            (void *) response_buffer,
                            MHD_RESPMEM_PERSISTENT);
                    //free(lastKey);
                } else {
                    response = NULL;
                }
            } else {
                uint8_t key[32];
                int i, j;
                bool isadded = false;
                for (j = 0; j <= post->count - 64; j += 64) {
                    for (i = 0; i < 32; i++) {
                        sscanf(post->buff + j * 64 + i * 2, "%02X", &(key[i]));
                    }
                    KEY *k1 = ConstructKey(key);
                    int ret = putKey(k1, dbhandle);
                    PrintKey(k1);
                    //free(k1);
                    if(ret==0)
                        isadded = true;
                }
                if (isadded) {
                    fprintf(stdout, "NEW KEYS\n");
                    fflush(stdout);
                    response_buffer[0] = 'O';
                    response_buffer[1] = 'K';
                    response = MHD_create_response_from_buffer(2,
                            (void*) response_buffer,
                            MHD_RESPMEM_PERSISTENT);
                } else {
                    response = NULL;
                }

            }
            free(post->buff);
        }
    }

    if (post != NULL)
        free(post);
    ret = MHD_queue_response(connection,
            MHD_HTTP_OK,
            response);
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
    size_t len;
    DBT db_key, db_data;
    KEY_IN_DB *key_in = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    if (key_in == NULL) {
        return NULL;
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
    int ret = db->put(db, NULL, &db_key, &db_data, DB_NOOVERWRITE);
    if (ret != 0) {
        my_err("Key ID exists\n");
    }
    db->sync(db, 0);
    return ret;
}

KEY_IN_DB *getByKey(uint8_t *sha, DB *db) {
    DBT key, data;
    memset(&key, 0, sizeof (key));
    memset(&data, 0, sizeof (data));
    key.data = sha;
    key.size = sizeof (uint8_t)*32;
    //data.data = &key_in;
    //data.ulen = sizeof(KEY_IN_DB);
    //data.flags = DB_DBT_USERMEM;
    int ret = db->get(db, NULL, &key, &data, 0);
    if (ret == 0) {
        return data.data;
    } else {
        printf("NOT FOUND\n");
        return NULL;
    }
}

KEY_IN_DB *getLastKey(DB *db) {
    DBT key_dbt, data_dbt;
    DBC *dbc;
    KEY_IN_DB *k = NULL;
    memset(&key_dbt, 0, sizeof (DBT));
    memset(&data_dbt, 0, sizeof (DBT));
    db->cursor(db, NULL, &dbc, 0);
    int ret;
    bool flag = false;
    for (ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_LAST);
            ret == 0;
            ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_PREV)) {
        k = data_dbt.data;
        if (k->usage != 0) {
            if (flag) {
                ret = dbc->get(dbc, &key_dbt, &data_dbt, DB_NEXT);
                k = data_dbt.data;
                break;
            } else {
                break;
            }
        } else {
            flag = true;
        }
    }
    if (k != NULL) {
        k->usage++;
        dbc->put(dbc, &key_dbt, &data_dbt, DB_CURRENT);
        db->sync(db, 0);
    }
    return k;
}

int main(int argc, char *argv[]) {
    
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
    int ret = 0;
    
    // Initialize our DB handle
    ret = db_create(&dbhandle, NULL, 0);
    fprintf(stdout, "Creating\n");
    fflush(stdout);
    if (ret != 0) {
        fprintf(stdout, "Failed to initialize the database handle: %s\n", db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    fprintf(stdout, "Created\n");
    fflush(stdout);
    // Open the existing DATABASE file or create a new one if it doesn't exist.
    ret = dbhandle->open(dbhandle, NULL, DATABASE, NULL, DB_BTREE, DB_CREATE/* | DB_THREAD*/, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to open database file %s: %s\n", DATABASE, db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    fprintf(stdout, "Opened\n");
    fflush(stdout);
    
    //sleep(1);

    KEY *newkey = ConstructKey(key[0]);

    PrintKey(newkey);

    putKey(newkey, dbhandle);

    KEY_IN_DB *retKey = getByKey(newkey->sha, dbhandle);

    if (retKey != NULL) {
        printf("1111");
        KEY *nextKey = ConstructKeyUsage(retKey->key, retKey->usage);
        PrintKey(nextKey);
        //PrintKey(ConstructKeyUsage(nextKey->key, nextKey->usage));
    }

    KEY_IN_DB *lastKey = getLastKey(dbhandle);
    if (lastKey != NULL) {
        printf("2222");
        PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
    }

    int pass = 1;

    /* Check command line options */
    
    fflush(stdout);
                    
    while ((option = getopt(argc, argv, "p:")) > 0) {
        switch (option) {
            case 'h':
                usage();
                break;
            case 'p':
                portCtrl = atoi(optarg);
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }
    /*argv += optind;
    argc -= optind;
    if (argc > 0) {
        my_err("Too many options!\n");
        usage();
    }*/
    struct MHD_Daemon *daemon = MHD_start_daemon(/*MHD_USE_POLL_INTERNALLY,/*/MHD_USE_SELECT_INTERNALLY,
            portCtrl,
            NULL,
            NULL,
            &ahc_echo,
            response_data,
            MHD_OPTION_END);
    if(daemon==NULL)
    {
                my_err("Error in libmicrohttpd\n");        
    }
    
    fflush(stdout);
    while (1) {
        if (toclose==1) {
            break;
        }else{
            sleep(1);
        }
    }
    MHD_stop_daemon(daemon);

    dbhandle->close(dbhandle, 0);
    //DestructQueue(q1);
    //DestructQueue(q2);
    return (0);
}
