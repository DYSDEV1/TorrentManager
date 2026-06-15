#ifndef TYPES__H_
#define TYPES__H_
#include <stdio.h>

struct curl_response {
    char *html;
    size_t size;
};

struct torrent {
    char* id;
    char *information;
    struct torrent *next;
};

#endif