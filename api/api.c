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
    bool code_function_return = true;
    curl_easy_getinfo(curl_handle,CURLINFO_COOKIELIST, &cookies);
    struct curl_slist *cookies_ptr = cookies;
    while(cookies != NULL){
        if(ct == COOKIE_RUTRACKER){
            if(strstr(cookies->data,"bb_session") != 0){
                if(sscanf(cookies->data,"%*s %*s %*s %*s %ld",&expiration_date) != 1){
                    code_function_return = false;
                    goto cleanup;
                }
                if(expiration_date > (long)time(NULL)){
                    code_function_return = true;
                    goto cleanup;
                
                }
            }
        }
        if(ct == COOKIE_QBITTORRENT){
            if(strstr(cookies->data,"SID") != 0){
                code_function_return = true;
                goto cleanup;
            }
                
        }

        cookies = cookies->next;
    }
    cleanup: 
        curl_slist_free_all(cookies_ptr);
    return code_function_return;
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
    CURLcode code_return_curl;
    int code_function_return = 0;
    char *postfields = NULL;
    char *encoded_user = NULL;
    char *encoded_password = NULL;
    if(!user || !password){
        fprintf(log_file,"[!] User or pasword NULL\n");
        code_function_return = -1;
        goto cleanup;

    }
    if((encoded_user = curl_easy_escape(curl_handle,user,(int)strlen(user))) == NULL){
        fprintf(log_file,"[!] Failed to encode user\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((encoded_password = curl_easy_escape(curl_handle,password,(int)strlen(password))) == NULL){
        fprintf(log_file,"[!] Failed to encode password\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEFILE,cookie_file_name)) == CURLE_OK){
        if(curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "RELOAD") != CURLE_OK){
            fprintf(log_file, "[!] Failed to reload cookies\n");
            code_function_return = -1;
            goto cleanup;
        }
        if(isLogged(curl_handle, ct)){
            fprintf(log_file,"[+] Already logged, cookies still actives\n");
            goto cleanup;
        }
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,endpoint_url)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }
    int nb_required_bytes = snprintf(NULL,0,fmt,encoded_user,encoded_password,LOGIN_STRING_ENCODED);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc postfield buffer.\n");
        code_function_return = -1;
        goto cleanup;
    }
    postfields = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!postfields){
        fprintf(log_file, "[!] Failed to alloc postfield buffer.\n");
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(postfields,(size_t)(nb_required_bytes+1),fmt,encoded_user,encoded_password,LOGIN_STRING_ENCODED);


    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,postfields)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set postfields\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_COOKIEJAR,cookie_file_name)) != CURLE_OK){
        fprintf(log_file, "[!] Failed to set cookies\n");
        code_function_return = -1;
        goto cleanup;
    }

    code_return_curl = curl_easy_perform(curl_handle);

    if(code_return_curl != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute authenticate request.\n");
        code_function_return = -1;
        goto cleanup;
    }


    if(isLogged(curl_handle,ct)){
        fprintf(log_file,"[+] Login successful\n");
    }
    else{
        fprintf(log_file,"[-] Failed to login\n");
        code_function_return = -1;
    }

    cleanup:
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
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url_search = NULL;
    char* encoded_search_string = NULL;

    if(!isLogged(curl_handle,COOKIE_RUTRACKER)){
        if(authenticate(curl_handle,FMT_RUTRACKER,getenv("username_rutracker"),getenv("password_rutracker"),log_file,COOKIE_FILENAME_RUTRACKER, ENDPOINT_LOGIN_RUTRACKER, COOKIE_RUTRACKER) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }   
    }

    if((encoded_search_string = curl_easy_escape(curl_handle,search_string,(int)strlen(search_string))) == NULL){
        fprintf(log_file,"[!] Failed to encode url.\n");
        code_function_return = -1;
        goto cleanup;
    }


    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_SEARCH,encoded_search_string);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    url_search = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!url_search){
        fprintf(log_file, "[!] Failed to alloc url buffer.\n");
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(url_search,(size_t)(nb_required_bytes+1),ENDPOINT_SEARCH,encoded_search_string);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url_search)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }

    code_return_curl = curl_easy_perform(curl_handle);

    if(code_return_curl != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }
    struct torrent *torrents_list = parse(&curl_res);

    cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
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
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url_download = NULL;
    FILE *torrent_file = NULL;

    if(!isLogged(curl_handle,COOKIE_RUTRACKER)){
        if(authenticate(curl_handle,FMT_RUTRACKER,getenv("username_rutracker"),getenv("password_rutracker"),log_file, COOKIE_FILENAME_RUTRACKER, ENDPOINT_LOGIN_RUTRACKER, COOKIE_RUTRACKER) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }  
    }

    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_DOWNLOAD,torrent_name);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    url_download = malloc((size_t)(nb_required_bytes + 1));

    if(!url_download){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto cleanup;
    }

    snprintf(url_download,(size_t)(nb_required_bytes+1),ENDPOINT_DOWNLOAD,torrent_name);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url_download)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleDownloadTorrent)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

    torrent_file = fopen(torrent_name,"wb");
    if(!torrent_file){
        fprintf(log_file, "[!] Failed to create torrent file\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)torrent_file)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }

    code_return_curl = curl_easy_perform(curl_handle);

    if(code_return_curl != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }

    cleanup:
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
            goto cleanup;
        }  
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,ENDPOINT_UPLOAD_TORRENT)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((mime = curl_mime_init(curl_handle)) == NULL){
        fprintf(log_file,"[!] Failed to setup mime\n");
        code_function_return = -1;
        goto cleanup;
    }
    /*--- ADD TORRENT ---*/
    if((part = curl_mime_addpart(mime)) == NULL){
        fprintf(log_file,"[!] Failed to add part for torrent\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_mime_filedata(part,torrent_name)) != CURLE_OK)
    {
        fprintf(log_file,"[!] Failed to setup the torrent to upload\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_mime_name(part, "torrents")) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set the torrent name\n");
        code_function_return = -1;
        goto cleanup;
    }
    /*--- ADD TAG ---*/
    if((part = curl_mime_addpart(mime)) == NULL){
        fprintf(log_file,"[!] Failed to add part for tag\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_mime_name(part,"tags")) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set tag name\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_mime_data(part,torrent_name,strlen(torrent_name))) != CURLE_OK){
        fprintf(log_file,"[!] Failed to add tag data\n");
        code_function_return = -1;
        goto cleanup;
    }


    if((code_return_curl = curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, mime)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set mime\n");
        code_function_return = -1;
        goto cleanup;
    }
     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_easy_perform(curl_handle)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }
    if(!curl_res.html || strstr(curl_res.html,"Ok.") == 0){
        fprintf(log_file,"[!] Failed to upload torrent\n");
        code_function_return = -1;
        goto cleanup;
    }else{

    }

      cleanup:
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


int retrieveTorrentInfo(CURL *curl_handle,char* torrent_id,char* torrent_name,char* hash,char* torrent_path,FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url_retrieve_hash = NULL;
    char* hash_ptr = NULL;
    char* path_ptr = NULL;
    char* name_ptr = NULL;

    curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);


    if(!isLogged(curl_handle,COOKIE_QBITTORRENT)){
        if(authenticate(curl_handle,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file, COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }  
    }

    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_GET_HASH,torrent_id);
    if(nb_required_bytes < 0){
        fprintf(log_file, "[!] Failed to allocate bytes for url\n");
        code_function_return = -1;
        goto cleanup;
    }
    if(!(url_retrieve_hash = malloc((size_t)nb_required_bytes+1))){
        fprintf(log_file, "[!] Failed to allocated bytes for url\n");
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(url_retrieve_hash,(size_t)nb_required_bytes+1,ENDPOINT_GET_HASH,torrent_id);

    if((code_return_curl = curl_easy_setopt(curl_handle,CURLOPT_URL,url_retrieve_hash)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url for hash retrieve\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_return_curl = curl_easy_perform(curl_handle)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }
    /* PATH */
    if(!curl_res.html || !(path_ptr = strstr(curl_res.html,HTML_INDICATOR_TORRENT_PATH))){
        fprintf(log_file,"[!] Failed to retrieve path torrent\n");
        code_function_return = -1;
        goto cleanup;
    }
    path_ptr += strlen(HTML_INDICATOR_TORRENT_PATH);
    if((sscanf(path_ptr,SSCANF_EXTRACT_TORRENT_PATH,torrent_path) != 1) || !torrent_path){
        fprintf(log_file,"[!] Failed to retrieve hash\n");
        code_function_return = -1;
        goto cleanup;
    }
    /* NAME */
    if(!curl_res.html || !(name_ptr = strstr(curl_res.html,HTML_INDICATOR_TORRENT_NAME))){
        fprintf(log_file,"[!] Failed to retrieve torrent name\n");
        code_function_return = -1;
        goto cleanup;
    }
    name_ptr += strlen(HTML_INDICATOR_TORRENT_NAME);
    if((sscanf(name_ptr,SSCANF_EXTRACT_TORRENT_NAME,torrent_name) != 1) || !torrent_name){
        fprintf(log_file,"[!] Failed to retrieve hash\n");
        code_function_return = -1;
        goto cleanup;
    }

    if(!curl_res.html || !(hash_ptr = strstr(curl_res.html,HTML_INDICATOR_HASH))){
        fprintf(log_file,"[!] Failed to retrieve hash info\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((sscanf(hash_ptr,SSCANF_EXTRACT_HASH,hash) != 1) || !hash){
        fprintf(log_file,"[!] Failed to retrieve hash\n");
        code_function_return = -1;
        goto cleanup;
    }

    cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(url_retrieve_hash)
            free(url_retrieve_hash);
        if(curl_res.html)
            free(curl_res.html);
       
    return code_function_return;
}

int retrieveUploadProgression(CURL *curl_handle,char* hash, FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url = NULL;
    char* progress_ptr = NULL;
    int progress = 0;
    curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);

    if(!isLogged(curl_handle,COOKIE_QBITTORRENT)){
        if(authenticate(curl_handle,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file, COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }  
    }
      
    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_GET_TORRENT_PROGRESSION,hash);

    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    url = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!url){
        fprintf(log_file, "[!] Failed to alloc url buffer.\n");
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(url,(size_t)(nb_required_bytes+1),ENDPOINT_GET_TORRENT_PROGRESSION,hash);

    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_URL,url)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }
    while(progress != 1){
        code_return_curl = curl_easy_perform(curl_handle);
        if(code_return_curl != CURLE_OK){
            fprintf(log_file,"[!] Failed to execute request\n");
            code_function_return = -1;
            goto cleanup;
        }
        if((progress_ptr = strstr(curl_res.html,HTML_INDICATOR_PROGRESS)) == NULL){
            fprintf(log_file,"[!] Failed to retrieve progress information\n");
            code_function_return = -1;
            goto cleanup;

        }
        if((sscanf(progress_ptr,SSCANF_EXTRACT_PROGRESS,&progress)) != 1){
            fprintf(log_file,"[!] Failed to retrieve progress value\n");
            code_function_return = -1;
            goto cleanup;
        }
        if(curl_res.html){
            free(curl_res.html);
            curl_res.html = NULL;
            curl_res.size = 0;
        }
        sleep(2);

    }
    cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(url)
            curl_free(url);
        if(curl_res.html)
            free(curl_res.html);
    

    return code_function_return;
}


int downloadFromServer(CURL *curl_handle,char* torrent_name,char* torrent_path, FILE *log_file){
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* full_path_torrent = NULL;
    CURLU *url = NULL;
    CURLUcode url_code;
    FILE *torrent = NULL;
    
    url = curl_url();
    if((url_code = curl_url_set(url,CURLUPART_SCHEME,"sftp",0)) != CURLUE_OK){
        fprintf(log_file,"[!] Failed to setup scheme in url\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((url_code = curl_url_set(url,CURLUPART_HOST,ENDPOINT_SFTP,0)) != CURLUE_OK){
        fprintf(log_file,"[!] Failed to setup host in url\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((url_code = curl_url_set(url,CURLUPART_PATH,torrent_path,CURLU_URLENCODE)) != CURLUE_OK){
        fprintf(log_file,"[!] Failed to setup path in url\n");
        code_function_return = -1;  
        goto cleanup;
    }
    if((url_code = curl_url_get(url,CURLUPART_URL,&full_path_torrent,0)) != CURLUE_OK){
        fprintf(log_file,"[!] Failed to retrieve complete url\n");
        code_function_return = -1;
        goto cleanup;
    }


    if((code_return_curl = curl_easy_setopt(curl_handle,CURLOPT_URL,full_path_torrent)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to setup url\n");
        code_function_return = -1;
        goto cleanup;
    }
    const char *user = getenv("username_sftp");
    const char *pass = getenv("password_sftp");

    if (!user || !pass) {
        fprintf(log_file, "[!] Missing SFTP credentials\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_USERNAME,user)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to setup user sftp\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_PASSWORD,pass)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to setup password sftp\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleDownloadTorrent)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    torrent = fopen(torrent_name,"wb");
    if(!torrent){
        fprintf(log_file, "[!] Failed to create torrent file\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)torrent)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }

    code_return_curl = curl_easy_perform(curl_handle);

    if(code_return_curl != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }



    cleanup:
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(full_path_torrent)
            curl_free(full_path_torrent);
        if(torrent)
            fclose(torrent);
        if(url)
            curl_url_cleanup(url);

    return code_function_return;
}