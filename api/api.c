#include "api.h"
#include "config_file.h"
#include "types.h"
#include "utils.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

int saveHtmlToFile(FILE *html_file, struct curl_response *curl_res) {
  html_file = fopen("html_save.html", "w");
  if (html_file == NULL) {
    return -1;
  }

  fwrite(curl_res->html, curl_res->size, 1, html_file);
  fclose(html_file);
  return 0;
}

bool isLogged(CURL *curl_handle, enum cookie_type cookie_type) {
  struct curl_slist *cookies = NULL;
  long expiration_date;
  bool code_function_return = false;
  curl_easy_getinfo(curl_handle, CURLINFO_COOKIELIST, &cookies);
  struct curl_slist *cookies_ptr = cookies;
  if (!cookies) {
    goto cleanup;
  }
  while (cookies != NULL) {
    if (cookie_type == COOKIE_RUTRACKER) {
      if (strstr(cookies->data, "bb_session") != 0) {
        if (sscanf(cookies->data, "%*s %*s %*s %*s %ld", &expiration_date) !=
            1) {
          goto cleanup;
        }
        if (expiration_date > (long)time(NULL)) {
          code_function_return = true;
          goto cleanup;
        }
      }
    }
    if (cookie_type == COOKIE_QBITTORRENT) {
      if (strstr(cookies->data, "SID") != 0)
        code_function_return = true;
    }

    cookies = cookies->next;
  }
cleanup:
  curl_slist_free_all(cookies_ptr);
  return code_function_return;
}

static size_t handleCurlResponse(char *data, size_t size, size_t nmemb,
                                 void *clientp) {

  size_t real_size;
  size_t alloc_size;
  size_t tmp_size;

  struct curl_response *curl_res = (struct curl_response *)clientp;

  if (!curl_res) {
    return 0;
  }

  if (ckd_mul(&real_size, size, nmemb)) {
    return 0;
  }
  if (ckd_add(&tmp_size, curl_res->size, real_size)) {
    return 0;
  }
  if (ckd_add(&alloc_size, tmp_size, 1)) {
    return 0;
  }
  if (alloc_size > MAX_RESPONSE_SIZE) {
    return 0;
  }

  char *ptr_response = realloc(curl_res->html, alloc_size);
  if (!ptr_response) {
    return 0;
  }

  curl_res->html = ptr_response;
  memcpy(&curl_res->html[curl_res->size], data, real_size);
  curl_res->size += real_size;
  curl_res->html[curl_res->size] = '\0';

  return real_size;
}

static size_t handleDownloadTorrent(char *ptr, size_t size, size_t nmemb,
                                    void *stream) {
  if (!ptr || !size || !nmemb || !stream) {
    return 0;
  }
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  if (written == 0) {
    return 0;
  }
  return written;
}

int authenticate(CURL *curl_handle, const char *fmt, const char *user,
                 const char *password, FILE *log_file,
                 const char *cookie_file_name, const char *endpoint_url,
                 enum cookie_type cookie_type) {

  struct curl_response curl_res = {0};
  int code_function_return = 0;
  char *postfields = NULL;
  char *encoded_user = NULL;
  char *encoded_password = NULL;
  const char *url = endpoint_url;
  char *alloc_url = NULL;
  if (!user || !password) {
    fprintf(log_file, "[!] User or pasword NULL\n");
    code_function_return = -1;
    goto cleanup;
  }
  if ((encoded_user = curl_easy_escape(curl_handle, user, (int)strlen(user))) ==
      NULL) {
    fprintf(log_file, "[!] Failed to encode user\n");
    code_function_return = -1;
    goto cleanup;
  }
  if ((encoded_password = curl_easy_escape(curl_handle, password,
                                           (int)strlen(password))) == NULL) {
    fprintf(log_file, "[!] Failed to encode password\n");
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE,
                                               cookie_file_name)) == CURLE_OK) {
    if (curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "RELOAD") !=
        CURLE_OK) {
      fprintf(log_file, "[!] Failed to reload cookies\n");
      goto cleanup;
    }
    if (cookie_type == COOKIE_RUTRACKER && isLogged(curl_handle, cookie_type)) {
      fprintf(log_file, "[+] Already logged, cookies still actives\n");
      goto cleanup;
    }
  }
  /* Qbitorrent url need to add user*/
  if (cookie_type == COOKIE_QBITTORRENT) {

    if (!(alloc_url = alloc_format(endpoint_url, encoded_user))) {
      fprintf(log_file, "[!] Failed to allocate url");
      code_function_return = -1;
      goto cleanup;
    }
    url = alloc_url;
  }
  if ((code_function_return =
           curl_easy_setopt(curl_handle, CURLOPT_URL, url)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set url.\n");
    goto cleanup;
  }
  if ((code_function_return =
           curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                            &handleCurlResponse)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set writefunction.\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(
           curl_handle, CURLOPT_WRITEDATA, (void *)&curl_res)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set result structure\n");
    goto cleanup;
  }
  if (!(postfields = alloc_format(fmt, encoded_user, encoded_password,
                                  LOGIN_STRING_ENCODED))) {
    fprintf(log_file, "[!] Failed to allocate postfields");
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS,
                                               postfields)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set postfields\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR,
                                               cookie_file_name)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set cookies\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_perform(curl_handle)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to execute authenticate request.\n");
    goto cleanup;
  }

  if (isLogged(curl_handle, cookie_type)) {
    fprintf(log_file, "[+] Login successful\n");
  } else {
    fprintf(log_file, "[-] Failed to login\n");
    code_function_return = -1;
  }

cleanup:
  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
  if (encoded_user)
    curl_free(encoded_user);
  if (encoded_password)
    curl_free(encoded_password);
  if (postfields)
    free(postfields);
  if (curl_res.html)
    free(curl_res.html);
  if (alloc_url)
    free(alloc_url);
  fflush(log_file);

  return code_function_return;
}

struct torrent *search(CURL *curl_handle, const char *search_string,
                       FILE *log_file, const char* full_path_cookie_rutracker) {
  struct curl_response curl_res = {0};
  int code_function_return = 0;
  char *url_search = NULL;
  char *postfields = NULL;
  char *encoded_search_string = NULL;
  struct torrent *torrents_list = NULL;

  if (!isLogged(curl_handle, COOKIE_RUTRACKER)) {
    if (authenticate(curl_handle, FMT_RUTRACKER, getenv("username_rutracker"),
                     getenv("password_rutracker"), log_file,
                     full_path_cookie_rutracker, ENDPOINT_LOGIN_RUTRACKER,
                     COOKIE_RUTRACKER) != 0) {
      fprintf(log_file, "[!] Failed to authenticate");
      code_function_return = -1;
      goto cleanup;
    }
  }

  if ((encoded_search_string = curl_easy_escape(
           curl_handle, search_string, (int)strlen(search_string))) == NULL) {
    fprintf(log_file, "[!] Failed to encode url.\n");
    code_function_return = -1;
    goto cleanup;
  }
  if (!(url_search = alloc_format(ENDPOINT_SEARCH_RUTRACKER, encoded_search_string))) {
    fprintf(log_file, "[!] Failed to allocated url\n");
    code_function_return = -1;
    goto cleanup;
  }
  if(!(postfields = alloc_format(POSTFIELDS_SEARCH,encoded_search_string))){
    fprintf(log_file, "[!] Failed to allocate postfields\n");
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_URL,
                                               url_search)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set url.\n");
    goto cleanup;
  }
  if ((code_function_return =
           curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                            &handleCurlResponse)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set writefunction.\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(
           curl_handle, CURLOPT_WRITEDATA, (void *)&curl_res)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set result structure\n");
    goto cleanup;
  }
  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS,
                                               postfields)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set postfields\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_perform(curl_handle)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to execute request\n");
    goto cleanup;
  }
  torrents_list = parse(&curl_res);

cleanup:
  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
  if (curl_res.html)
    free(curl_res.html);
  if (url_search)
    free(url_search);
  if (encoded_search_string)
    curl_free(encoded_search_string);
  if (postfields)
    free(postfields);

  fflush(log_file);
  if (code_function_return == -1) {
    return NULL;
  } else {
    return torrents_list;
  }
}
int download(CURL *curl_handle, const char *torrent_name,const char* full_path_torrent_file, FILE *log_file, const char* full_path_cookie_rutracker) {
  struct curl_response curl_res = {0};
  int code_function_return = 0;
  char *url_download = NULL;
  FILE *torrent_file = NULL;

  if (!isLogged(curl_handle, COOKIE_RUTRACKER)) {
    if (authenticate(curl_handle, FMT_RUTRACKER, getenv("username_rutracker"),
                     getenv("password_rutracker"), log_file,
                     full_path_cookie_rutracker, ENDPOINT_LOGIN_RUTRACKER,
                     COOKIE_RUTRACKER) != 0) {
      fprintf(log_file, "[!] Failed to authenticate");
      code_function_return = -1;
      goto cleanup;
    }
  }
  if (!(url_download =
            alloc_format(ENDPOINT_DOWNLOAD_RUTRACKER, torrent_name))) {
    fprintf(log_file, "[!] Failed to allocate ENDPOINT_DOWNLOAD_RUTRACKER\n");
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(curl_handle, CURLOPT_URL,
                                               url_download)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set url.\n");
    goto cleanup;
  }
  if ((code_function_return =
           curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                            &handleDownloadTorrent)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set writefunction.\n");
    goto cleanup;
  }

  torrent_file = fopen(full_path_torrent_file, "wb");
  if (!torrent_file) {
    fprintf(log_file, "[!] Failed to create torrent file\n");
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(
           curl_handle, CURLOPT_WRITEDATA, (void *)torrent_file)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to set result structure\n");
    goto cleanup;
  }

  if ((code_function_return = curl_easy_perform(curl_handle)) != CURLE_OK) {
    fprintf(log_file, "[!] Failed to execute request\n");
    goto cleanup;
  }

cleanup:
  if (torrent_file)
    fclose(torrent_file);
  if (curl_res.html)
    free(curl_res.html);
  if (url_download)
    free(url_download);

  fflush(log_file);
  return code_function_return;
}

int uploadToServer(struct thread_torrent *torrent_thread) {
    struct curl_response curl_res = {0};
    int code_function_return = 0;
    curl_mime *mime = NULL;
    curl_mimepart *part = NULL;
    char *url = NULL;
    const char *user = getenv("username_qbittorrent");

    if (!isLogged(torrent_thread->curl_handle, COOKIE_QBITTORRENT)) {
        code_function_return = -1;
        goto cleanup;
    }
    if (!user) {
        code_function_return = -1;
        goto cleanup;
    }
    if (!(url = alloc_format(ENDPOINT_UPLOAD_TORRENT, user))) {
        code_function_return = -1;
        goto cleanup;
    }
    if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_URL, url)) != CURLE_OK) {
        goto cleanup;
    }
    if ((mime = curl_mime_init(torrent_thread->curl_handle)) == NULL) {
        code_function_return = -1;
        goto cleanup;
    }
    /* ADD TORRENT */
    if ((part = curl_mime_addpart(mime)) == NULL) {
        code_function_return = -1;
        goto cleanup;
    }
    if ((code_function_return = curl_mime_filedata(part, torrent_thread->full_path_torrent_file)) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_mime_name(part, "torrents")) != CURLE_OK) {
        goto cleanup;
    }
    /* ADD TAG */
    if ((part = curl_mime_addpart(mime)) == NULL) {
        code_function_return = -1;
        goto cleanup;
    }
    if ((code_function_return = curl_mime_name(part, "tags")) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_mime_data(part, torrent_thread->id, strlen(torrent_thread->id))) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_MIMEPOST, mime)) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA, (void *)&curl_res)) != CURLE_OK) {
        goto cleanup;
    }
    if ((code_function_return = curl_easy_perform(torrent_thread->curl_handle)) != CURLE_OK) {
        goto cleanup;
    }
    if (!curl_res.html || strstr(curl_res.html, "Ok.") == 0) {
        code_function_return = -1;
        goto cleanup;
    }
    if (remove(torrent_thread->full_path_torrent_file) != 0) {
        code_function_return = -1;
        goto cleanup;
    }

cleanup:
    curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_MIMEPOST, NULL);
    if (url) free(url);
    if (mime) curl_mime_free(mime);
    if (curl_res.html) free(curl_res.html);
    return code_function_return;
}


int retrieveTorrentInfo(struct thread_torrent *torrent_thread) {
  struct curl_response curl_res = {0};
  int code_function_return = 0;
  char *url_retrieve_hash = NULL;
  char *hash_ptr = NULL;
  char *path_ptr = NULL;
  char *name_ptr = NULL;
  const char *username = getenv("username_qbittorrent");

  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_MIMEPOST, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POST, 0L);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_HTTPGET, 1L);

  if (!isLogged(torrent_thread->curl_handle, COOKIE_QBITTORRENT)) {
    code_function_return = -1;
    goto cleanup;
  }

  if (!username) {
    code_function_return = -1;
    goto cleanup;
  }
  if (!(url_retrieve_hash =
            alloc_format(ENDPOINT_GET_HASH, username, torrent_thread->id))) {
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return =
           curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_URL,
                            url_retrieve_hash)) != CURLE_OK) {
    goto cleanup;
  }

  if ((code_function_return =
           curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION,
                            &handleCurlResponse)) != CURLE_OK) {
    goto cleanup;
  }

  if ((code_function_return =
           curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA,
                            (void *)&curl_res)) != CURLE_OK) {
    goto cleanup;
  }

  if ((code_function_return = curl_easy_perform(torrent_thread->curl_handle)) !=
      CURLE_OK) {
    goto cleanup;
  }
  /* PATH */
  if (!curl_res.html ||
      !(path_ptr = strstr(curl_res.html, HTML_INDICATOR_TORRENT_PATH))) {
    code_function_return = -1;
    goto cleanup;
  }
  path_ptr += strlen(HTML_INDICATOR_TORRENT_PATH);
  if ((sscanf(path_ptr, SSCANF_EXTRACT_TORRENT_PATH,
              torrent_thread->full_path) != 1)) {
    code_function_return = -1;
    goto cleanup;
  }
  /* NAME */
  if (!curl_res.html ||
      !(name_ptr = strstr(curl_res.html, HTML_INDICATOR_TORRENT_NAME))) {
    code_function_return = -1;
    goto cleanup;
  }
  name_ptr += strlen(HTML_INDICATOR_TORRENT_NAME);
  if ((sscanf(name_ptr, SSCANF_EXTRACT_TORRENT_NAME, torrent_thread->name) !=
       1)) {
    code_function_return = -1;
    goto cleanup;
  }

  if (!curl_res.html ||
      !(hash_ptr = strstr(curl_res.html, HTML_INDICATOR_HASH))) {
    code_function_return = -1;
    goto cleanup;
  }
  if ((sscanf(hash_ptr, SSCANF_EXTRACT_HASH, torrent_thread->hash) != 1)) {
    code_function_return = -1;
    goto cleanup;
  }

cleanup:
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POST, 0L);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
  if (url_retrieve_hash)
    free(url_retrieve_hash);
  if (curl_res.html)
    free(curl_res.html);

  return code_function_return;
}

int retrieveUploadProgression(struct thread_torrent *torrent_thread) {
  struct curl_response curl_res = {0};
  int code_function_return = 0;
  const char *username = getenv("username_qbittorrent");
  char *url = NULL;
  char *progress_ptr = NULL;
  float progress = 0;
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_MIMEPOST, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_POST, 0L);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_HTTPGET, 1L);

  if (!username) {
    code_function_return = -1;
    goto cleanup;
  }
  if (!(url = alloc_format(ENDPOINT_GET_TORRENT_PROGRESSION, username,torrent_thread->hash))) {
    code_function_return = -1;
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle,
                                               CURLOPT_URL, url)) != CURLE_OK) {
    goto cleanup;
  }
  if ((code_function_return =
           curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION,
                            &handleCurlResponse)) != CURLE_OK) {
    goto cleanup;
  }

  if ((code_function_return = curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA,
                            (void *)&curl_res)) != CURLE_OK) {
    goto cleanup;
  }
  while (progress != 1) {
    code_function_return = curl_easy_perform(torrent_thread->curl_handle);
    if (code_function_return != CURLE_OK) {
      goto cleanup;
    }

    if ((progress_ptr = strstr(curl_res.html, HTML_INDICATOR_PROGRESS)) ==
        NULL) {
      code_function_return = -1;
      goto cleanup;
    }
    if ((sscanf(progress_ptr, SSCANF_EXTRACT_PROGRESS, &progress)) != 1) {
      code_function_return = -1;
      goto cleanup;
    }
    pthread_mutex_lock(&torrent_thread->mutex);
    memset(torrent_thread->upload_progress, 0, UPLOAD_PROGRESS_BUFFER_SIZE);
    snprintf(torrent_thread->upload_progress, UPLOAD_PROGRESS_BUFFER_SIZE,
             "Downloading : %d%%", (int)(progress * 100));
    pthread_mutex_unlock(&torrent_thread->mutex);
    if (curl_res.html) {
      free(curl_res.html);
      curl_res.html = NULL;
      curl_res.size = 0;
    }
    sleep(2);
  }
cleanup:
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt(torrent_thread->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
  if (url)
    curl_free(url);
  if (curl_res.html)
    free(curl_res.html);

  return code_function_return;
}

int downloadFromServer(struct thread_torrent *torrent_thread) {
  char *full_rsync_path = NULL;
  int code_function_return = 0;
  int pipefd[2];
  pid_t pid_rsync;
  char read_buffer;
  const char *endpoint_rsync = getenv("server_rsync");
  char rsync_buffer[DOWNLOAD_PROGRESS_BUFFER_SIZE];
  const char *download_path = getenv("download_path");
  int count = 0;

  if (!endpoint_rsync) {
    code_function_return = -1;
    goto cleanup;
  }

  if (!download_path) {
    code_function_return = -1;
    goto cleanup;
  }
  if ((endpoint_rsync = getenv("server_rsync")) == NULL) {
    code_function_return = -1;
    goto cleanup;
  }
  if (!(full_rsync_path =
            alloc_format("%s:%s",endpoint_rsync, torrent_thread->full_path))) {
    code_function_return = -1;
    goto cleanup;
  }

  if (pipe(pipefd) == -1) {
    code_function_return = -1;
    goto cleanup;
  }

  pid_rsync = fork();

  if (pid_rsync < 0) {
    code_function_return = -1;
    goto cleanup;
  }
  // child
  if (pid_rsync == 0) {
    if (close(pipefd[0]) == -1) {
      _exit(EXIT_FAILURE);
    }
    if (dup2(pipefd[1], 1) == -1) {
      _exit(EXIT_FAILURE);
    }
    close(pipefd[1]);
    if (execlp("rsync", "rsync", "-a", "--info=progress2", "--info=name0", "--",
               full_rsync_path, download_path, NULL) == -1) {
      _exit(EXIT_FAILURE);
    }
    _exit(EXIT_SUCCESS);
  }
  // parent
  if (close(pipefd[1]) == -1) {
    code_function_return = -1;
    goto cleanup;
  }
  while (read(pipefd[0], &read_buffer, 1) > 0) {
    if ((read_buffer == '\r') || (read_buffer == '\n') ||
        (count >= (DOWNLOAD_PROGRESS_BUFFER_SIZE - 1))) {
      rsync_buffer[count] = '\0';
      pthread_mutex_lock(&torrent_thread->mutex);
      memset(torrent_thread->download_progress, 0,
             DOWNLOAD_PROGRESS_BUFFER_SIZE);
      memcpy(torrent_thread->download_progress, rsync_buffer,
             ((size_t)count + 1));
      pthread_mutex_unlock(&torrent_thread->mutex);
      count = 0;
    } else {
      rsync_buffer[count++] = read_buffer;
    }
  }
  close(pipefd[0]);
  if (count > 0) {
    rsync_buffer[count] = '\0';
    pthread_mutex_lock(&torrent_thread->mutex);
    memset(torrent_thread->download_progress, 0, DOWNLOAD_PROGRESS_BUFFER_SIZE);
    memcpy(torrent_thread->download_progress, rsync_buffer,
           ((size_t)count + 1));
    pthread_mutex_unlock(&torrent_thread->mutex);
  }
  waitpid(pid_rsync, &code_function_return, 0);

cleanup:
  if (full_rsync_path)
    free(full_rsync_path);

  return code_function_return;
}

void *handle_threaded_torrent_download(void *arg) {

  struct thread_torrent *torrent_thread = (struct thread_torrent *)arg;
  pthread_mutex_lock(&torrent_thread->mutex);
  torrent_thread->status = UPLOADING_OR_DOWNLOADING;
  pthread_mutex_unlock(&torrent_thread->mutex);
  if (uploadToServer(torrent_thread)) {
    pthread_mutex_lock(&torrent_thread->mutex);
    torrent_thread->status = FAILED;
    pthread_mutex_unlock(&torrent_thread->mutex);
    return NULL;
  }
  if (retrieveTorrentInfo(torrent_thread)) {
    pthread_mutex_lock(&torrent_thread->mutex);
    torrent_thread->status = FAILED;
    pthread_mutex_unlock(&torrent_thread->mutex);
    return NULL;
  }
  if (retrieveUploadProgression(torrent_thread) != 0) {
    pthread_mutex_lock(&torrent_thread->mutex);
    torrent_thread->status = FAILED;
    pthread_mutex_unlock(&torrent_thread->mutex);
    return NULL;
  }
  if (downloadFromServer(torrent_thread) != 0) {
    pthread_mutex_lock(&torrent_thread->mutex);
    torrent_thread->status = FAILED;
    pthread_mutex_unlock(&torrent_thread->mutex);
    return NULL;
  }
  pthread_mutex_lock(&torrent_thread->mutex);
  torrent_thread->status = FINISHED;
  pthread_mutex_unlock(&torrent_thread->mutex);
  return NULL;
}
