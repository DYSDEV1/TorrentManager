#include "gui.h"


void gui_init(){
    initscr();	
    refresh();
	cbreak();			
	keypad(stdscr, TRUE);		
}

void gui_cleanup(struct ctx *ctx){
    delwin(ctx->win_search_bar);
    delwin(ctx->win_torrent_list);
    endwin();
}

int gui_create_window_search_bar(struct ctx *ctx){
    int width, height;
    getmaxyx(stdscr,height,width);
    WINDOW *win_search_bar = newwin(SB_HEIGHT, SB_WIDTH,2,(width - SB_WIDTH)/2);
    if(win_search_bar == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_search_bar = win_search_bar;
    box(win_search_bar,0,0);
    mvwprintw(win_search_bar,0,1,"search");
    wmove(win_search_bar, 1, 1);

    return 0;
  
}



int gui_create_window_torrent_list(struct ctx *ctx){
    int width, height;
    getmaxyx(stdscr,height,width);
    WINDOW *win_torrent_list = newwin((height - SB_HEIGHT-2),width,SB_Y+2,0);
    if(win_torrent_list == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_torrent_list = win_torrent_list;
    box(win_torrent_list,0,0);
    mvwprintw(win_torrent_list,0,1,"torrent list");
    return 0;
   
}
void gui_draw_windows(struct ctx *ctx){
    wrefresh(ctx->win_torrent_list);
    wrefresh(ctx->win_search_bar);
    ctx->current_windows_state = SEARCH;
}


