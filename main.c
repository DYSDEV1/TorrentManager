#include "api/api.h"
#include "api/env.h"
#include "api/types.h"
#include "gui/gui.h"
#include <curl/curl.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE 256

int main() {
  FILE *log_file = 0;
  struct ctx ctx = {0};
  int input_ch = 0;
  bool exit = false;

  struct thread_torrent *active_downloads[MAX_THREAD] = {0};
  struct info_download_torrents *info_dt[MAX_THREAD] = {0};
  struct thread_torrent *thread_torrent = {0};
  struct torrent *torrents_list = {0};
  struct torrent *selected_torrent = {0};

  int count_torrent_currently_downloading = 0;
  char user_input[MAX_INPUT_SIZE];
  int size_user_input;

  log_file = fopen("logs.txt", "w");
  if (log_file == NULL) {
    goto cleanup;
  }
  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl_handle_rutracker = curl_easy_init();
  if (!curl_handle_rutracker) {
    fprintf(log_file, "[!] Failed to create curl handle for rutracker\n");
    goto cleanup;
  }
  CURL *curl_handle_qbitorrent = curl_easy_init();
  if (!curl_handle_qbitorrent) {
    fprintf(log_file, "[!] Failed to create curl handle for qbitorrent\n");
    goto cleanup;
  }
  CURL *curl_handle_sftp = curl_easy_init();
  if (!curl_handle_sftp) {
    fprintf(log_file, "[!] Failed to create curl handle for sftp\n");
    goto cleanup;
  }

  if (loadEnv() != 0) {
    fprintf(log_file, "[!] Failed to set env variables\n");
    goto cleanup;
  }

  if (authenticate(curl_handle_rutracker, FMT_RUTRACKER,
                   getenv("username_rutracker"), getenv("password_rutracker"),
                   log_file, COOKIE_FILENAME_RUTRACKER,
                   ENDPOINT_LOGIN_RUTRACKER, COOKIE_RUTRACKER) != 0) {
    fprintf(log_file, "[!] Failed to authenticate to rutorrent\n");
    goto cleanup;
  }

  if (authenticate(curl_handle_qbitorrent, FMT_QBITTORRENT,
                   getenv("username_qbittorrent"),
                   getenv("password_qbittorrent"), log_file,
                   COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT,
                   COOKIE_QBITTORRENT) != 0) {
    fprintf(log_file, "[!] Failed to authenticate to qbitorrent\n");
    goto cleanup;
  }
  gui_init(&ctx);
  ctx.current_windows_state = SEARCH;
  ctx.tab_changed = true;
  ctx.is_result = false;
  size_user_input = 0;
  while (!exit) {
    enum thread_status curr_status;
    /*Retrieve Torrent download update*/
    for (int i = 0; (i < count_torrent_currently_downloading) &&
                    (count_torrent_currently_downloading > 0);
         i++) {
      pthread_mutex_lock(&active_downloads[i]->mutex);
      curr_status = active_downloads[i]->status;
      pthread_mutex_unlock(&active_downloads[i]->mutex);
      if (curr_status == UPLOADING_OR_DOWNLOADING) {
        pthread_mutex_lock(&active_downloads[i]->mutex);
        snprintf(info_dt[i]->upload_progress,
                 sizeof(info_dt[i]->upload_progress), "%s",
                 active_downloads[i]->upload_progress);
        snprintf(info_dt[i]->download_progress,
                 sizeof(info_dt[i]->download_progress), "%s",
                 active_downloads[i]->download_progress);
        snprintf(info_dt[i]->name, sizeof(active_downloads[i]->name), "%s",
                 active_downloads[i]->name);
        pthread_mutex_unlock(&active_downloads[i]->mutex);
      }
      if ((curr_status == FINISHED) || (curr_status == FAILED)) {
        pthread_join(active_downloads[i]->thread, NULL);
        pthread_mutex_destroy(&active_downloads[i]->mutex);
        curl_easy_cleanup(active_downloads[i]->curl_handle);
        free(active_downloads[i]->id);
        free(active_downloads[i]);
        if (count_torrent_currently_downloading == 1) {
          active_downloads[i] = 0;
        } else {
          active_downloads[i] =
              active_downloads[count_torrent_currently_downloading - 1];
          i--;
        }
        count_torrent_currently_downloading--;
      }
    }
    /* Handle user input*/
    input_ch = getch();
    switch (input_ch) {
    case '\x1b':
      exit = true;
      goto cleanup;
    case KEY_UP:
      if (ctx.current_windows_state == RESULTS) {
        menu_driver(ctx.menu, REQ_UP_ITEM);
        selected_torrent = item_userptr(current_item(ctx.menu));
        if (!selected_torrent) {
          fprintf(log_file, "[!] Failed to get item ptr\n");
          goto cleanup;
        }
        ctx.dirty_menu = true;
        ctx.dirty_description = true;
      }
      break;
    case KEY_DOWN:
      if (ctx.current_windows_state == RESULTS) {
        menu_driver(ctx.menu, REQ_DOWN_ITEM);
        selected_torrent = item_userptr(current_item(ctx.menu));
        if (!selected_torrent) {
          fprintf(log_file, "[!] Failed to get item ptr\n");
          goto cleanup;
        }
        ctx.dirty_menu = true;
        ctx.dirty_description = true;
      }
      break;
    case KEY_LEFT:
      if (ctx.current_windows_state != 0) {
        ctx.tab_changed = true;
        ctx.current_windows_state--;
      }
      break;
    case KEY_RIGHT:
      if ((ctx.current_windows_state < 2) && (ctx.is_result)) {
        ctx.tab_changed = true;
        ctx.current_windows_state++;
      }
      break;
    case KEY_NPAGE:
      menu_driver(ctx.menu, REQ_SCR_DPAGE);
      break;
    case KEY_PPAGE:
      menu_driver(ctx.menu, REQ_SCR_UPAGE);
      break;
    case KEY_BACKSPACE:
      if (ctx.current_windows_state == SEARCH) {
        if (size_user_input > 0) {
          user_input[--size_user_input] = '\0';
        }
        ctx.dirty_search = true;
      }
      break;
    case 10:
      /*search on rutorrent user_input*/
      if (ctx.current_windows_state == SEARCH) {
        /* SEARCH  */
        user_input[size_user_input] = '\0';
        size_user_input = 0;
        torrents_cleanup(torrents_list);
        torrents_list = NULL;
        torrents_list = search(curl_handle_rutracker, user_input, log_file);
        if (!torrents_list) {
          continue;
        }
        if (gui_create_menu(torrents_list, &ctx) != 0) {
          fprintf(log_file, "[!] Failed to create menu\n");
          goto cleanup;
        }
        if (!ctx.menu) {
          fprintf(log_file, "[!] Ctx menu not posted\n");
          goto cleanup;
        }
        ctx.is_result = true;
        ctx.tab_changed = true;
        ctx.current_windows_state = RESULTS;
        selected_torrent = item_userptr(current_item(ctx.menu));
        continue;
      }
      /* download the torrent file from rtorrent*/
      if (ctx.current_windows_state == RESULTS) {
        if (count_torrent_currently_downloading >= MAX_THREAD) {
          fprintf(log_file, "[!] Max 5 downloads at a time, please wait\n");
          ctx.current_windows_state = SEARCH;
          break;
        }
        selected_torrent = item_userptr(current_item(ctx.menu));
        if (!selected_torrent) {
          fprintf(log_file, "[!] Failed to get item ptr\n");
          ctx.current_windows_state = SEARCH;
          ctx.tab_changed = true;
          break;
        }
        if (download(curl_handle_rutracker, selected_torrent->id, log_file) !=
            0) {
          fprintf(log_file, "[!] Failed to download torrent\n");
          ctx.current_windows_state = SEARCH;
          break;
        }
        /* Create a thread for each torrent:
         *  - retrieve download status on qbittorent
         *  - retrieve final file/directory with rsync
         */
        thread_torrent = malloc(sizeof(struct thread_torrent));
        info_dt[count_torrent_currently_downloading] =
            calloc(1, sizeof(struct info_download_torrents));
        if (selected_torrent) {
          thread_torrent->id = strdup(selected_torrent->id);
        }
        thread_torrent->status = INIT;
        thread_torrent->curl_handle = curl_easy_init();
        if (!thread_torrent->curl_handle) {
          fprintf(log_file, "[!] Failed to create handle for the torrent\n");
          ctx.current_windows_state = SEARCH;
          break;
        }
        pthread_mutex_init(&thread_torrent->mutex, NULL);
        /* Authenticate the curl_handle thread*/
        if (authenticate(thread_torrent->curl_handle, FMT_QBITTORRENT,
                         getenv("username_qbittorrent"),
                         getenv("password_qbittorrent"), log_file,
                         COOKIE_FILENAME_QBITTORRENT,
                         ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0) {
          fprintf(log_file, "[!] Failed to authenticate to qbitorrent\n");
          goto cleanup;
        }

        if (pthread_create(&thread_torrent->thread, NULL,
                           handle_threaded_torrent_download,
                           (void *)thread_torrent) != 0) {
          fprintf(log_file, "[!] Failed to create thread \n");
          break;
        }
        active_downloads[count_torrent_currently_downloading++] =
            thread_torrent;
        ctx.current_windows_state = DOWNLOADS;
        ctx.tab_changed = true;
      }
      break;
    default:
      if ((ctx.current_windows_state == SEARCH) &&
          (size_user_input < (MAX_INPUT_SIZE - 1)) && (input_ch != ERR)) {
        user_input[size_user_input++] = (char)input_ch;
        user_input[size_user_input] = '\0';
        ctx.dirty_search = true;
      }
    }

    gui_render(&ctx, selected_torrent, user_input, info_dt,
               count_torrent_currently_downloading);
  }

cleanup:
  gui_menu_cleanup(&ctx);
  torrents_cleanup(torrents_list);
  if (log_file)
    fclose(log_file);
  if (curl_handle_rutracker)
    curl_easy_cleanup(curl_handle_rutracker);
  if (curl_handle_qbitorrent)
    curl_easy_cleanup(curl_handle_qbitorrent);
  if (curl_handle_sftp)
    curl_easy_cleanup(curl_handle_sftp);
  delwin(ctx.win_search_bar);
  ctx.win_search_bar = NULL;
  delwin(ctx.win_description);
  ctx.win_description = NULL;
  endwin();
  curl_global_cleanup();

  return 0;
}
