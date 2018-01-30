/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   post_curl.h
 * Author: giggsoff
 *
 * Created on 16 января 2018 г., 18:19
 */
#include <stdbool.h>
#ifndef POST_CURL_H
#define POST_CURL_H

#ifdef __cplusplus
extern "C" {
#endif

int curl_get_key(char *postthis, bool newkey);
int curl_init_addr(char* _addr, int len);


#ifdef __cplusplus
}
#endif

#endif /* POST_CURL_H */

