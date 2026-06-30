#include "gui.h"


void gui_init(){
    initscr();	
    refresh();
	cbreak();			
	keypad(stdscr, TRUE);		
}

void gui_menu_cleanup(struct ctx *ctx){
    if(ctx->menu){
        unpost_menu(ctx->menu);
        free_menu(ctx->menu);
        ctx->menu = NULL;
    }
    ctx->menu = NULL;
    if(ctx->items){
        for(int i = 0; i < ctx->nb_items;i++){
            if(ctx->items[i]){
                free_item(ctx->items[i]);
            }
        }
        free(ctx->items);
        ctx->items = NULL;
        ctx->nb_items = 0;
    }
}

int gui_create_window_notification(struct ctx *ctx){
    WINDOW *win_notif = newwin(1,ctx->width_max,ctx->height_max - 1,0);
    if(win_notif == NULL){
        fprintf(stderr, "[!] Failed to create window\n");
        return -1;
    }
    ctx->win_notification = win_notif;
    return 0;

}

int gui_create_window_search_bar(struct ctx *ctx){
    WINDOW *win_search_bar = newwin(SB_HEIGHT, SB_WIDTH,2,(ctx->width_max - SB_WIDTH)/2);
    if(win_search_bar == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_search_bar = win_search_bar;
    return 0;
  
}

int gui_create_window_description(struct ctx *ctx){
    WINDOW *win_desc = newwin(7,ctx->width_max,ctx->height_max - 8,0);
    if(win_desc == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_description = win_desc;
    return 0;
}

int gui_create_window_tabs(struct ctx *ctx){
    WINDOW *win_tabs = newwin(1,ctx->width_max,ctx->height_max - 1,0);
    if(win_tabs == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_tabs = win_tabs;
    return 0;

}

int gui_create_window_downloads(struct ctx *ctx){
    WINDOW *win_downloads = newwin(ctx->height_max-1,ctx->width_max,0,0);
    if(win_downloads == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_downloads = win_downloads;
    return 0;
}


void gui_draw_window_tabs(struct ctx *ctx){
    if(ctx->current_windows_state == SEARCH){
        wattr_on(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
        mvwprintw(ctx->win_tabs,0,1,"Search");
        wattr_off(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
    }else{
        mvwprintw(ctx->win_tabs,0,1,"Search");
    }
    if(ctx->current_windows_state == RESULTS){
        wattr_on(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
        mvwprintw(ctx->win_tabs,0,8,"Results");
        wattr_off(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
    }else{
        mvwprintw(ctx->win_tabs,0,8,"Results");
    }
    if(ctx->current_windows_state == DOWNLOADS){
        wattr_on(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
        mvwprintw(ctx->win_tabs,0,16,"Downloads");
        wattr_off(ctx->win_tabs,(A_BOLD|A_UNDERLINE),NULL);
    }else{
        mvwprintw(ctx->win_tabs,0,16,"Downloads");
    }
    wrefresh(ctx->win_tabs);
}
void gui_draw_window_description(struct ctx *ctx){
    box(ctx->win_description,0,0);
    mvwprintw(ctx->win_description,0,1,"Informations");
    wrefresh(ctx->win_description);
}

void gui_draw_window_notification(struct ctx *ctx, const char* notification){
    mvwprintw(ctx->win_notification,0,1,"%s",notification);
    wrefresh(ctx->win_notification);
}

void gui_draw_search_bar(struct ctx *ctx){
    box(ctx->win_search_bar,0,0);
    mvwprintw(ctx->win_search_bar,0,1,"search");
    wmove(ctx->win_search_bar, 1, 1);
    wrefresh(ctx->win_search_bar);
}

void gui_draw_downloads(struct ctx *ctx){
    box(ctx->win_downloads,0,0);
    mvwprintw(ctx->win_downloads,0,1,"Downloads");
    wrefresh(ctx->win_downloads);
}

void gui_clear_windows(struct ctx *ctx){
    if(ctx->current_windows_state == SEARCH){
        werase(stdscr);
        refresh();
        werase(ctx->win_description);
        wrefresh(ctx->win_description);
        werase(ctx->win_downloads);
        wrefresh(ctx->win_downloads);
    }
    if(ctx->current_windows_state == RESULTS){
        werase(ctx->win_search_bar);
        wrefresh(ctx->win_search_bar);
        werase(ctx->win_downloads);
        wrefresh(ctx->win_downloads);
    }
    if(ctx->current_windows_state == DOWNLOADS){
        werase(ctx->win_search_bar);
        wrefresh(ctx->win_search_bar);
        werase(stdscr);
        refresh();
        werase(ctx->win_description);
        wrefresh(ctx->win_description);

    }
}



int gui_create_menu(struct torrent *torrents_list,struct ctx *ctx){
    int count = 0;
    ctx->nb_items = 0;
    int code_function_return = 0;
    int width,height;
    struct torrent *cp_tl = torrents_list;
    while(cp_tl != NULL){
        ctx->nb_items ++;
        cp_tl = cp_tl->next;
    }
    ctx->items = (ITEM**) calloc((size_t)(ctx->nb_items+1),sizeof(ITEM *));
    if(!ctx->items){
        code_function_return = -1;
        goto Cleanup;
    }
    cp_tl = torrents_list;
    while(cp_tl != NULL){
        ctx->items[count] = new_item(cp_tl->id, cp_tl->information);
        if(!ctx->items[count]){
            code_function_return = -1;
            goto Cleanup;
        }
        set_item_userptr(ctx->items[count],cp_tl);
        count ++;
        cp_tl = cp_tl->next;
    }
    ctx->items[ctx->nb_items] = NULL;
    ctx->menu = new_menu((ITEM **)ctx->items);
    if(!ctx->menu){
        code_function_return = -1;
        goto Cleanup;
    }

    set_menu_format(ctx->menu,MAX_MENU_SIZE,1);
    post_menu(ctx->menu);
    refresh();

    Cleanup:
        if(code_function_return != 0){
            if(ctx->items){
                for(int i = 0; i < ctx->nb_items;i++){
                    if(ctx->items[i]){
                        free_item(ctx->items[i]);
                    }
                }
                free(ctx->items);
                ctx->items = NULL;
                ctx->nb_items = 0;
            }
            if(ctx->menu){
                unpost_menu(ctx->menu);
                free_menu(ctx->menu);
                ctx->menu = NULL;
            }
        }

    return code_function_return;

}

