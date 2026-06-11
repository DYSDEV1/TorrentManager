#ifndef TYPES__H_
#define TYPES__H_
#include <stdio.h>

struct curl_response {
    char *html;
    size_t size;
};

struct torrent {
    int id;
    char *information;
    struct torrent *next;
};

#endif