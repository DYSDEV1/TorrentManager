#include "utils.h"

struct torrent* parse(struct curl_response *curl_res){
    char* cp_html = curl_res->html;
    char* clean_string = NULL;
    struct torrent *head = NULL;
    int code_function_return = 0;
    int count = 0;
    
    char* line = curl_res->html;
    while((line = strstr(cp_html,HTML_INDICATOR)) != NULL){
        if(count >= 20){
            break;
        }
        struct torrent *curr = malloc(sizeof(struct torrent));
        if(!curr){
            code_function_return = -1;
            goto Cleanup;
        }
        curr->id = malloc(MAX_ID_SIZE);
        if(!curr->id){
            free(curr);
            code_function_return = -1;
            goto Cleanup;
        }
        curr->information = malloc(MAX_INFO_SIZE);
        if(!curr->information){
            free(curr);
            code_function_return = -1;
            goto Cleanup;
        }
        if(sscanf(line,SSCANF_EXTRACT_STRING,curr->id,curr->information) != 2){
            if(curr->id)
                free(curr->id);
            if(curr->information)
                free(curr->information);
            free(curr);
            continue;
        }
        clean_string = (char*)remove_non_utf8_char(curr->information);
        if(clean_string){
            free(curr->information);
            curr->information = clean_string;
        }
        curr->next = head;
        head = curr;
        cp_html = line;
        cp_html += strlen(HTML_INDICATOR);
        count ++;
   
    }

    Cleanup: 
        if(code_function_return == -1){
            while(head != NULL){
                struct torrent *next = head->next;
                free(head->id);
                free(head->information);
                free(head);
                head = next;
            }
        }

    return head;
}


void torrents_cleanup(struct torrent *torrents_list){
    if(torrents_list){
        while(torrents_list != NULL){
            struct torrent *next = torrents_list->next;
            free(torrents_list->information);
            free(torrents_list->id);
            free(torrents_list);
            torrents_list = next;
        }
    }
}

char* remove_non_utf8_char(char* str){
    char *clean_str = malloc(strlen(str) +1);
    if(!clean_str){
        return NULL;
    }
    size_t pos = 0;
    while(*str != '\0'){
        char c = *str;
        if(c >= 0x20 && c <= 0x7E){
            clean_str[pos++] = c;
        }
        str++;
    }
    clean_str[pos] = '\0';
    return clean_str;
}

