#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 1024

struct key {
  char num[8];
  char key[BUF_SIZE];
  size_t size;
  struct key *next;
};

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
		fprintf(stderr, "%x", key->num[i]);
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
    fprintf(stderr, "%x", key->num[i]);
  fprintf(stderr, "\n");
}

size_t crypt(struct key *key)
{
  size_t size;
  int i;

  fprintf(stderr, "crypt ");

  print_key(key);

  size = fread(in, 1, key->size, stdin);
  fprintf(stderr, "size: %d\n", size);
  if (size == 0) return(0);

  //fprintf(stderr, "size: %d in: '%s' \n", size, in);

  for(i=0; i < size; i++)
    out[i] = in[i] ^ key->key[i];

  fwrite(key->num, 1, 8, stdout);
  size = fwrite(out, 1, size, stdout);
  fflush(stdout);

  return(size);
}

size_t decrypt()
{
  struct key *key;
  size_t size;
  int i;

  fprintf(stderr, "decrypt ");
  size = fread(in, 1, 8, stdin);
  fprintf(stderr, "size=%d\n", size);
  if (size == 0) return(0);

  for(i=0; i<8; i++)
    fprintf(stderr, "%x", in[i]);

  for(key = keys; key->next != keys; key=key->next)
    if(strncmp(in, key->num, 8) == 0) break;

  print_key(key);
  size = fread(in, 1, key->size, stdin);
  fprintf(stderr, "size=%d key->size=%d\n", size, key->size);
  //if (size != key->size) return(0);

  for(i=0; i < size; i++)
    out[i] = in[i] ^ key->key[i];
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

  result = get_files(argv[1], argv[2]);
  if(result)
    fprintf(stderr, "%d keys\n", result);
  else
    {
      fprintf(stderr, "Keys not found\n", result);
      exit(-2);
    }

  if(strncmp(fname, "qbcrypt", strlen("qbcrypt")) == 0)
    {
      for(key = keys; ; key=key->next)
	if (!crypt(key)) break;
    }
  else if(strncmp(fname, "qbuncrypt", strlen("qbuncrypt")) == 0)
    while(size)
      size = decrypt();
}
