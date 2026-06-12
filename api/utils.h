#ifndef UTILS__H_
#define UTILS__H_
#include "types.h"
#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#define HTML_INDICATOR "<a data-topic_id="
#define MAX_INFO_SIZE 2048
#define SSCANF_EXTRACT_STRING "%*[^d]data-topic_id=\"%d\"%*[^>]>%2047[^<]"

struct torrent* parse(struct curl_response *curl_res);

void torrents_cleanup(struct torrent *torrents_list);

#endif