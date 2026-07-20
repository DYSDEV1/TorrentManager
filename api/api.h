#ifndef API__H_
#define API__H_

#include "../gui/gui.h"
#include "types.h"
#include "utils.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE_URL "https://rutracker.org"
#define LOGIN_STRING_ENCODED "%C2%F5%EE%E4"
#define ENDPOINT_LOGIN_RUTRACKER "https://rutracker.org/forum/login.php"
#define ENDPOINT_SEARCH_RUTRACKER                                              \
  "https://rutracker.org/forum/tracker.php?nm=%s"
#define ENDPOINT_DOWNLOAD_RUTRACKER "https://rutracker.org/forum/dl.php?t=%s"
#define ENDPOINT_LOGIN_QBITTORRENT                                             \
  "https://kit.seedhost.eu/%s/qbittorrent/api/v2/auth/login"
#define ENDPOINT_UPLOAD_TORRENT                                                \
  "https://kit.seedhost.eu/%s/qbittorrent/api/v2/torrents/add"
#define ENDPOINT_GET_HASH                                                      \
  "https://kit.seedhost.eu/%s/qbittorrent/api/v2/torrents/info?tag=%s"
#define ENDPOINT_GET_TORRENT_PROGRESSION                                       \
  "https://kit.seedhost.eu/%s/qbittorrent/api/v2/torrents/info?hashes=%s"


#define FMT_RUTRACKER "login_username=%s&login_password=%s&login=%s"
#define FMT_QBITTORRENT "username=%s&password=%s"
#define MAX_RESPONSE_SIZE (50 * 1024 * 1024)
#define FILENAME_SIZE 1

#define POSTFIELDS_SEARCH "f%%5B%%5D=-1&o=7&s=2&pn=&nm=%s"

int authenticate(CURL *curl_handle, const char *fmt, const char *user,
                 const char *password, FILE *log_file,
                 const char *cookie_file_name, const char *endpoint_url,
                 enum cookie_type ct);

struct torrent *search(CURL *curl_handle, const char *search_string,
                       FILE *log_file, const char* full_path_cookie_rutracker);

bool isLogged(CURL *curl_handle, enum cookie_type);

int download(CURL *curl_handle, const char *torrent_name,const char *full_path_torrent_file, FILE *log_file, const char* full_path_cookie_rutracker);

int uploadToServer(struct thread_torrent *tt);

int retrieveTorrentInfo(struct thread_torrent *tt);

/* thread functions */

int retrieveUploadProgression(struct thread_torrent *tt);

void *handle_threaded_torrent_download(void *arg);

#endif
