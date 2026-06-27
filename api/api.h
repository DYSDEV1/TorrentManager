#ifndef API__H_
#define API__H_

#define _GNU_SOURCE

#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "types.h"
#include "utils.h"


#define BASE_URL "https://rutracker.org"
#define LOGIN_STRING_ENCODED "%C2%F5%EE%E4"
#define ENDPOINT_LOGIN_RUTRACKER "https://rutracker.org/forum/login.php"
#define ENDPOINT_SEARCH "https://rutracker.org/forum/tracker.php?nm=%s"
#define ENDPOINT_DOWNLOAD "https://rutracker.org/forum/dl.php?t=%s" 
#define ENDPOINT_LOGIN_QBITTORRENT "https://kit.seedhost.eu/temp432456/qbittorrent/api/v2/auth/login"
#define ENDPOINT_UPLOAD_TORRENT "https://kit.seedhost.eu/temp432456/qbittorrent/api/v2/torrents/add"
#define ENDPOINT_GET_HASH "https://kit.seedhost.eu/temp432456/qbittorrent/api/v2/torrents/info?tag=%s"
#define ENDPOINT_GET_TORRENT_PROGRESSION "https://kit.seedhost.eu/temp432456/qbittorrent/api/v2/torrents/info?hashes=%s"
#define ENDPOINT_SFTP "kit.seedhost.eu"
#define COOKIE_FILENAME_RUTRACKER "rutracker_cookie.txt"
#define COOKIE_FILENAME_QBITTORRENT "qbitorrent_cookie.txt"

#define FMT_RUTRACKER "login_username=%s&login_password=%s&login=%s"
#define FMT_QBITTORRENT "username=%s&password=%s"
#define MAX_RESPONSE_SIZE (50 * 1024 * 1024) 
#define FILENAME_SIZE 1


int authenticate(CURL *curl_handle,const char* fmt,const char* user, const char* password, FILE *log_file, const char* cookie_file_name, const char* endpoint_url, enum cookie_type ct);

struct torrent* search(CURL *curl_handle,const char* search_string, FILE *log_file);

bool isLogged(CURL *curl_handle, enum cookie_type);


int download(CURL *curl_handle,const char* torrent_name, FILE *log_file);

int uploadToServer(CURL *curl_handle,char* torrent_name, FILE *log_file);

int retrieveUploadProgression(CURL *curl_handle,char* hash, FILE *log_file);

int retrieveTorrentInfo(CURL *curl_handle,char* torrent_id,char* torrent_name,char* hash,char* torrent_path,FILE *log_file);

int downloadFromServer(CURL *curl_handle,char* torrent_name,char* torrent_path, FILE *log_file);


#endif