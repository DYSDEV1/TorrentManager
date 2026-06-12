#include "utils.h"

struct torrent* parse(struct curl_response *curl_res){
    char* cp_html = curl_res->html;
    struct torrent *head = NULL;
    int code_function_return = 0;
    
    char* line;
    while((line = strstr(cp_html,HTML_INDICATOR)) != NULL){
        struct torrent *curr = malloc(sizeof(struct torrent));
        if(!curr){
            code_function_return = -1;
            goto Cleanup;
        }
        curr->information = malloc(MAX_INFO_SIZE);
        if(!curr->information){
            free(curr);
            code_function_return = -1;
            goto Cleanup;
        }
        if(sscanf(line,SSCANF_EXTRACT_STRING,&curr->id,curr->information) != 2){
            free(curr->information);
            free(curr);
            continue;
        }
        curr->next = head;
        head = curr;
        cp_html += strlen(HTML_INDICATOR);
   
    }

    Cleanup: 
        if(code_function_return == -1){
            while(head != NULL){
                free(head->information);
                head = head->next;
            }
        }

    return head;
}


void torrents_cleanup(struct torrent *torrents_list){
    if(torrents_list){
        while(torrents_list != NULL){
            struct torrent *next = torrents_list->next;
            free(torrents_list->information);
            free(torrents_list);
            torrents_list = next;
        }
    }
}