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
    bool code_function_return = false;
    curl_easy_getinfo(curl_handle,CURLINFO_COOKIELIST, &cookies);
    struct curl_slist *cookies_ptr = cookies;
    if(!cookies){
        goto cleanup;
    }
    while(cookies != NULL){
        if(ct == COOKIE_RUTRACKER){
            if(strstr(cookies->data,"bb_session") != 0){
                if(sscanf(cookies->data,"%*s %*s %*s %*s %ld",&expiration_date) != 1){
                    goto cleanup;
                }
                if(expiration_date > (long)time(NULL)){
                    code_function_return = true;
                    goto cleanup;
                
                }
            }
        }
        if(ct == COOKIE_QBITTORRENT){
            if(strstr(cookies->data,"SID") != 0)
                code_function_return = true;
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
        if(ct == COOKIE_RUTRACKER && isLogged(curl_handle, ct)){
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
    char* postfields = NULL;
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

    nb_required_bytes = snprintf(NULL,0,POSTFIELDS_SEARCH,encoded_search_string);
    if(nb_required_bytes < 0){
        fprintf(log_file,"[!] Failed to alloc postfield\n");
        code_function_return = -1;
        goto cleanup;
    }
    postfields = malloc((size_t)nb_required_bytes + 1);
    if(!postfields){
        fprintf(log_file,"[!] Failed to alloc postfields buffer\n");
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(postfields,(size_t)(nb_required_bytes+1),POSTFIELDS_SEARCH,encoded_search_string);

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
      if((code_function_return = curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,postfields)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set postfields\n");
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
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(curl_res.html)
            free(curl_res.html);
        if(url_search)
            free(url_search);
        if(encoded_search_string)
            curl_free(encoded_search_string);
        if(postfields)
            free(postfields);
       
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

int uploadToServer(struct torrent *tr,FILE *log_file){
    CURLcode code_return_curl;
    struct curl_response curl_res = {0};
    int code_function_return = 0;
    curl_mime *mime = NULL;
    curl_mimepart *part = NULL;

    if(!isLogged(tr->curl_handle,COOKIE_QBITTORRENT)){
        if(authenticate(tr->curl_handle,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file, COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }  
    }

    if((code_function_return = curl_easy_setopt(tr->curl_handle,CURLOPT_URL,ENDPOINT_UPLOAD_TORRENT)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url.\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((mime = curl_mime_init(tr->curl_handle)) == NULL){
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
    if((code_return_curl = curl_mime_filedata(part,tr->name)) != CURLE_OK)
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
    if((code_return_curl = curl_mime_data(part,tr->name,strlen(tr->name))) != CURLE_OK){
        fprintf(log_file,"[!] Failed to add tag data\n");
        code_function_return = -1;
        goto cleanup;
    }


    if((code_return_curl = curl_easy_setopt(tr->curl_handle, CURLOPT_MIMEPOST, mime)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set mime\n");
        code_function_return = -1;
        goto cleanup;
    }
     if((code_function_return = curl_easy_setopt(tr->curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(tr->curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((code_return_curl = curl_easy_perform(tr->curl_handle)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to execute request\n");
        code_function_return = -1;
        goto cleanup;
    }
    if(!curl_res.html || strstr(curl_res.html,"Ok.") == 0){
        fprintf(log_file,"[!] Failed to upload torrent\n");
        code_function_return = -1;
        goto cleanup;
    }

    if(remove(tr->name) != 0){
        fprintf(log_file,"[!] Failed to delete torrent file\n");
        code_function_return = -1;
        goto cleanup;
    }


    cleanup:
        curl_easy_setopt(tr->curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(tr->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(tr->curl_handle, CURLOPT_MIMEPOST, NULL);
        if(mime)
            curl_mime_free(mime);
        if(curl_res.html)
            free(curl_res.html);
    fflush(log_file);
    return code_function_return;

}


int retrieveTorrentInfo(struct torrent *tr,FILE *log_file){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url_retrieve_hash = NULL;
    char* hash_ptr = NULL;
    char* path_ptr = NULL;
    char* name_ptr = NULL;

    curl_easy_setopt(tr->curl_handle, CURLOPT_MIMEPOST, NULL);
    curl_easy_setopt(tr->curl_handle, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(tr->curl_handle, CURLOPT_POST, 0L);
    curl_easy_setopt(tr->curl_handle, CURLOPT_HTTPGET, 1L);


    if(!isLogged(tr->curl_handle,COOKIE_QBITTORRENT)){
        if(authenticate(tr->curl_handle,FMT_QBITTORRENT,getenv("username_qbittorrent"),getenv("password_qbittorrent"),log_file, COOKIE_FILENAME_QBITTORRENT, ENDPOINT_LOGIN_QBITTORRENT, COOKIE_QBITTORRENT) != 0){
            fprintf(log_file,"[!] Failed to authenticate");
            code_function_return = -1;
            goto cleanup;
        }  
    }

    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_GET_HASH,tr->id);
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
    snprintf(url_retrieve_hash,(size_t)nb_required_bytes+1,ENDPOINT_GET_HASH,tr->id);

    if((code_return_curl = curl_easy_setopt(tr->curl_handle,CURLOPT_URL,url_retrieve_hash)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set url for hash retrieve\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_function_return = curl_easy_setopt(tr->curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set writefunction.\n");
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(tr->curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        fprintf(log_file,"[!] Failed to set result structure\n");
        code_function_return = -1;
        goto cleanup;
    }

    if((code_return_curl = curl_easy_perform(tr->curl_handle)) != CURLE_OK){
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
    if((sscanf(path_ptr,SSCANF_EXTRACT_TORRENT_PATH,tr->full_path) != 1)){
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
    if((sscanf(name_ptr,SSCANF_EXTRACT_TORRENT_NAME,tr->name) != 1)){
        fprintf(log_file,"[!] Failed to retrieve torrent name\n");
        code_function_return = -1;
        goto cleanup;
    }

    if(!curl_res.html || !(hash_ptr = strstr(curl_res.html,HTML_INDICATOR_HASH))){
        fprintf(log_file,"[!] Failed to retrieve hash info\n");
        code_function_return = -1;
        goto cleanup;
    }
    if((sscanf(hash_ptr,SSCANF_EXTRACT_HASH,tr->hash) != 1)){
        fprintf(log_file,"[!] Failed to retrieve hash\n");
        code_function_return = -1;
        goto cleanup;
    }

    cleanup:
        curl_easy_setopt(tr->curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(tr->curl_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(tr->curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(tr->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(url_retrieve_hash)
            free(url_retrieve_hash);
        if(curl_res.html)
            free(curl_res.html);
       
    return code_function_return;
}

int retrieveUploadProgression(struct thread_torrent *tt){
    struct curl_response curl_res = {0};
    CURLcode code_return_curl;
    int code_function_return = 0;
    char* url = NULL;
    char* progress_ptr = NULL;
    float progress = 0;
    curl_easy_setopt(tt->curl_handle, CURLOPT_MIMEPOST, NULL);
    curl_easy_setopt(tt->curl_handle, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(tt->curl_handle, CURLOPT_POST, 0L);
    curl_easy_setopt(tt->curl_handle, CURLOPT_HTTPGET, 1L);

    int nb_required_bytes = snprintf(NULL,0,ENDPOINT_GET_TORRENT_PROGRESSION,tt->hash);

    if(nb_required_bytes < 0){
        code_function_return = -1;
        goto cleanup;
    }
    url = (char*)malloc((size_t)(nb_required_bytes + 1));

    if(!url){
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(url,(size_t)(nb_required_bytes+1),ENDPOINT_GET_TORRENT_PROGRESSION,tt->hash);

    if((code_function_return = curl_easy_setopt(tt->curl_handle,CURLOPT_URL,url)) != CURLE_OK){
        code_function_return = -1;
        goto cleanup;
    }
    if((code_function_return = curl_easy_setopt(tt->curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse)) != CURLE_OK){
        code_function_return = -1;
        goto cleanup;
    }

     if((code_function_return = curl_easy_setopt(tt->curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res)) != CURLE_OK){
        code_function_return = -1;
        goto cleanup;
    }
    while(progress != 1){
        code_return_curl = curl_easy_perform(tt->curl_handle);
        if(code_return_curl != CURLE_OK){
            code_function_return = -1;
            goto cleanup;
        }
        
    
        if((progress_ptr = strstr(curl_res.html,HTML_INDICATOR_PROGRESS)) == NULL){
            code_function_return = -1;
            goto cleanup;

        }
        if((sscanf(progress_ptr,SSCANF_EXTRACT_PROGRESS,&progress)) != 1){
            code_function_return = -1;
            goto cleanup;
        }
        pthread_mutex_lock(&tt->mutex);
        memset(tt->upload_progress,0,UPLOAD_PROGRESS_BUFFER_SIZE);
        snprintf(tt->upload_progress,UPLOAD_PROGRESS_BUFFER_SIZE,"Uploading : %d%%",(int)(progress*100));
        pthread_mutex_unlock(&tt->mutex);
        if(curl_res.html){
            free(curl_res.html);
            curl_res.html = NULL;
            curl_res.size = 0;
        }
        sleep(2);

    }
    cleanup:
        curl_easy_setopt(tt->curl_handle, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(tt->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        if(url)
            curl_free(url);
        if(curl_res.html)
            free(curl_res.html);
    
    return code_function_return;
}


int downloadFromServer(struct thread_torrent *tt){
    char* full_rsync_path = NULL;
    int code_function_return = 0;
    int pipefd[2];
    pid_t pid_rsync;
    char read_buffer;
    char tmp_buffer[128];
    char* ptr_rsync_buffer = tmp_buffer;
    const char* download_path = getenv("download_path");
    int count = 0;

    if(!download_path){
        code_function_return = -1;
        goto cleanup;
    }
    int nb_required_bytes = snprintf(NULL,0,"%s:%s",ENDPOINT_RSYNC,tt->full_path);


    if(nb_required_bytes < 0){
        code_function_return = -1;
        goto cleanup;
    }
    full_rsync_path = (char*)malloc((size_t)nb_required_bytes + 1);

    if(!full_rsync_path){
        code_function_return = -1;
        goto cleanup;
    }
    snprintf(full_rsync_path,((size_t)nb_required_bytes+1),"%s:%s",ENDPOINT_RSYNC,tt->full_path);
    
    if(pipe(pipefd) == -1){
        code_function_return = -1;
        goto cleanup;
    }

    pid_rsync = fork();

    if(pid_rsync < 0){
        code_function_return = -1;
        goto cleanup;
    }
    //child
    if(pid_rsync == 0){
        if(close(pipefd[0]) == -1){
            _exit(EXIT_FAILURE);
        }
        if(dup2(pipefd[1],1) == -1){
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        if(execlp("rsync","rsync","-a","--info=progress2","--info=name0","--",full_rsync_path,download_path, NULL) == -1){
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }
    //parent
    if(close(pipefd[1]) == -1){
        code_function_return = -1;
        goto cleanup;
    }
    while(read(pipefd[0],&read_buffer,1) > 0){
        if((read_buffer == '\r') || (read_buffer == '\n') || (count >= (DOWNLOAD_PROGRESS_BUFFER_SIZE-1))){
            *ptr_rsync_buffer = '\0';
            pthread_mutex_lock(&tt->mutex);
            memset(tt->download_progress,0,UPLOAD_PROGRESS_BUFFER_SIZE);
            memcpy(tt->download_progress,ptr_rsync_buffer,UPLOAD_PROGRESS_BUFFER_SIZE);
            pthread_mutex_unlock(&tt->mutex);
            memset(tt->download_progress,0,DOWNLOAD_PROGRESS_BUFFER_SIZE);
            count = 0;
        }else{
            count++;
            *ptr_rsync_buffer = read_buffer;
            ptr_rsync_buffer++;
        }
    }
    close(pipefd[0]);
    if(count > 0){
        *ptr_rsync_buffer = '\0';
        pthread_mutex_lock(&tt->mutex);
        memset(tt->download_progress,0,UPLOAD_PROGRESS_BUFFER_SIZE);
        memcpy(tt->download_progress,ptr_rsync_buffer,UPLOAD_PROGRESS_BUFFER_SIZE);
        pthread_mutex_unlock(&tt->mutex);
    }
    waitpid(pid_rsync,&code_function_return,0);
    
    cleanup:
        if(full_rsync_path)
            free(full_rsync_path);

    return code_function_return;
}


void* handle_threaded_torrent_download(void* arg){

    struct thread_torrent *tt = (struct thread_torrent*) arg;
    pthread_mutex_lock(&tt->mutex);
        tt->status = UPLOADING;
    pthread_mutex_unlock(&tt->mutex);
    if(retrieveUploadProgression(tt) != 0){
        pthread_mutex_lock(&tt->mutex);
        tt->status = FAILED;
        pthread_mutex_unlock(&tt->mutex);
        return NULL;
    }else{
        pthread_mutex_lock(&tt->mutex);
        tt->status = DOWNLOADING;
        pthread_mutex_unlock(&tt->mutex);
    }    
    if(downloadFromServer(tt) != 0){
        pthread_mutex_lock(&tt->mutex);
        tt->status = FAILED;
        pthread_mutex_unlock(&tt->mutex);
        return NULL;
    }
    else{
        pthread_mutex_lock(&tt->mutex);
        tt->status = FINISHED;
        pthread_mutex_unlock(&tt->mutex);
    }
    return NULL;
}