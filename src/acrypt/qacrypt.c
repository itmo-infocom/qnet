#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


#define BUF_SIZE 1024
#define BLOCK_SIZE BUF_SIZE/128

struct key {
  char num[8];
  char key[BUF_SIZE];
  int curpos;
  size_t size;
  struct key *next;
};
char *myfifo = "/tmp/myfifo";
int fd;
char buf[1];
pthread_t tid;
pthread_mutex_t lock;
int reread = 0;
int toclose = 0;
char *arg1;

int checkfifo(){
	return access(myfifo, F_OK)!=-1;
}

void* doWork(void *arg){
	int size; 

        while(checkfifo()){
                size = read(fd, buf, 1);
                if(size>0&&strncmp(buf,"\n",1)){
                        if(!strncmp(buf,"r",1)){
				fprintf(stderr, "REREAD KEYS\n");
                                pthread_mutex_lock(&lock);
                                reread = 1;
                                pthread_mutex_unlock(&lock);
                        }else{
				if(!strncmp(buf,"q",1)){
                                        pthread_mutex_lock(&lock);
                                        toclose = 1;
                                        pthread_mutex_unlock(&lock);
                                        return NULL;
                                }
                        }
                        fflush(stdout);
                }
         }
}



struct key *keys = NULL;

char in[BUF_SIZE], out[BUF_SIZE];
struct key *key_alloc(int num)
{
  struct key *key, *k;

  key = malloc(sizeof(struct key));
  if (key == NULL) return(key);

  if(num) {
    // Find last key in chain
    k = keys;
    while(k->next != keys) {
      fprintf(stderr, "key->next=0x%x\n", k->next);
      k=k->next;
    }
    k->next = key;
  } else keys = key;

  key->size = 0;
  memcpy(key->num,(void*)num,sizeof(int));
  // Circular list
  key->next = keys;

  return(key);
}

int get_key(char *file)
{
  FILE *f;
  size_t res=0;
  int bit, byte=0, num=0, bcnt=0, cnt=0, ret=0, tmp;
  int i;
  struct key *key, *k;

  fprintf(stderr, "file=%s\n", file);
  f = fopen(file, "r");
  if (f == NULL) return(0);

  key = key_alloc(0);
  if (key == NULL) return(0);
  key->curpos = 0;
  while(fgets(in, BUF_SIZE, f) != NULL) {
    ret = sscanf(in, "%d       %d", &bit, &tmp);
    if (ret) {
      if (bcnt < 8) {
	byte |= bit << bcnt; bcnt++;
	fprintf(stderr, "bit=%d tmp=%d byte=0%o\n", bit, tmp, byte);
      }
      else {
	fprintf(stderr, "byte=%x cnt=%d\n", byte, cnt);
	// Next byte
	key->key[cnt] = byte;
	key->size = cnt++;
	bcnt = 0;
	byte = bit;
	// Next key
	if (cnt > BUF_SIZE) {
	  res += cnt;
	  fprintf(stderr, "key.num=%s key.size=%d\n", key->num, key->size);
	  for(i=0; i < key->size; i++)
	    fprintf(stderr, "key[%d]: 0%o\n", i, key->key[i]);

	  key = key_alloc(++num);
	  if (key == NULL) return(0);
          key->curpos = 0;
	  cnt = 0;
	}
      }
    }
  }

  res += cnt;
  return(res);
}


void print_key(struct key * key)
{
  int i;

  fprintf(stderr, "%d ", key->size+8);
  for(i=0; i<8; i++)
    fprintf(stderr, "%x ", key->num[i]);
  fprintf(stderr, "%d", key->curpos);
  fprintf(stderr, "\n");
}

size_t crypt_my(struct key *key)
{
  size_t size;
  int i;
  size_t readsize;
  fprintf(stderr, "crypt ");

  print_key(key);
  while(!feof(stdin)){
  pthread_mutex_lock(&lock);
  if(toclose){
	break;
  }else{
	if(reread){
		reread = 0;
		get_key(arg1);
		key = keys;
        }
  }
  pthread_mutex_unlock(&lock);

  size = fread(in, 1, BLOCK_SIZE, stdin);
  readsize = size;
  fprintf(stderr, "size: %d\n", size);
  if (size == 0) return(0);

  fprintf(stderr, "size: %d in: '%s' \n", size, in);
  

  for(i=0; i < size; i++){
    out[i] = in[i] ^ key->key[i+key->curpos];
    fprintf(stderr, "%x", out[i]);
  }
  fwrite(key->num, 1, 8, stdout);
  fwrite(&(key->curpos), sizeof(int),1, stdout);
  key->curpos+=size;
  if(key->curpos>=key->size){
	key->curpos=0;
        key = key->next;
  }
  size = fwrite(out, 1, size, stdout);
  fflush(stdout);
  if(readsize < BLOCK_SIZE)
	if(feof(stdin))
		break;
  }
  return(size);
}

size_t decrypt()
{
  struct key *key;
  size_t size;
  int i;
  int flagToReload = 1;

  fprintf(stderr, "decrypt ");
  size = fread(in, 1, 8, stdin);
  fprintf(stderr, "size=%d\n", size);
  if (size == 0) return(0);

  for(i=0; i<8; i++)
    fprintf(stderr, "%x", in[i]);

  while(flagToReload==1){
  	for(key = keys; key->next != keys; key=key->next)
    		if(strncmp(in, key->num, 8) == 0){
			flagToReload = 0;
			break;
    		}
  	if(flagToReload==1){
		get_key(arg1);
  	}
  }
  //current key position
  fread(&(key->curpos), sizeof(int),1, stdin);
  print_key(key);
  size = fread(in, 1, size, stdin);
  fprintf(stderr, "size=%d key->size=%d\n", size, key->size);
  //if (size != key->size) return(0);

  for(i=0; i < size; i++){
    out[i] = in[i] ^ key->key[i+key->curpos];
  }
  size = fwrite(out, 1, size, stdout);
  fflush(stdout);

  return(size);
}

int main(int argc, char **argv)
{
  char *fname;
  char *p;
  int result;
  struct key *key;
  size_t size=1;

  if ((p = strrchr(argv[0], '/'))  != NULL) fname = p+1;
  else exit(-1);
  fprintf(stderr, "fname=%s\n", fname);

  if(argc != 2)
    {
      fprintf(stderr, "Usage: %s keys_file\n", fname);
      exit(-1);
    }

  arg1 = malloc(strlen(argv[1])+1);
  strcpy(arg1, argv[1]);
  result = get_key(arg1);
  if(result)
    fprintf(stderr, "%d bytes in key\n", result);
  else
    {
      fprintf(stderr, "Key not found\n", result);
      exit(-2);
    }

  if(strncmp(fname, "qacrypt", strlen("qacrypt")) == 0)
    {
        while(!checkfifo()){
                sleep(0.1);
        }
        fd = open(myfifo, O_RDONLY);
	pthread_mutex_init(&lock, NULL);
	pthread_create(&tid,NULL,&doWork, NULL);
	key = keys;
	crypt_my(key);
	pthread_mutex_destroy(&lock);
	close(fd);
    }
  else if(strncmp(fname, "qauncrypt", strlen("qauncrypt")) == 0)
    while(size)
      size = decrypt();
  free(arg1);
}

