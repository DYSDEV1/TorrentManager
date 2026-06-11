#include "utils.h"

struct torrent* parse(struct curl_response *curl_res){
    char* html = curl_res->html;
    struct torrent *head = NULL;
    
    char* torrent_line;
    while((torrent_line = strstr(curl_res->html,HTML_INDICATOR)) != NULL){
        struct torrent *curr = malloc(sizeof(struct torrent));
        if(!curr) return NULL;

        curr->information = malloc(MAX_INFO_SIZE);
        if(!curr->information){
            free(curr);
            return NULL;
        }
        curr->next = head;
        head = curr;
        sscanf(torrent_line,SSCANF_EXTRACT_STRING,&curr->id,curr->information);
        curl_res->html += strlen(HTML_INDICATOR);
   
    }
    return head;
}
