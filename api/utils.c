#include "utils.h"



void free_torrent(struct torrent *torrent){
    if(!torrent) return;
    curl_easy_cleanup(torrent->curl_handle);
    free(torrent->id);
    free(torrent->information);
    free(torrent->seeders);
    free(torrent->size);
    free(torrent);
    
}


struct torrent* parse(struct curl_response *curl_res){
    char* cp_html = curl_res->html;
    char* cp_html2 = curl_res->html;
    char* cp_html3 = curl_res->html;
    char* clean_string = NULL;
    struct torrent *head = NULL;
    struct torrent *tail = NULL;
    char* line = curl_res->html;
    char* line2 = curl_res->html;
    char* line3 = curl_res->html;
    char size[SIZE_ARRAY];
    char unit_size[SIZE_UNIT_ARRAY];
    while((line = strstr(cp_html,HTML_INDICATOR_DESC)) != NULL){
        memset(size,0,SIZE_ARRAY);
        memset(unit_size,0,SIZE_UNIT_ARRAY);
        struct torrent *curr = calloc(1,sizeof(struct torrent));
        if(!curr){
            continue;
        }
        curr->id = malloc(MAX_ID_SIZE);
        if(!curr->id){
            free_torrent(curr);
            continue;
        }
        curr->information = malloc(MAX_INFO_SIZE);
        if(!curr->information){
            free_torrent(curr);
            continue;
        }
        curr->seeders = malloc(MAX_SEEDERS_SIZE);
        if(!curr->seeders){
            free_torrent(curr);
            continue;
        }
        curr->size = malloc(MAX_SIZE_SIZE);
        if(!curr->size){
            free_torrent(curr);
            continue;
        }

        if(sscanf(line,SSCANF_EXTRACT_DESC,curr->id,curr->information) != 2){
            free_torrent(curr);
            continue;
        }
        line2 = strstr(cp_html2,HTML_INDICATOR_SEEDERS);
        if(!line2 || (sscanf(line2,SSCANF_EXTRACT_SEEDERS,curr->seeders) != 1)){
            curr->seeders[0] = '\0';
        }
        line3 = strstr(cp_html3,HTML_INDICATOR_SIZE);
        if(!line3 || (sscanf(line3,SSCANF_EXTRACT_SIZE,size,unit_size) != 2)){
            snprintf(curr->size,MAX_SIZE_SIZE,"unknown");
        }else{
            snprintf(curr->size,MAX_SIZE_SIZE,"%s %s",size,unit_size);
        }
        

        clean_string = (char*)remove_non_utf8_char(curr->information);
        if(clean_string){
            free(curr->information);
            curr->information = clean_string;
        }
        if(!head){
            head = curr;
            tail = curr;
        }else{
            tail->next = curr;
            tail = curr;
        }
        cp_html = line;
        if(!line2){
            cp_html2 = line;
        }else{
            cp_html2 = line2;
        }
        if(!line3){
            cp_html3 = line;
        }else{
            cp_html3 = line3;
        }
        cp_html2 += strlen(HTML_INDICATOR_SEEDERS);
        cp_html += strlen(HTML_INDICATOR_DESC);
        cp_html3 += strlen(HTML_INDICATOR_SIZE);
   
    }

    return head;
}


void torrents_cleanup(struct torrent *torrents_list){
    while(torrents_list != NULL){
            struct torrent *next = torrents_list->next;
            free_torrent(torrents_list);
            torrents_list = next;
        }
}


char* remove_non_utf8_char(char* str){
    char *clean_str = malloc(strlen(str) +1);
    if(!clean_str){
        return NULL;
    }
    size_t pos = 0;
    while(*str != '\0'){
        unsigned char c = (unsigned char)*str;
        if(c >= 0x20 && c <= 0x7E){
            clean_str[pos++] = c;
        }
        str++;
    }
    clean_str[pos] = '\0';
    return clean_str;
}


