#include "api.h"

struct curl_response{
    char *html;
    size_t size;
};

static size_t handleCurlResponse(char *data, size_t size, size_t nmemb, void *clientp){
    size_t real_size = nmemb;
    struct curl_response *curl_res = (struct curl_response *) clientp;

    char* ptr_response = realloc(curl_res,curl_res->size + real_size + 1);
    if(!ptr_response){
        return 0;
    }

    curl_res->html = ptr_response;
    memcpy(curl_res->html[curl_res->size],data,real_size);
    curl_res->size += real_size;
    curl_res->html[real_size] = 0;

    return real_size;
}


int authenticate(CURL *curl_handle,const char* user,const char* password){

    struct curl_response curl_res = {0};
    CURLcode code_return_curl_request;
    char* fmt= "login_username=%s,login_password=%s,login=%s";
    char* postfields;

    if(curl_easy_setopt(curl_handle,CURLOPT_URL,ENDPOINT_LOGIN) != CURLE_OK){
        return -1;
    }
    if(curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION, &handleCurlResponse) != CURLE_OK){
        return -1;
    }

    if(curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void*)&curl_res) != CURLE_OK){
        return -1;
    }
    int nb_required_bytes = snprintf(NULL,0,fmt,user,password,LOGIN_STRING_ENCODED);

    if(nb_required_bytes < 0){
        return -1;
    }
    postfields = (char*)malloc(nb_required_bytes + 1);

    if(!postfields){
        return -1;
    }
    snprintf(postfields,nb_required_bytes+1,fmt,user,password,LOGIN_STRING_ENCODED);


    if(curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDSIZE,postfields) != CURLE_OK){
        return -1;
    }

    code_return_curl_request = curl_easy_perform(curl_handle);

    free(curl_res.html);

    curl_easy_cleanup(curl_handle);

    return 0;

}
