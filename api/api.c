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

bool isLogged(CURL *curl_handle, enum cookie_type ct){
    struct curl_slist *cookies;
    long expiration_date;
    curl_easy_getinfo(curl_handle,CURLINFO_COOKIELIST, &cookies);
    struct curl_slist *cookies_ptr = cookies;
    while(cookies != NULL){
        if(ct == COOKIE_RUTRACKER){
            if(strstr(cookies->data,"bb_session") != 0){
                if(sscanf(cookies->data,"%*s %*s %*s %*s %ld",&expiration_date) != 1){
                    curl_slist_free_all(cookies_ptr);
                    return false;
                }
                if(expiration_date > (long)time(NULL)){
                    curl_slist_free_all(cookies_ptr);
                    return true;
                
                }
            }
        }
        if(ct == COOKIE_QBITTORRENT){
            if(strstr(cookies->data,"SID") != 0)
                return true;
        }

        cookies = cookies->next;
    }
    curl_slist_free_all(cookies_ptr);
    return false;
}



static size_t handleCurlResponse(char *data, size_t size, size_t nmemb, void *clientp){

   
    size_t real_size;
    size_t alloc_size;
    size_t tmp_size;

    struct curl_response *curl_res = (struct curl_response *) clientp;

    if(!curl_res){
        return 0;
    }

    if(ckd_mul(&real_size,size,nmemb)){
        return 0;
    }
    if(ckd_add(&tmp_size,curl_res->size,real_size)){
        return 0;
    }
    if(ckd_add(&alloc_size,tmp_size,1)){
        return 0;
    }
    if(alloc_size > MAX_RESPONSE_SIZE){
        return 0;
    }

    char* ptr_response = realloc(curl_res->html,alloc_size);
    if(!ptr_response){
        return 0;
    }

    curl_res->html = ptr_response;
    memcpy(&curl_res->html[curl_res->size],data,real_size);
    curl_res->size += real_size;
    curl_res->html[curl_res->size] = '\0';

    return real_size;
}


static size_t handleDownloadTorrent(char* ptr, size_t size, size_t nmemb,void* stream){
    if(!ptr || !size || !nmemb || !stream){
        return 0;
    }
    size_t written = fwrite(ptr,size,nmemb,(FILE *) stream);
    if(written == 0){
        return 0;
    }
    return written;
}


int authenticate(CURL *curl_handle,const char* fmt,const char* user,const char* password, FILE *log_file, const char* cookie_file_name, const char* endpoint_url, enum cookie_type ct){

    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    int code_function_return = 0;
    char *postfields = NULL;
    char *encoded_user = NULL;
    char *encoded_password = NULL;
    if(!user || !password){
        fprintf(log_file,"[!] Uer or pasword NULL\n");
        goto Cleanup;

    }
    if((encoded_user = curl_easy_escape(curl_handle,user,(int)strlen(user))) == NULL){
        fprintf(log_file,"[!] Failed to encode user\n");
        goto Cleanup;
    }
    if((encoded_password = curl_easy_escape(curl_handle,password,(int)strlen(password))) == NULL){
        fprintf(log_file,"[!] Failed to encode password\n");
        goto Cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEFILE,cookie_file_name)) == CURLE_OK){
        if(curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "RELOAD") != CURLE_OK){
            fprintf(log_file, "[!] Failed to reload cookies\n");
        }
        if(isLogged(curl_handle, ct)){
            fprintf(log_file,"[+] Already logged, cookies still actives\n");
            return CURLE_OK;
        }
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,endpoint_url)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        goto Cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto Cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto Cleanup;
    }
    int nb_required_bytes = snprintf(NULL,0,fmt,encoded_user,encoded_password,LOGIN_STRING_ENCODED);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc postfield buffer.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    postfields = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!postfields){
        fprintf(log_file, "[!] Failed to alloc postfield buffer.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    snprintf(postfields,(size_t)(nb_required_bytes+1),fmt,encoded_user,encoded_password,LOGIN_STRING_ENCODED);


    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,postfields)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set postfields\n");
        code_function_return = -1;
        goto Cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEJAR,cookie_file_name)) != CURLE_OK){
        fprintf(log_file, "[!] Failed to set cookies\n");
        code_function_return = -1;
        goto Cleanup;
    }

    code_return_curl_request = curl_easy_perform(curl_handle);

    if(code_return_curl_request != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute authenticate request.\n");
        code_function_return = -1;
        goto Cleanup;
    }


    if(isLogged(curl_handle,ct)){
        fprintf(log_file,"[+] Login successful\n");
    }
    else{
        fprintf(log_file,"[-] Failed to login\n");
        code_function_return = -1;
    }

    Cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(encoded_user)
            curl_free(encoded_user);
        if(encoded_password)
            curl_free(encoded_password);
        if(postfields)
            free(postfields);
        if(curl_res.html)
            free(curl_res.html);

    fflush(log_file);

    return (int) code_function_return;

}




struct torrent *search(CURL *curl_handle,const char* search_string,FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    int code_function_return = 0;
    char* url_search = NULL;
    char* encoded_search_string = NULL;

    if(!isLogged(curl_handle,COOKIE_RUTRACKER)){
        if(authenticate(curl_handle,FMT_RUTRACKER,getenv("username_rutracker"),getenv("password_rutracker"),log_file,COOKIE_FILENAME_RUTRACKER, ENDPOINT_LOGIN_RUTRACKER, COOKIE_RUTRACKER) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto Cleanup;
        }   
    }

    if((encoded_search_string = curl_easy_escape(curl_handle,search_string,(int)strlen(search_string))) == NULL){
        fprintf(log_file,"[!] Failed to encode url.\n");
        code_function_return = -1;
        goto Cleanup;
    }


    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_SEARCH,encoded_search_string);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    url_search = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!url_search){
        fprintf(log_file, "[!] Failed to alloc url buffer.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    snprintf(url_search,(size_t)(nb_required_bytes+1),ENDPOINT_SEARCH,encoded_search_string);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url_search)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto Cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto Cleanup;
    }

    code_return_curl_request = curl_easy_perform(curl_handle);

    if(code_return_curl_request != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto Cleanup;
    }
    struct torrent *torrents_list = parse(&curl_res);

    Cleanup:
        if(curl_res.html)
            free(curl_res.html);
        if(url_search)
            free(url_search);
        if(encoded_search_string)
            curl_free(encoded_search_string);
       
    fflush(log_file);
    if(code_function_return == -1){
        return NULL;
    }else{
        return torrents_list;
    }

}

int download(CURL *curl_handle,const char *torrent_name, FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    int code_function_return = 0;
    char* url_download = NULL;
    FILE *torrent_file = NULL;

    if(!isLogged(curl_handle,COOKIE_RUTRACKER)){
        if(authenticate(curl_handle,FMT_RUTRACKER,getenv("username_rutracker"),getenv("password_rutracker"),log_file, COOKIE_FILENAME_RUTRACKER, ENDPOINT_LOGIN_RUTRACKER, COOKIE_RUTRACKER) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto Cleanup;
        }  
    }

    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_DOWNLOAD,torrent_name);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    url_download = malloc((size_t)(nb_required_bytes + 1));

    if(!url_download){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto Cleanup;
    }

    snprintf(url_download,(size_t)(nb_required_bytes+1),ENDPOINT_DOWNLOAD,torrent_name);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url_download)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleDownloadTorrent)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto Cleanup;
    }

    torrent_file = fopen(torrent_name,"wb");
    if(!torrent_file){
        fprintf(log_file, "[!] Failed to create torrent file\n");
        code_function_return = -1;
        goto Cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)torrent_file)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto Cleanup;
    }

    code_return_curl_request = curl_easy_perform(curl_handle);

    if(code_return_curl_request != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto Cleanup;
    }

    Cleanup:
        if(torrent_file)
            fclose(torrent_file);
        if(curl_res.html)
            free(curl_res.html);
        if(url_download)
            free(url_download);
    
    fflush(log_file);
    return code_function_return;

}

int uploadToServer(CURL *curl_handle,char* torrent_name, FILE *log_file){
    CURLcode code_return_curl;
    struct curl_response curl_res = {0};
    int code_function_return = 0;
    curl_mime *mime = NULL;
    curl_mimepart *part = NULL;

    if(!isLogged(curl_handle,COOKIE_QBITTORRENT)){
        if(authenticate(curl_handle,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file, COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto Cleanup;
        }  
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,ENDPOINT_UPLOAD_TORRENT)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto Cleanup;
    }

    if((mime = curl_mime_init(curl_handle)) == NULL){
        fprintf(log_file,"[!] Failed to setup mime\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((part = curl_mime_addpart(mime)) == NULL){
        fprintf(log_file,"[!] Failed to setup mime\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((code_return_curl = curl_mime_filedata(part,torrent_name)) != CURLE_OK)
    {
        fprintf(log_file,"[!] Failed to setup the torrent to upload\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((code_return_curl = curl_mime_name(part, "torrents")) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set the torrent name\n");
        code_function_return = -1;
        goto Cleanup;
    }

    if((code_return_curl = curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, mime)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set mime\n");
        code_function_return = -1;
        goto Cleanup;
    }
     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto Cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if((code_return_curl = curl_easy_perform(curl_handle)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto Cleanup;
    }
    if(!curl_res.html || strstr(curl_res.html,"Ok.") == 0){
        fprintf(log_file,"[!] Failed to upload torrent\n");
        code_function_return = -1;
        goto Cleanup;
    }else{

    }

      Cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, NULL);
        if(mime)
            curl_mime_free(mime);
        if(curl_res.html)
            free(curl_res.html);
    fflush(log_file);
    return code_function_return;

}
