#include "api.h"



int main(){

    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl_handle = curl_easy_init();

    return 0; 
}