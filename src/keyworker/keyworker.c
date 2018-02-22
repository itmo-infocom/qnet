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
#include "keyqueue.h"
#include <microhttpd.h>
#include <db.h>
#include <libgen.h>
#include <openssl/evp.h>
#include <pthread.h>

pthread_mutex_t lock;

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000
#define CLIENT 0
#define SERVER 1
#define PORT 55555
#define PORTC 55554
#define NUMBER_OF_THREADS 2
DB_ENV *dbenv = NULL;
char dbname[200] = "db/keys.db";

unsigned int deviceId = 0;

typedef struct {
    int devnum;
    DB *dbhandle;
} DB_IN_ARRAY;

typedef struct {
    DB_IN_ARRAY *array;
    size_t size;
} Array;

typedef struct {
    uint8_t key[32];
    unsigned int addedtime;
    unsigned int lastusagetime;
    unsigned int usage;
    unsigned int device;
    u_int32_t num_in_support;
} KEY_IN_DB;

typedef struct {
    uint8_t sha[32];
} KEY_IN_SUPPORT_DB;

int initArray(Array *a) {
    a->array = (DB_IN_ARRAY *) malloc(sizeof (DB_IN_ARRAY));
    (a->array)->devnum = -1;
    DB *dbhandle;
    int ret = db_create(&dbhandle, dbenv, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to initialize the database handle: %s\n", db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    ret = dbhandle->open(dbhandle, NULL, basename(dbname), NULL, DB_BTREE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to open database file %s: %s\n", basename(dbname), db_strerror(ret));
        fflush(stdout);
        return 1;
    }
    a->size = 1;
    (a->array)->dbhandle = dbhandle;
    return 0;
}

DB *getOrCreateHandler(Array *a, int num) {
    pthread_mutex_lock(&lock);
    int i;
    for (i = 0; i < a->size; i++) {
        if (((DB_IN_ARRAY *) (a->array + i))->devnum == num) {
            pthread_mutex_unlock(&lock);
            return ((DB_IN_ARRAY *) (a->array + i))->dbhandle;
        }
    }
    a->size++;
    a->array = (DB_IN_ARRAY *) realloc(a->array, a->size * sizeof (DB_IN_ARRAY));
    ((DB_IN_ARRAY *) (a->array + (a->size - 1)))->devnum = num;
    DB *dbhandle;
    int ret = db_create(&dbhandle, dbenv, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to initialize the database handle: %s\n", db_strerror(ret));
        fflush(stdout);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    char supportdbname[205];
    char append[10];
    sprintf(append, "%d", num);
    strcpy(supportdbname, basename(dbname));
    strcat(supportdbname, ".sup");
    strcat(supportdbname, append);
    ret = dbhandle->set_re_len(dbhandle, sizeof (KEY_IN_SUPPORT_DB));
    if (ret != 0) {
        fprintf(stdout, "Failed to set queue size database file %s: %s\n", supportdbname, db_strerror(ret));
        fflush(stdout);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    ret = dbhandle->open(dbhandle, NULL, supportdbname, NULL, DB_QUEUE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0);
    if (ret != 0) {
        fprintf(stdout, "Failed to open database file %s: %s\n", supportdbname, db_strerror(ret));
        fflush(stdout);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    (a->array + (a->size - 1))->dbhandle = dbhandle;
    pthread_mutex_unlock(&lock);
    return dbhandle;
}

void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->size = 0;
}

Array HandlerArray;

char *progname;
volatile int toclose = 0;

volatile int debug = 0;

KEY_IN_DB *getLastKeyDevice(unsigned int device);
KEY_IN_DB *getByKey(uint8_t *sha);
int putKey(KEY *k, unsigned int device);
int assignDeviceBySha(uint8_t *sha, unsigned int device);

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
                if (strncmp(post->buff, "request", 7) == 0) {
                unsigned int realdeviceId = deviceId;
                if (post->count > 7) {
                    realdeviceId = strtoul(post->buff + 7, NULL, 10);
                }
                KEY_IN_DB *lastKey = getLastKeyDevice(deviceId);
                if (lastKey == NULL)
                    return MHD_NO;
                KEY *k = ConstructKeyUsage(lastKey->key, lastKey->usage);
                if (debug) {
                    PrintKey(k);
                }
                int ret = assignDeviceBySha(k->sha, realdeviceId);
                free(k);
                if (ret != 0) {
                    return MHD_NO;
                }
                int i;
                for (i = 0; i < 32; i++) {
                    sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                }
                free(lastKey);
                response = MHD_create_response_from_buffer(sizeof (response_buffer),
                        (void *) response_buffer,
                        MHD_RESPMEM_MUST_COPY);
                to_sync = 1;
            } else
                if (strncmp(post->buff, "last", 4) == 0) {
                unsigned int realdeviceId = deviceId;
                if (post->count > 4) {
                    realdeviceId = strtoul(post->buff + 4, NULL, 10);
                }
                KEY_IN_DB *lastKey = getLastKeyDevice(realdeviceId);
                if (lastKey == NULL)
                    return MHD_NO;
                if (debug) {
                    PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                }
                int i;
                for (i = 0; i < 32; i++) {
                    sprintf(response_buffer + i * 2, "%02X", lastKey->key[i]);
                }
                free(lastKey);
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
                KEY_IN_DB *lastKey = getByKey(sha);
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
                    int ret = putKey(k1, deviceId);
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
    fprintf(stderr, "-p <port>: port to listen on (default 55554)\n");
    fprintf(stderr, "-d 1: show debug\n");
    fprintf(stderr, "-w <threads>: run num of threads (0 to 6)\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

int putKey(KEY *k, unsigned int device) {
    DB_TXN *t;
    size_t len;
    DBC *dbc;
    DBT db_key, db_data;
    DBT db_support_key, db_support_data;
    KEY_IN_DB *key_in = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    if (key_in == NULL) {
        return 1;
    }
    KEY_IN_SUPPORT_DB *key_in_support = (KEY_IN_SUPPORT_DB*) malloc(sizeof (KEY_IN_SUPPORT_DB));
    if (key_in_support == NULL) {
        return 1;
    }
    uint8_t key_buffer[32];
    memset(&db_key, 0, sizeof (DBT));
    memset(&db_data, 0, sizeof (DBT));
    memset(&db_support_key, 0, sizeof (DBT));
    db_support_key.flags = DB_DBT_MALLOC;
    memset(&db_support_data, 0, sizeof (DBT));
    memcpy(&(key_in->key), &(k->key), sizeof (k->key));
    unsigned int curt = (unsigned int) time(NULL);
    memcpy(&(key_in->addedtime), &curt, sizeof (unsigned int));
    memcpy(&(key_in->lastusagetime), &curt, sizeof (unsigned int));
    uint8_t was_usage = 0;
    memcpy(&(key_in->usage), &was_usage, sizeof (uint8_t));
    memcpy(&(key_in->device), &device, sizeof (unsigned int));
    memcpy(&key_buffer, &(k->sha), sizeof (key_buffer));
    memcpy(&(key_in_support->sha), &(k->sha), sizeof (k->sha));
    db_key.data = &key_buffer;
    db_key.size = sizeof (key_buffer);
    db_data.data = key_in;
    db_data.size = sizeof (KEY_IN_DB);
    db_support_data.data = key_in_support;
    db_support_data.size = sizeof (KEY_IN_SUPPORT_DB);
    int ret = 0;
    DB *dbhandle = getOrCreateHandler(&HandlerArray, -1);
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        dbhandle->cursor(dbhandle, t, &dbc, 0);

        ret = dbhandle->put(dbhandle, t, &db_key, &db_data, DB_NOOVERWRITE);
        if (ret != 0) {
            if (t != NULL)
                (void)t->abort(t);
            free(key_in);
            free(key_in_support);
            free(db_support_key.data);
            return -1;
        }
        DB *supportdbhandle = getOrCreateHandler(&HandlerArray, device);
        ret = supportdbhandle->put(supportdbhandle, t, &db_support_key, &db_support_data, DB_APPEND);
        if (ret != 0) {
            if (t != NULL)
                (void)t->abort(t);
            free(key_in);
            free(key_in_support);
            free(db_support_key.data);
            return -1;
        }
        memcpy(&(key_in->num_in_support), db_support_key.data, sizeof (key_in->num_in_support));
        ret = dbhandle->put(dbhandle, t, &db_key, &db_data, 0);
        if (ret != 0) {
            if (t != NULL)
                (void)t->abort(t);
            free(key_in);
            free(key_in_support);
            free(db_support_key.data);
            return -1;
        }
        if (t->commit(t, 0) != 0) {
            printf("ERROR IN COMMIT!!!\n");
        }
        free(db_support_key.data);
    } else {
        if (t != NULL)
            (void)t->abort(t);
        printf("ERROR IN TRANSACTION!!!\n");
        free(key_in);
        free(key_in_support);
        free(db_support_key.data);
        return -1;
    }
    if (ret != 0) {
        if (debug) {
            switch (ret) {
                case DB_KEYEXIST:
                    my_err("Key ID exists\n");
                    break;
                default:
                    dbhandle->err(dbhandle, ret, "DB->put");
            }
        }
        return -1;
    }
    free(key_in);
    return ret;
}

int assignDeviceBySha(uint8_t *sha, unsigned int device) {
    DB_TXN *t;
    DBC *dbc;
    DBT key, data;
    memset(&key, 0, sizeof (DBT));
    memset(&data, 0, sizeof (DBT));
    data.flags = DB_DBT_MALLOC;
    KEY_IN_SUPPORT_DB *key_in_support = (KEY_IN_SUPPORT_DB*) malloc(sizeof (KEY_IN_SUPPORT_DB));
    if (key_in_support == NULL) {
        return 1;
    }
    DBT db_support_key, db_support_data;
    memset(&db_support_key, 0, sizeof (DBT));
    memset(&db_support_data, 0, sizeof (DBT));
    db_support_data.flags = DB_DBT_MALLOC;
    key.data = sha;
    key.size = sizeof (uint8_t)*32;
    //data.data = &key_in;
    //data.ulen = sizeof(KEY_IN_DB);
    //data.flags = DB_DBT_USERMEM;
    DB *dbhandle = getOrCreateHandler(&HandlerArray, -1);
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        dbhandle->cursor(dbhandle, t, &dbc, 0);
        int ret = dbc->get(dbc, &key, &data, DB_SET);
        if (ret != 0) {
            my_err("NOT FOUND\n");
            (void) t->abort(t);
            return 1;
        }
        if (((KEY_IN_DB*) data.data)->usage == 0) {
            DB *supportdbhandlefrom = getOrCreateHandler(&HandlerArray, ((KEY_IN_DB*) data.data)->device);
            db_support_key.data = &(((KEY_IN_DB*) data.data)->num_in_support);
            db_support_key.size = sizeof (((KEY_IN_DB*) data.data)->num_in_support);
            ret = supportdbhandlefrom->del(supportdbhandlefrom, t, &db_support_key, 0);
            if (ret != 0) {
                my_err("CANNOT DELETE %d from dev %d\n", ((KEY_IN_DB*) data.data)->num_in_support, ((KEY_IN_DB*) data.data)->device);
                (void) t->abort(t);
                return 1;
            }
            DB *supportdbhandleto = getOrCreateHandler(&HandlerArray, device);
            memset(&db_support_key, 0, sizeof (DBT));
            memset(&db_support_data, 0, sizeof (DBT));
            db_support_key.flags = DB_DBT_MALLOC;
            memcpy(&(key_in_support->sha), sha, sizeof (key_in_support->sha));
            db_support_data.data = key_in_support;
            db_support_data.size = sizeof (KEY_IN_SUPPORT_DB);
            ret = supportdbhandleto->put(supportdbhandleto, t, &db_support_key, &db_support_data, DB_APPEND);
            if (ret != 0) {
                my_err("ERROR IN SUPPORT PUT\n");
                if (t != NULL)
                    (void)t->abort(t);
                return -1;
            }
            memcpy(&(((KEY_IN_DB*) data.data)->num_in_support), db_support_key.data, sizeof (((KEY_IN_DB*) data.data)->num_in_support));
        }
        unsigned int curt = (unsigned int) time(NULL);
        memcpy(&(((KEY_IN_DB*) data.data)->lastusagetime), &curt, sizeof (unsigned int));
        memcpy(&(((KEY_IN_DB*) data.data)->device), &device, sizeof (unsigned int));
        ret = dbc->put(dbc, &key, &data, DB_CURRENT);
        if (ret != 0) {
            my_err("ERROR IN PUT\n");
            (void) t->abort(t);
            return 1;
        }
        ret = t->commit(t, 0);
        free(data.data);
        if (ret != 0) {
            (void) t->abort(t);
            printf("ERROR IN COMMIT!!!\n");
            return 1;
        }
        if (ret == 0) {
            return 0;
        }
    } else {
        printf("ERROR IN TRANSACTION!!!\n");
        fflush(stdout);
        if (t != NULL)
            (void)t->abort(t);
        return 2;
    }
    printf("ERROR IN ASSIGN!!!\n");
    fflush(stdout);
    free(key_in_support);
    return 1;
}

KEY_IN_DB *getByKey(uint8_t *sha) {
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
    DB *dbhandle = getOrCreateHandler(&HandlerArray, -1);
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        int ret = dbhandle->get(dbhandle, t, &key, &data, 0);
        if (ret != 0) {
            if (t != NULL)
                (void)t->abort(t);
            free(data.data);
            return NULL;
        }
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

KEY_IN_DB *getLastKeyDevice(unsigned int device) {
    DBT key_dbt, data_dbt;
    DBT support_key_dbt, support_data_dbt;
    DBC *dbc1;
    DBC *dbc2;
    DB_TXN *t;
    KEY_IN_DB *k = (KEY_IN_DB*) malloc(sizeof (KEY_IN_DB));
    KEY_IN_SUPPORT_DB *kis = (KEY_IN_SUPPORT_DB*) malloc(sizeof (KEY_IN_SUPPORT_DB));
    memset(&key_dbt, 0, sizeof (DBT));
    memset(&data_dbt, 0, sizeof (DBT));
    data_dbt.flags = DB_DBT_MALLOC;
    memset(&support_key_dbt, 0, sizeof (DBT));
    memset(&support_data_dbt, 0, sizeof (DBT));
    support_key_dbt.flags = DB_DBT_MALLOC;
    support_data_dbt.flags = DB_DBT_MALLOC;
    DB *dbhandle = getOrCreateHandler(&HandlerArray, -1);
    DB *supportdbhandle = getOrCreateHandler(&HandlerArray, device);
    if (dbenv->txn_begin(dbenv, NULL, &t, 0) == 0) {
        supportdbhandle->cursor(supportdbhandle, t, &dbc1, 0);
        int ret = dbc1->get(dbc1, &support_key_dbt, &support_data_dbt, DB_CONSUME);
        if (ret == 0) {
            dbhandle->cursor(dbhandle, t, &dbc2, 0);
            key_dbt.size = sizeof(((KEY_IN_SUPPORT_DB*)(support_data_dbt.data))->sha);
            key_dbt.data = &(((KEY_IN_SUPPORT_DB*)(support_data_dbt.data))->sha);
            ret = dbc2->get(dbc2, &key_dbt, &data_dbt, DB_SET);
            if (ret == 0) {
                unsigned int curt = (unsigned int) time(NULL);
                memcpy(&(((KEY_IN_DB*) data_dbt.data)->lastusagetime), &curt, sizeof (unsigned int));
                memcpy(k, data_dbt.data, sizeof (KEY_IN_DB));
                if (k->usage < 1000000) {
                    k->usage++;
                    ((KEY_IN_DB*) data_dbt.data)->usage = k->usage;
                }
                dbc2->put(dbc2, &key_dbt, &data_dbt, DB_CURRENT);
                free(data_dbt.data);
                if (t->commit(t, 0) != 0) {
                    if (t != NULL)
                        (void)t->abort(t);
                    printf("ERROR IN COMMIT!!!\n");
                }
            } else {
                fprintf(stdout, "Failed to req from cursor database: %s\n", db_strerror(ret));
                if (t != NULL)
                    (void)t->abort(t);
                return NULL;
            }
        } else {
            dbhandle->cursor(dbhandle, t, &dbc2, 0);
            int isok = 0;
            for (ret = dbc2->get(dbc2, &key_dbt, &data_dbt, DB_LAST);
                    ret == 0;
                    ret = dbc2->get(dbc2, &key_dbt, &data_dbt, DB_PREV)) {
                if (((KEY_IN_DB*) (data_dbt.data))->device == device) {
                    unsigned int curt = (unsigned int) time(NULL);
                    memcpy(&(((KEY_IN_DB*) data_dbt.data)->lastusagetime), &curt, sizeof (unsigned int));
                    memcpy(k, data_dbt.data, sizeof (KEY_IN_DB));
                    if (k->usage < 1000000) {
                        k->usage++;
                        ((KEY_IN_DB*) data_dbt.data)->usage = k->usage;
                    }
                    dbc2->put(dbc2, &key_dbt, &data_dbt, DB_CURRENT);
                    free(data_dbt.data);
                    if (t->commit(t, 0) != 0) {
                        if (t != NULL)
                            (void)t->abort(t);
                        printf("ERROR IN COMMIT!!!\n");
                    }
                    isok = 1;
                    break;
                }
            }
            if (isok == 0) {
                if (t != NULL)
                    (void)t->abort(t);
                printf("NO KEYS!!!\n");
                return NULL;
            }
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
    printf("\nCLEARING\n");
    if (my_daemon) {
        MHD_stop_daemon(my_daemon);
    }
    int i;
    for (i = 0; i < (&HandlerArray)->size; i++) {
        DB *supportdbhandle = ((&HandlerArray)->array + i)->dbhandle;
        if (supportdbhandle) {
            supportdbhandle->sync(supportdbhandle, 0);
            supportdbhandle->close(supportdbhandle, 0);
        }
    }
    if (dbenv) {
        dbenv->close(dbenv, 0);
    }
    EVP_cleanup();
    pthread_mutex_destroy(&lock);
    freeArray(&HandlerArray);
    printf("EXIT\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    int thnum = NUMBER_OF_THREADS;
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
    uint8_t key[3][32] = {
        {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4},
        {0x61, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4},
        {0x62, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4}
    };
    u_int32_t envFlags;
    int ret = 0;
    char dbdir[200] = "";

    OpenSSL_add_all_digests();

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }

    while ((option = getopt(argc, argv, "p:h:n:d:w:")) > 0) {
        switch (option) {
            case 'd':
                debug = atoi(optarg);
                break;
            case 'h':
                usage();
                break;
            case 'p':
                portCtrl = atoi(optarg);
                break;
            case 'w':
                thnum = atoi(optarg);
                break;
            case 'n':
                strncpy(dbname, optarg, 200);
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }
    strcpy(dbdir, dbname);
    dirname(dbdir);

    mkdir(dbdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    fprintf(stdout, "file %s\n", dbname);

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        printf("%s: %s\n", progname, db_strerror(ret));
        return (ret);
    }
    if ((ret =
            dbenv->open(dbenv, dbdir, DB_CREATE | DB_RECOVER | DB_INIT_LOCK |
            DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_THREAD, 0)) != 0) {
        dbenv->err(dbenv, ret, "environment open: %s", dirname(dbname));
        dbenv->close(dbenv, 0);
        return (ret);
    }
    if (debug) {
        fprintf(stdout, "Creating\n");
        fflush(stdout);
    }
    if (debug) {
        fprintf(stdout, "Created\n");
        fflush(stdout);
    }


    initArray(&HandlerArray);

    if (debug) {
        fprintf(stdout, "Opened\n");
        fflush(stdout);
    }

    KEY *newkey = ConstructKey(key[1]);

    putKey(newkey, 1);

    newkey = ConstructKey(key[2]);

    putKey(newkey, 2);

    newkey = ConstructKey(key[0]);

    putKey(newkey, deviceId);

    if (debug) {
        PrintKey(newkey);
    }
    if (debug < 2)
        free(newkey);

    if (debug > 1) {
        PrintKey(newkey);

        KEY_IN_DB *retKey = getByKey(newkey->sha);

        free(newkey);

        if (retKey != NULL) {
            printf("0000\n");
            KEY *nextKey = ConstructKeyUsage(retKey->key, retKey->usage);

            printf("devID: %d\n", retKey->device);
            PrintKey(nextKey);
            //PrintKey(ConstructKeyUsage(nextKey->key, nextKey->usage));
            free(retKey);
            KEY_IN_DB *lastKeyOk = getLastKeyDevice(1);
            if (lastKeyOk != NULL) {
                printf("1111\n");
                printf("devID: %d\n", lastKeyOk->device);
                PrintKey(ConstructKeyUsage(lastKeyOk->key, lastKeyOk->usage));
                free(lastKeyOk);
            }
            assignDeviceBySha(nextKey->sha, deviceId + 1);


            KEY_IN_DB *lastKeyW = getLastKeyDevice(deviceId);
            if (lastKeyW != NULL) {
                printf("2222\n");
                printf("devID: %d\n", lastKeyW->device);
                PrintKey(ConstructKeyUsage(lastKeyW->key, lastKeyW->usage));
                free(lastKeyW);
            }
            assignDeviceBySha(nextKey->sha, deviceId);
            
            KEY_IN_DB *lastKey = getLastKeyDevice(deviceId);
            if (lastKey != NULL) {
                printf("3333\n");
                printf("devID: %d\n", lastKey->device);
                PrintKey(ConstructKeyUsage(lastKey->key, lastKey->usage));
                free(lastKey);
            }
        }
        fflush(stdout);
    }

    int pass = 1;

    if (thnum > 6) {
        thnum = 6;
    } else if (thnum <= 0) {
        thnum = 1;
    }
    my_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_SUPPRESS_DATE_NO_CLOCK
            | MHD_USE_EPOLL_LINUX_ONLY | MHD_USE_EPOLL_TURBO
            ,
            portCtrl,
            NULL,
            NULL,
            &ahc_echo,
            NULL,
            MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
            MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) thnum,
            MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 100,
            MHD_OPTION_END);
    if (my_daemon == NULL) {
        my_err("Error in libmicrohttpd\n");
        return 1;
    }
    fflush(stdout);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    while (1) {
        sleep(10); /*
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fileno(stdout), &set);
        if (select(fileno(stdout) + 1, &set, NULL, NULL, &timeout) != 1) {
            sleep(1);
            getc(stdout);
        }*/
    }
    intHandler(0);
    return (0);
}

