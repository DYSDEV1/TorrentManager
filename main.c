#include "api_rutracker/api.h"
#include "api_rutracker/env.h"
#include "gui/gui.h"

#define MAX_INPUT_SIZE 256

int main(){
    FILE *html_saved;
    FILE *log_file;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();
    struct ctx ctx;
    char ch;
    char user_search_input[MAX_INPUT_SIZE];
    int char_count = 0;
    bool exit = false;

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
    if(gui_create_window_search_bar(&ctx) != 0){
        goto cleanup;
    }
    if(gui_create_window_torrent_list(&ctx) != 0){
        goto cleanup;
    }
    gui_draw_windows(&ctx);

    while(!exit){
        if(ctx.current_windows_state == SEARCH){
            wgetnstr(ctx.win_search_bar,user_search_input,MAX_INPUT_SIZE);
            search(curl_handle,user_search_input, log_file);
            ctx.current_windows_state = LIST_TORRENTS;
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
                break;
            case KEY_UP:
                break;
            case '\n':
            case '\r':
                break;
            default:
        }

        
    }

    cleanup:
        gui_cleanup(&ctx);
        if(log_file)
            fclose(log_file);
        if(curl_handle)
            curl_easy_cleanup(curl_handle);
    
    return 0; 
}