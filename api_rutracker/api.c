#include "api.h"

int saveHtmlToFile(FILE *html_file,struct curl_response *curl_res){
    html_file = fopen("html_save.html","w");
    if(html_file == NULL){
        return -1;
    }

    fwrite(curl_res->html,curl_res->size,1,html_file);
    fclose(html_file);
    return 0;
}


bool isLogged(CURL *curl_handle){
    struct curl_slist *cookies;
    long expiration_date;
    curl_easy_getinfo(curl_handle,CURLINFO_COOKIELIST, &cookies);
    struct curl_slist *cookies_ptr = cookies;
    while(cookies != NULL){
        if(strstr(cookies->data,"bb_session") != 0){
            if(sscanf(cookies->data,"%*s %*s %*s %*s %ld",&expiration_date) != 1){
                return false;
            }
            if(expiration_date > (long)time(NULL)){
                curl_slist_free_all(cookies_ptr);
                return true;
            
            }
        }
        cookies = cookies->next;
    }
    curl_slist_free_all(cookies_ptr);
    return false;
}



static size_t handleCurlResponse(char *data, size_t size, size_t nmemb, void *clientp){
    size_t real_size = nmemb;
    struct curl_response *curl_res = (struct curl_response *) clientp;

    char* ptr_response = realloc(curl_res->html,curl_res->size + real_size + 1);
    if(!ptr_response){
        return 0;
    }

    curl_res->html = ptr_response;
    memcpy(&curl_res->html[curl_res->size],data,real_size);
    curl_res->size += real_size;
    curl_res->html[real_size] = 0;

    return real_size;
}



int authenticate(CURL *curl_handle,const char* user,const char* password, FILE *log_file){

    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    CURLcode code_function_return;
    char* fmt= "login_username=%s&login_password=%s&login=%s";
    char* postfields;
    FILE *html_file;

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEFILE,"cookies.txt") == CURLE_OK)){
        if(curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "RELOAD") != CURLE_OK){
            fprintf(log_file, "[!] Failed to reload cookies\n");
        }
        if(isLogged(curl_handle)){
            fprintf(log_file,"[+] Already logged, cookies still actives\n");
            return CURLE_OK;
        }
    }else{
    
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,ENDPOINT_LOGIN) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set url.\n");
        return code_function_return;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        return code_function_return;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set result structure\n");
        return code_function_return;
    }
    int nb_required_bytes = snprintf(NULL,0,fmt,user,password,LOGIN_STRING_ENCODED);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc postfield buffer.\n");
        return -1;
    }
    postfields = (char*)malloc(nb_required_bytes + 1);

    if(!postfields){
        fprintf(log_file, "[!] Failed to alloc postfield buffer.\n");
        return -1;
    }
    snprintf(postfields,nb_required_bytes+1,fmt,user,password,LOGIN_STRING_ENCODED);


    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,postfields) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set postfields\n");
        return code_function_return;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEJAR,"cookies.txt") != CURLE_OK)){
        fprintf(log_file, "[!] Failed to set cookies\n");
        return code_function_return;
    }

    code_return_curl_request = curl_easy_perform(curl_handle);

    if(code_return_curl_request == CURLE_OK){
        saveHtmlToFile(html_file,&curl_res);
    }

    free(curl_res.html);

    if(isLogged(curl_handle)){
        fprintf(log_file,"[+] Login successful\n");
    }
    else{
        fprintf(log_file,"[-] Failed to login\n");
    }

    return (int) code_return_curl_request;

}





int search(CURL *curl_handle,const char* search_string,FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    CURLcode code_function_return;
    FILE *html_file;
    char* url_search;
    char* fmt = "https://rutracker.org/forum/tracker.php?nm=%s";

    if(!isLogged(curl_handle)){
        if(authenticate(curl_handle,getenv("username_rutracker"),getenv("password_rutracker"),log_file) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            return -1;
        }   
    }

    int nb_required_bytes = snprintf(NULL,0,fmt,search_string);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url buffer.\n");
        return -1;
    }
    url_search = (char*)malloc(nb_required_bytes + 1);

    if(!url_search){
        fprintf(log_file, "[!] Failed to alloc url buffer.\n");
        return -1;
    }
    snprintf(url_search,nb_required_bytes+1,fmt,search_string);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url_search) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set url.\n");
        return code_function_return;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        return code_function_return;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res) != CURLE_OK)){
        fprintf(log_file,"[!] Failed to set result structure\n");
        return code_function_return;
    }
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    code_return_curl_request = curl_easy_perform(curl_handle);

    if(code_return_curl_request != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        return code_function_return;
    }
    struct torrent *torrents_list = parse(&curl_res);
    while(torrents_list != NULL){
        printf("id : %d, info:%s\n",torrents_list->id,torrents_list->information);
        torrents_list = torrents_list->next;
    }

    free(curl_res.html);

    return 0;

}


