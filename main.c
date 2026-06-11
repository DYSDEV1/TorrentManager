#include "api_rutracker/api.h"
#include "api_rutracker/env.h"
#include "gui/gui.h"



int main(){

    FILE *html_saved;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();

    /*
    if(loadEnv() != 0){
        fprintf(stderr, "[!] Failed to set env variables\n");
        return -1;
    }

    if(authenticate(curl_handle,getenv("username_rutracker"),getenv("password_rutracker")) != 0){
        fprintf(stderr,"[!] Failed to authenticate");
        return -1;
    }
    */
    curl_easy_cleanup(curl_handle);

    return 0; 
}