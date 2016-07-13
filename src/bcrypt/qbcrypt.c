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
char *arg2;

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

struct key *get_key(char *file)
{
  FILE *f;
  size_t res;
  struct key *key, *k;

  fprintf(stderr, "file=%s\n", file);
  f = fopen(file, "r");
  if (f == NULL) return(NULL);

  key = malloc(sizeof(struct key));
  if (key == NULL) return(NULL);
  key->curpos=0;
  res = fread(key->num, 1, 8, f);
  if (res < 8)
    {
      free(key);
      return(NULL);
    }
  res = fread(key->key, 1, BUF_SIZE, f);
  if (res)
    {
      key->size = res;
      if(keys == NULL)
	{
	  keys = key;
	}
      else
	{
	  k = keys;
	  while(k->next != keys)
	    {
	      k=k->next;
	    }
	  k->next = key;
	}
      // Circular list
      key->next = keys;
    }
  else
    {
      free(key);
      return(NULL);
    }
  return(key);
}

int get_files(char *dir_name, char *tail)
{
  DIR *d;
  struct dirent *dir;
  int cnt=0, i;
  size_t len;
  struct key *key;
  char fname[BUF_SIZE];
  
  if(keys!=NULL){
	free(keys);
	keys = NULL;
  }
  d = opendir(dir_name);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      len = strlen(dir->d_name);
      if (strncmp(dir->d_name + len - strlen(tail), tail, len) == 0)
	{
	  strncpy(fname,dir_name,BUF_SIZE);
	  strncat(fname,"/",BUF_SIZE);
	  strncat(fname,dir->d_name,BUF_SIZE);
	  key = get_key(fname);
	  if(key)
	    {
	      cnt++;
	      fprintf(stderr, "%s %d\n", dir->d_name, key->size+8);
	      for(i=0; i<8; i++)
		fprintf(stderr, "%x ", key->num[i]);
	      fprintf(stderr, "\n");
	    }
	}
    }
    closedir(d);
  }
  return(cnt);
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
		get_files(arg1,arg2);
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
		get_files(arg1,arg2);
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

main(int argc, char **argv)
{
  char *fname;
  char *p;
  int result;
  struct key *key;
  size_t size=1;

  if ((p = strrchr(argv[0], '/'))  != NULL) fname = p+1;
  else exit(-1);
  fprintf(stderr, "fname=%s\n", fname);

  if(argc != 3)
    {
      fprintf(stderr, "Usage: %s keys_dir key_tail\n", fname);
      exit(-1);
    }

  arg1 = malloc(strlen(argv[1])+1);
  strcpy(arg1, argv[1]);
  arg2 = malloc(strlen(argv[2])+1);
  strcpy(arg2, argv[2]);
  result = get_files(arg1, arg2);
  if(result)
    fprintf(stderr, "%d keys\n", result);
  else
    {
      fprintf(stderr, "Keys not found\n", result);
      exit(-2);
    }

  if(strncmp(fname, "qbcrypt", strlen("qbcrypt")) == 0)
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
  else if(strncmp(fname, "qbuncrypt", strlen("qbuncrypt")) == 0)
    while(size)
      size = decrypt();
  free(arg1);
  free(arg2);
}

