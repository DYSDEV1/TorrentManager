#include "api/api.h"
#include "api/env.h"
#include "api/types.h"
#include "api/utils.h"
#include "gui/gui.h"
#include <curl/curl.h>
#include <dirent.h>
#include <menu.h>
#include <ncurses.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

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
  enum thread_status curr_status;
  struct downloaded_movies_list *dl_movies_list = {0};


  int count_torrent_currently_downloading = 0;
  char user_input[MAX_INPUT_SIZE];
  int size_user_input;

  CURL *curl_handle_rutracker = NULL;
  CURL *curl_handle_qbittorrent = NULL;
  CURL *curl_handle_sftp = NULL;

  pid_t pid_mpv;
  struct dirent *dirent_mpv_movie = {0};
  char* full_path_mpv_movie = NULL;



  log_file = fopen("logs.txt", "w");
  if (!log_file) {
    goto cleanup;
  }
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle_rutracker = curl_easy_init();
  if (!curl_handle_rutracker) {
    fprintf(log_file, "[!] Failed to create curl handle for rutracker\n");
    goto cleanup;
  }
  curl_handle_qbittorrent = curl_easy_init();
  if (!curl_handle_qbittorrent) {
    fprintf(log_file, "[!] Failed to create curl handle for qbitorrent\n");
    goto cleanup;
  }
  curl_handle_sftp = curl_easy_init();
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

  if (authenticate(curl_handle_qbittorrent, FMT_QBITTORRENT,
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

  /* init and populate downloaded movies tab*/
  if(!(dl_movies_list = enumerate_movies(log_file))){
    fprintf(log_file, "[!] Failed to retrieve downloaded movies\n");
    goto cleanup;
  }
  if(gui_create_downloaded_movies_menu(dl_movies_list, &ctx) != 0){
    fprintf(log_file, "[!] Failed to create downloaded movies menu\n");
    goto cleanup;
  }
  while (!exit) {
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
        free(info_dt[i]);
        /* Edge case if it is the last download*/
        if (count_torrent_currently_downloading == 1) {
          active_downloads[i] = 0;
        } else {
          active_downloads[i] = active_downloads[count_torrent_currently_downloading - 1];
          i--;
        }
        count_torrent_currently_downloading--;
        ctx.dirty_menu_downloaded_movies = true;

        /* Update the list of downloaded movies*/
        downloaded_movies_list_cleanup(dl_movies_list);
        gui_menu_cleanup(&ctx.menu_obj_movies_downloaded);
        if(!(dl_movies_list = enumerate_movies(log_file))){
          fprintf(log_file, "[!] Failed to retrieve downloaded movies\n");
          goto cleanup;
        }
        if(gui_create_downloaded_movies_menu(dl_movies_list, &ctx) != 0){
          fprintf(log_file, "[!] Failed to create downloaded movies menu\n");
          goto cleanup;
        }
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
          menu_driver(ctx.menu_obj_torrents_result.menu, REQ_UP_ITEM);
          selected_torrent = item_userptr(current_item(ctx.menu_obj_torrents_result.menu));
          if (!selected_torrent) {
            fprintf(log_file, "[!] Failed to get item ptr\n");
            continue;
          }
          ctx.dirty_menu = true;
          ctx.dirty_description = true;
        }
        if(ctx.current_windows_state == DOWNLOADED_MOVIES){
          menu_driver(ctx.menu_obj_movies_downloaded.menu, REQ_UP_ITEM);
          ctx.dirty_menu_downloaded_movies = true;
        }
        break;
      case KEY_DOWN:
        if (ctx.current_windows_state == RESULTS) {
          menu_driver(ctx.menu_obj_torrents_result.menu, REQ_DOWN_ITEM);
          selected_torrent = item_userptr(current_item(ctx.menu_obj_torrents_result.menu));
          if (!selected_torrent) {
            fprintf(log_file, "[!] Failed to get item ptr\n");
            continue;
          }
          ctx.dirty_menu = true;
          ctx.dirty_description = true;
        }
        if(ctx.current_windows_state == DOWNLOADED_MOVIES){
          menu_driver(ctx.menu_obj_movies_downloaded.menu, REQ_DOWN_ITEM);
          ctx.dirty_menu_downloaded_movies = true;
        }
        break;
      case KEY_LEFT:
        if (ctx.current_windows_state != 0) {
          if(!ctx.is_result){
            ctx.current_windows_state = SEARCH;
          }else{
            ctx.current_windows_state --;
          }
          ctx.tab_changed = true;
        }
        break;
      case KEY_RIGHT:
        if ((ctx.current_windows_state < 3)) {
          if(!ctx.is_result){
            ctx.current_windows_state = DOWNLOADED_MOVIES;
          }else{
            ctx.current_windows_state ++;
          }
          ctx.tab_changed = true;
        }
        break;
      case KEY_NPAGE:
        if(ctx.current_windows_state == RESULTS){
          menu_driver(ctx.menu_obj_torrents_result.menu, REQ_SCR_DPAGE);
        }
        if(ctx.current_windows_state == DOWNLOADED_MOVIES){
          menu_driver(ctx.menu_obj_movies_downloaded.menu, REQ_SCR_DPAGE);
        }
        break;
      case KEY_PPAGE:
      if(ctx.current_windows_state == RESULTS){
          menu_driver(ctx.menu_obj_torrents_result.menu, REQ_SCR_UPAGE);
        }
        if(ctx.current_windows_state == DOWNLOADED_MOVIES){
          menu_driver(ctx.menu_obj_movies_downloaded.menu, REQ_SCR_UPAGE);
        }
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
          torrents_cleanup(torrents_list);
          torrents_list = NULL;
          torrents_list = search(curl_handle_rutracker, user_input, log_file);
          memset(user_input,0,(size_t)size_user_input);
          size_user_input = 0;
          if (!torrents_list) {
            ctx.dirty_search = true;
            continue;
          }
          gui_menu_cleanup(&ctx.menu_obj_torrents_result);
          if (gui_create_menu(torrents_list, &ctx) != 0) {
            fprintf(log_file, "[!] Failed to create menu\n");
            goto cleanup;
          }
          if (!ctx.menu_obj_torrents_result.menu) {
            fprintf(log_file, "[!] Ctx menu not posted\n");
            goto cleanup;
          }
          ctx.is_result = true;
          ctx.tab_changed = true;
          ctx.current_windows_state = RESULTS;
          selected_torrent = item_userptr(current_item(ctx.menu_obj_torrents_result.menu));
          continue;
        }
        /* download the torrent file from rtorrent*/
        if (ctx.current_windows_state == RESULTS) {
          if (count_torrent_currently_downloading >= MAX_THREAD) {
            fprintf(log_file, "[!] Max 5 downloads at a time, please wait\n");
            ctx.current_windows_state = SEARCH;
            break;
          }
          selected_torrent = item_userptr(current_item(ctx.menu_obj_torrents_result.menu));
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
          thread_torrent = calloc(1,sizeof(struct thread_torrent));
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
        /* open mpv when selected a movie */
        if(ctx.current_windows_state == DOWNLOADED_MOVIES){

          if(!(dirent_mpv_movie = item_userptr(current_item(ctx.menu_obj_movies_downloaded.menu)))){
            fprintf(log_file, "[!] Failed to retrieve selected movie\n");
            continue;
          }
          if((pid_mpv = fork()) < 0){
            fprintf(log_file, "[!] Failed fork for mpv\n");
            continue;
          }
          const char* path_movies = getenv("download_path");
          if(!path_movies){
            fprintf(log_file, "[!] Failed to retrieve movie path\n");
            continue;
          }
          if(full_path_mpv_movie){
            free(full_path_mpv_movie);
            full_path_mpv_movie = NULL;
          }
          if(!(full_path_mpv_movie = alloc_format("%s/%s",path_movies,dirent_mpv_movie->d_name))){
            fprintf(log_file, "[!] Failed to allocate full path movies mpv\n");
            continue;
          }

          /* child */        
          if(pid_mpv == 0){
            int fd = open("/dev/null",O_WRONLY);
            dup2(fd,STDOUT_FILENO);
            dup2(fd,STDERR_FILENO);
            close(fd);
            if(execlp("mpv","mpv","--msg-level=all=no",full_path_mpv_movie,NULL) == -1){
              _exit(EXIT_FAILURE);
            }
            _exit(EXIT_SUCCESS);
          }
          
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
  gui_menu_cleanup(&ctx.menu_obj_torrents_result);
  gui_menu_cleanup(&ctx.menu_obj_movies_downloaded);
  downloaded_movies_list_cleanup(dl_movies_list);
  torrents_cleanup(torrents_list);
  if (log_file)
    fclose(log_file);

  /*cleanup handle */
  if (curl_handle_rutracker)
    curl_easy_cleanup(curl_handle_rutracker);
  if (curl_handle_qbittorrent)
    curl_easy_cleanup(curl_handle_qbittorrent);
  if (curl_handle_sftp)
    curl_easy_cleanup(curl_handle_sftp);

  /*cleanup gui */
  delwin(ctx.win_search_bar);
  delwin(ctx.win_description);
  delwin(ctx.win_menu);
  delwin(ctx.win_tabs);
  delwin(ctx.win_menu_downloaded_movies);
  endwin();
  curl_global_cleanup();


  return 0;
}
