#ifndef TYPES__H_
#define TYPES__H_
#include <curl/curl.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>

#define MAX_THREAD 5
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
};

enum thread_status { INIT,UPLOADING_OR_DOWNLOADING, FAILED, FINISHED };

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

struct info_download_torrents {
  char upload_progress[40];
  char download_progress[128];
  char name[128];
};

struct downloaded_movies_list{
  struct dirent **movies_list;
  char **movies_list_name_cleaned;
  int number_of_movies;
};

enum cookie_type { COOKIE_RUTRACKER, COOKIE_QBITTORRENT };

#endif
