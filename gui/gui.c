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

int gui_create_window_search_bar(struct ctx *ctx){
    int width,height;
    getmaxyx(stdscr,height,width);
    WINDOW *win_search_bar = newwin(SB_HEIGHT, SB_WIDTH,2,(width - SB_WIDTH)/2);
    if(win_search_bar == NULL){
        fprintf(stderr,"[!] Failed to create window\n");
        return -1;
    }
    ctx->win_search_bar = win_search_bar;
    return 0;
  
}

void gui_draw_search_bar(struct ctx *ctx){
    werase(ctx->win_search_bar);
    box(ctx->win_search_bar,0,0);
    mvwprintw(ctx->win_search_bar,0,1,"search");
    wmove(ctx->win_search_bar, 1, 1);
    wrefresh(ctx->win_search_bar);
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
        count ++;
        cp_tl = cp_tl->next;
    }
    ctx->items[ctx->nb_items] = NULL;
    ctx->menu = new_menu((ITEM **)ctx->items);
    if(!ctx->menu){
        code_function_return = -1;
        goto Cleanup;
    }

    //create window
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

