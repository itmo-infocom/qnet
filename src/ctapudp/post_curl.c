#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
 
struct MemoryStruct {
  char *memory;
  size_t size;
};

char addr[30];

char key[64];
 
size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

int curl_init_addr(char* _addr, int len){
    memcpy(addr,_addr,len);
    return 0;
}
 
char* curl_get_key(char *postthis, bool newkey)
{
  bool realkey=false;
  CURL *curl;
  CURLcode res;
  struct MemoryStruct chunk;
 
  chunk.memory = malloc(1);  /* will be grown as needed by realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) { 
    curl_easy_setopt(curl, CURLOPT_URL, addr);
 
    /* send all data to this function  */ 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
 
    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */ 
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
 
    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));
 
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    else {
      printf("%s\n",chunk.memory);
      if(newkey){
          realkey = true;
      }
    }
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
 
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
    if(realkey){
        memcpy(key,chunk.memory,64);
        free(chunk.memory);
        return key;
    }else{
        free(chunk.memory);
    }
  }
  return NULL;
}