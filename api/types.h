#ifndef TYPES__H_
#define TYPES__H_
#include <stdio.h>

struct curl_response {
    char *html;
    size_t size;
};

struct torrent {
    char *id;
    char *information;
    char *seeders;
    char *size;
    struct torrent *next;
};

enum cookie_type{
    COOKIE_RUTRACKER,
    COOKIE_QBITTORRENT
};

#endif