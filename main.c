#include "api/api.h"
#include "api/env.h"
#include "gui/gui.h"
#include <pthread.h>

#define MAX_INPUT_SIZE 256

int main(){
    FILE *log_file;
    struct ctx ctx = {0};
    int ch;
    char user_search_input[MAX_INPUT_SIZE];
    bool exit = false;
    struct torrent *torrents_list = NULL;

    log_file = fopen("logs.txt","w");
    if(log_file == NULL){
        goto cleanup;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle_rutracker = curl_easy_init();
    if(!curl_handle_rutracker){
        fprintf(log_file,"[!] Failed to create curl handle for rutracker\n");
        goto cleanup;
    }
    CURL *curl_handle_qbitorrent = curl_easy_init();
    if(!curl_handle_qbitorrent){
        fprintf(log_file,"[!] Failed to create curl handle for qbitorrent\n");
        goto cleanup;
    }
    CURL *curl_handle_sftp = curl_easy_init();
    if(!curl_handle_sftp){
        fprintf(log_file,"[!] Failed to create curl handle for sftp\n");
        goto cleanup;
    }

    if(loadEnv() != 0){
        fprintf(log_file, "[!] Failed to set env variables\n");
        goto cleanup;
    }
    
    if(authenticate(curl_handle_rutracker,FMT_RUTRACKER,getenv("username_rutracker"),getenv("password_rutracker"),log_file, COOKIE_FILENAME_RUTRACKER, ENDPOINT_LOGIN_RUTRACKER,COOKIE_RUTRACKER) != 0){
        fprintf(log_file,"[!] Failed to authenticate to rutorrent\n");
        goto cleanup;
    }

    if(authenticate(curl_handle_qbitorrent,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file,COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
        fprintf(log_file,"[!] Failed to authenticate to qbitorrent\n");
        goto cleanup;
    }



    gui_init();
    getmaxyx(stdscr,ctx.height_max,ctx.width_max);

    if(gui_create_window_search_bar(&ctx) != 0){
        goto cleanup;
    }
    if(gui_create_window_description(&ctx) != 0){
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
            torrents_list = search(curl_handle_rutracker,user_search_input, log_file);
            if(!torrents_list){
                goto cleanup;
            }
            ctx.current_windows_state = LIST_TORRENTS;
            if(gui_create_menu(torrents_list,&ctx) != 0){
                fprintf(log_file, "[!] Failed to create menu\n");
                goto cleanup;
            }
        }
        gui_draw_window_description(&ctx);
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
                struct torrent *torrent = item_userptr(current_item(ctx.menu));
                if(!torrent){
                    fprintf(log_file,"[!] Failed to get item ptr\n");
                    goto cleanup;
                }            
                if(download(curl_handle_rutracker,torrent->id,log_file) != 0){
                    fprintf(log_file,"[!] Failed to download torrent\n");
                    goto cleanup;
                }
                if(uploadToServer(curl_handle_qbitorrent,torrent->id,log_file) != 0){
                    fprintf(log_file,"[!] Failed to upload to qbittorrent\n");
                    goto cleanup;
                }
                if(retrieveTorrentInfo(curl_handle_qbitorrent,torrent->id,torrent->name,torrent->hash,torrent->full_path,log_file) != 0){
                    fprintf(log_file,"[!] Failed to retrieve hash\n");
                    goto cleanup;
                }
                if(retrieveUploadProgression(curl_handle_qbitorrent,torrent->hash,log_file) != 0){
                    fprintf(log_file,"[!] Failed to retrieve torrent upload progression\n");
                    goto cleanup;
                }
                if(downloadFromServer(curl_handle_sftp,torrent->name,torrent->full_path,log_file) != 0){
                    fprintf(log_file,"[!] Failed to download file from sftp\n");
                    goto cleanup;
                }

                ctx.current_windows_state = SEARCH;
                break;
            default:
        }
        ITEM *item = current_item(ctx.menu);
        struct torrent *torrent = item_userptr(item);
        if(!torrent){
            fprintf(log_file,"[!] Failed to get item ptr\n");
            goto cleanup;
        }
        wclear(ctx.win_description);
        mvwprintw(ctx.win_description, 1, 1, " Description: %s \n seeders:%s\n size:%s",torrent->information, torrent->seeders,torrent->size);
 
    }

    cleanup:
        gui_menu_cleanup(&ctx);
        torrents_cleanup(torrents_list);
        if(log_file)
            fclose(log_file);
        if(curl_handle_rutracker)
            curl_easy_cleanup(curl_handle_rutracker);
        if(curl_handle_qbitorrent)
            curl_easy_cleanup(curl_handle_qbitorrent);
        if(curl_handle_sftp)
            curl_easy_cleanup(curl_handle_sftp);
        delwin(ctx.win_search_bar);
        ctx.win_search_bar = NULL;
        delwin(ctx.win_description);
        ctx.win_description = NULL;
        endwin();
        curl_global_cleanup();
    
    return 0; 
}