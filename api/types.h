#ifndef TYPES__H_
#define TYPES__H_
#include <stdio.h>

struct curl_response {
    char *html;
    size_t size;
};

struct torrent {
    char *id;
    char name[128];
    char *information;
    char *seeders;
    char *size;
    char hash[65];
    char full_path[4096];
    struct torrent *next;
};

enum cookie_type{
    COOKIE_RUTRACKER,
    COOKIE_QBITTORRENT
};

#endif