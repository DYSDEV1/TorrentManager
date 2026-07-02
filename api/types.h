#ifndef TYPES__H_
#define TYPES__H_
#include <stdio.h>
#include <pthread.h>
#include <curl/curl.h>


#define DOWNLOAD_PROGRESS_BUFFER_SIZE 128
#define UPLOAD_PROGRESS_BUFFER_SIZE 40

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
    CURL *curl_handle;
    char download_progress[DOWNLOAD_PROGRESS_BUFFER_SIZE];
    char upload_progress[UPLOAD_PROGRESS_BUFFER_SIZE];
};

enum thread_status{
    RUNNING,
    UPLOADING,
    DOWNLOADING,
    FAILED,
    FINISHED
};

struct thread_torrent {
    pthread_t thread;
    pthread_mutex_t mutex;

    CURL *curl_handle;

    char *id;
    char name[128];
    char hash[65];
    char full_path[4096];

    enum thread_status status;

    char upload_progress[40];
    char download_progress[128];

};

enum cookie_type{
    COOKIE_RUTRACKER,
    COOKIE_QBITTORRENT
};

#endif