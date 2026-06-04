#ifndef API__H_
#define API__H_

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>

#define BASE_URL "https://rutracker.org"
#define LOGIN_STRING_ENCODED "%C2%F5%EE%E4"
#define ENDPOINT_LOGIN "https://rutracker.org/forum/login.php"
#define ENDPOINT_SEARCH "https://rutracker.org/forum/tracker.php"
#define ENDPOINT_DOWNLOAD_TORRENT "https://rutracker.org/forum/dl.php?t={id}" 


int authUser(CURL *curl_handle,const char* user, const char* password);





#endif