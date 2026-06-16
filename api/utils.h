#ifndef UTILS__H_
#define UTILS__H_
#include "types.h"
#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#define HTML_INDICATOR_DESC "<a data-topic_id="
#define HTML_INDICATOR_SEEDERS "<b class=\"seedmed\">"
#define HTML_INDICATOR_SIZE "class=\"small tr-dl dl-stub\""

#define MAX_INFO_SIZE 2048
#define MAX_ID_SIZE 10
#define MAX_SIZE_SIZE 40
#define MAX_SEEDERS_SIZE 6

#define SSCANF_EXTRACT_DESC "%*[^d]data-topic_id=\"%9[^\"]\"%*[^>]>%2047[^<]"
#define SSCANF_EXTRACT_SEEDERS "%*[^>]>%5[0-9]<"
#define SSCANF_EXTRACT_SIZE "%*[^>]>%31[^&]&nbsp;%7[^& ]"

#define SIZE_ARRAY 32
#define SIZE_UNIT_ARRAY 8

struct torrent* parse(struct curl_response *curl_res);

void torrents_cleanup(struct torrent *torrents_list);


char* remove_non_utf8_char(char* str);
#endif