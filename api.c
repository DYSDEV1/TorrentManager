#include "api.h"


int authUser(CURL *curl_handle,const char* user,const char* password){

    if(curl_easy_setopt(curl_handle,CURLOPT_URL,ENDPOINT_LOGIN) != CURLE_OK){
        return -1;
    }

    return 0;

}
