#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

//#define BUF_SIZE 1024
#define BUF_SIZE 100

struct key {
  int num;
  char key[BUF_SIZE];
  size_t size;
  struct key *next;
};

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
  key->num = num;
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
	  fprintf(stderr, "key.num=%d key.size=%d\n", key->num, key->size);
	  for(i=0; i < key->size; i++)
	    fprintf(stderr, "key[%d]: 0%o\n", i, key->key[i]);

	  key = key_alloc(++num);
	  if (key == NULL) return(0);
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

  fprintf(stderr, "key: %d %d\n", key->num, key->size);
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

  for(i=0; i < size; i++) {
    out[i] = in[i] ^ key->key[i];
    fprintf(stderr, "i=%d in: 0%o out: 0%o key: 0%o\n",
	    i, in[i], out[i], key->key[i]);
  }
  fwrite(&key->num, 1, sizeof(int), stdout);
  fwrite(&key->size, 1, sizeof(size_t), stdout);
  size = fwrite(out, 1, size, stdout);
  fflush(stdout);

  fprintf(stderr, "num=%d size=%d\n", key->num, key->size);

  return(size);
}

size_t decrypt()
{
  struct key *key = keys;
  size_t num, size;
  int i;

  fprintf(stderr, "decrypt ");
  if (! fread(&num, 1, sizeof(int), stdin)) return(0);
  if (! fread(&size, 1, sizeof(size_t), stdin)) return(0);
  fprintf(stderr, "num=%d key->num=%d (num - key->num)=%d\n", num, key->num, num - key->num);
  fprintf(stderr, "num=%d size=%d\n", num, size);

  do {
    fprintf(stderr, "num=%d key->num=%d (num - key->num)=%d\n", num, key->num, num - key->num);
    if(num == key->num) {
      print_key(key);
      size = fread(in, 1, key->size, stdin);
      fprintf(stderr, "size=%d key->size=%d\n", size, key->size);
      fflush(stdout);

      for(i=0; i < size; i++) {
	out[i] = in[i] ^ key->key[i];
	fprintf(stderr, "i=%d in: 0%o out: 0%o key: 0%o\n",
		i, in[i], out[i], key->key[i]);
      }
      size = fwrite(out, 1, size, stdout);
      fflush(stdout);
      break;
    }
    key=key->next;
  } while (key->next != keys);

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

  if(argc != 2)
    {
      fprintf(stderr, "Usage: %s key_file\n", fname);
      exit(-1);
    }

  result = get_key(argv[1]);
  if(result)
    fprintf(stderr, "%d bytes in key\n", result);
  else
    {
      fprintf(stderr, "Key not found\n", result);
      exit(-2);
    }

  if(strncmp(fname, "qacrypt", strlen("qacrypt")) == 0)
    {
      for(key = keys; ; key=key->next)
	if (!crypt(key)) break;
    }
  else if(strncmp(fname, "qauncrypt", strlen("qauncrypt")) == 0)
    while(size)
      size = decrypt();
}
