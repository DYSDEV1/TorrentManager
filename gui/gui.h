#ifndef GUI__H_
#define GUI__H_

#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include "../api_rutracker/types.h"

#define SB_HEIGHT 3
#define SB_WIDTH  40 
#define SB_Y 4

enum win_state{
    SEARCH,
    LIST_TORRENTS
};


struct ctx{
    WINDOW *win_search_bar;
    WINDOW *win_torrent_list;
    enum win_state current_windows_state;
    ITEM **items;
    MENU *menu;
    int nb_items;
};

void gui_init();
void gui_cleanup(struct ctx *ctx);

int gui_create_window_search_bar(struct ctx *ctx);
int gui_create_window_torrent_list(struct ctx *ctx);

void gui_draw_windows(struct ctx *ctx);

int gui_use_search_bar(struct ctx *ctx);

void gui_create_menu(struct torrent *torrents_list,struct ctx *ctx);


#endif