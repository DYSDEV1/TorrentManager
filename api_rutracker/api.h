#ifndef API__H_
#define API__H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
//#include <libxml2/libxml/HTMLparser.h>
//#include <libxml2/libxml/xpath.h>

#define BASE_URL "https://rutracker.org"
#define LOGIN_STRING_ENCODED "%C2%F5%EE%E4"
#define ENDPOINT_LOGIN "https://rutracker.org/forum/login.php"
#define ENDPOINT_SEARCH "https://rutracker.org/forum/tracker.php"
#define ENDPOINT_DOWNLOAD_TORRENT "https://rutracker.org/forum/dl.php?t={id}" 


int authenticate(CURL *curl_handle,const char* user, const char* password);

int search(CURL *curl_handle,const char* search_string);

bool isLogged(CURL *curl_handle);




#endif