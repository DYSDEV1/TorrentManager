#include "api/api.h"
#include "api/env.h"
#include "gui/gui.h"

#define MAX_INPUT_SIZE 256

int main(){
    FILE *log_file;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();
    struct ctx ctx = {0};
    int ch;
    char user_search_input[MAX_INPUT_SIZE];
    bool exit = false;
    struct torrent *torrents_list = NULL;
    log_file = fopen("logs.txt","w");

    if(log_file == NULL){
        goto cleanup;
    }

    if(loadEnv() != 0){
        fprintf(log_file, "[!] Failed to set env variables\n");
        goto cleanup;
    }

    if(authenticate(curl_handle,getenv("username_rutracker"),getenv("password_rutracker"),log_file) != 0){
        fprintf(log_file,"[!] Failed to authenticate");
        goto cleanup;
    }


    gui_init();
    int max_height,max_width;
    getmaxyx(stdscr,max_height,max_width);
    if(gui_create_window_search_bar(&ctx) != 0){
        goto cleanup;
    }

    while(!exit){
        if(ctx.current_windows_state == SEARCH){
            gui_menu_cleanup(&ctx);
            refresh();
            gui_draw_search_bar(&ctx);  
            torrents_cleanup(torrents_list);
            torrents_list = NULL;
            wgetnstr(ctx.win_search_bar,user_search_input,MAX_INPUT_SIZE);
            torrents_list = search(curl_handle,user_search_input, log_file);
            if(!torrents_list){
                goto cleanup;
            }
            ctx.current_windows_state = LIST_TORRENTS;
            if(gui_create_menu(torrents_list,&ctx) != 0){
                fprintf(log_file, "[!] Failed to create menu\n");
                goto cleanup;
            }
        }
        ch  = getch();
        switch(ch){
            case '\x1b':
                exit = true;
                break;
            case '\t':
                ctx.current_windows_state = SEARCH;
                break;
            case KEY_DOWN:
                menu_driver(ctx.menu,REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(ctx.menu,REQ_UP_ITEM);
                break;
            case KEY_NPAGE:
				menu_driver(ctx.menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(ctx.menu, REQ_SCR_UPAGE);
				break;
            case 10: 
                download(curl_handle,item_name(current_item(ctx.menu)),log_file);
                ctx.current_windows_state = SEARCH;
                break;
            default:
        }
        mvprintw((max_height-2),3,item_description(current_item(ctx.menu)));
        refresh();

        
    }

    cleanup:
        gui_menu_cleanup(&ctx);
        torrents_cleanup(torrents_list);
        if(log_file)
            fclose(log_file);
        if(curl_handle)
            curl_easy_cleanup(curl_handle);
        delwin(ctx.win_search_bar);
        ctx.win_search_bar = NULL;
        delwin(ctx.win_torrent_list);
        ctx.win_torrent_list = NULL;
        endwin();
    
    return 0; 
}