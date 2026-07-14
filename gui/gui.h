#ifndef GUI__H_
#define GUI__H_

#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "../api/types.h"


#define SB_HEIGHT 3
#define SB_WIDTH  40 
#define SB_Y 4
#define MAX_MENU_SIZE 6
#define MAX_MENU_SIZE_DOWNLOADED_MOVIES 12


enum win_state{
    SEARCH,
    RESULTS,
    DOWNLOADS,
    DOWNLOADED_MOVIES
};

struct menu_objects{
    ITEM **items;
    MENU *menu;
    int nb_items;
};

struct ctx{
    int height_max;
    int width_max;
    WINDOW *win_search_bar;
    WINDOW *win_description;
    WINDOW *win_tabs;
    WINDOW *win_downloads;
    WINDOW *win_menu;
    WINDOW *win_menu_downloaded_movies;
    enum win_state current_windows_state;
    bool dirty_menu;
    bool dirty_search;
    bool dirty_description;
    bool dirty_tabs;
    bool dirty_downloads;
    bool dirty_menu_downloaded_movies;
    bool tab_changed;
    struct menu_objects menu_obj_torrents_result;
    struct menu_objects menu_obj_movies_downloaded;
    bool is_result;
};

int gui_init(struct ctx *ctx);
void gui_menu_cleanup(struct menu_objects *menu);

int gui_create_window_search_bar(struct ctx *ctx);
int gui_create_window_description(struct ctx *ctx);
int gui_create_window_notification(struct ctx *ctx);
int gui_create_window_tabs(struct ctx *ctx);
int gui_create_window_downloads(struct ctx *ctx);
int gui_create_window_menu(struct ctx *ctx);
int gui_create_window_menu_downloaded_movies(struct ctx *ctx);



void gui_clear_windows(struct ctx *ctx);

int gui_use_search_bar(struct ctx *ctx);

int gui_create_menu(struct torrent *torrents_list,struct ctx *ctx);

int gui_create_downloaded_movies_menu(struct downloaded_movies_list *movies_list, struct ctx *ctx);

void gui_render(struct ctx *ctx, struct torrent *curr_torrent, char *user_input, struct info_download_torrents *active_downloads[MAX_THREAD], int count_torrent_currently_downloading);

#endif