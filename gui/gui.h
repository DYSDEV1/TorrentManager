#ifndef GUI__H_
#define GUI__H_

#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../api/types.h"

#define SB_HEIGHT 3
#define SB_WIDTH  40 
#define SB_Y 4
#define MAX_MENU_SIZE 8

enum win_state{
    SEARCH,
    RESULTS,
    DOWNLOADS
};


struct ctx{
    int height_max;
    int width_max;
    WINDOW *win_search_bar;
    WINDOW *win_description;
    WINDOW *win_notification;
    WINDOW *win_tabs;
    WINDOW *win_downloads;
    enum win_state current_windows_state;
    ITEM **items;
    MENU *menu;
    int nb_items;
};

void gui_init();
void gui_menu_cleanup(struct ctx *ctx);

int gui_create_window_search_bar(struct ctx *ctx);
int gui_create_window_description(struct ctx *ctx);
int gui_create_window_notification(struct ctx *ctx);
int gui_create_window_tabs(struct ctx *ctx);
int gui_create_window_downloads(struct ctx *ctx);



void gui_draw_windows(struct ctx *ctx);
void gui_draw_search_bar(struct ctx *ctx);
void gui_draw_window_description(struct ctx *ctx);
void gui_draw_window_notification(struct ctx *ctx, const char* notification);
void gui_draw_window_tabs(struct ctx *ctx);
void gui_draw_downloads(struct ctx *ctx);

void gui_clear_windows(struct ctx *ctx);

int gui_use_search_bar(struct ctx *ctx);

int gui_create_menu(struct torrent *torrents_list,struct ctx *ctx);


#endif