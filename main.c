/*
(tcc)gcc -Wall main.c yyjson/yyjson.c -lcurl -o kem_dl
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

/* #define SOCKS */
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT
#define YYJSON_DISABLE_WRITER 1
#define YYJSON_DISABLE_INCR_READER 1

#include <stdio.h>
#include <string.h> /* for memcpy */
#include <sys/types.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include "stb/stb_sprintf.h"
#include "yyjson/yyjson.h"
#include "black_lace/black_lace.h"
#include "funcs.h"


int main(int argc, char **argv)
{
    if (argc < 4) {
        printf("At least 3 args are needed. "
               "%s 'service' 'userid' postid'\n", argv[0]);
        return 1;
    }
    CURL *curl;
    CURLcode res;
    struct memory chunk = { 0 };
    struct curl_slist *list = NULL;
    struct stat st;
    int foldercheck;
    enum { CHUNK_BUFFER = 65536, MAX_LINK_SIZE = 256 };
    char get_link[MAX_LINK_SIZE];

    stbsp_sprintf(get_link,
                  "https://kemono.cr/api/v1/%.7s/"
                  "user/%.9s/post/%.9s",
                  argv[1], argv[2], argv[3]);
    foldercheck = stat(argv[3], &st);
    if (foldercheck == -1)
        mkdir(argv[3], 0777);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        printf("curl_easy_init() returned NULL.\n");
        return 2;
    }

    chunk.response = malloc(sizeof(char) * CHUNK_BUFFER);
    if (chunk.response == NULL) {
        printf("Memory allocation failure for chunk.response.\n");
        return 3;
    }

    list = curl_slist_append(list, "Accept: text/css");
    res = curl_ez_setup(curl, list, get_link, &chunk, NULL);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return 4;
    }

    /*
    TODO: Download the very first file i.e. what "file" value have. Not
    "attachments".
    */
    long response_code;
    double total;
    int ret_code;
    unsigned int in_fact, unp_lenght;
    yyjson_doc *doc;
    enum { GZIP_ARCHIVE_SIZE = 6144 };

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);

    if (chunk.size < GZIP_ARCHIVE_SIZE) {
        printf("Response code: %ld  Time: %.1f seconds\n",
               response_code, total);
        printf("%d bytes retrieved. Got .gzip archive.\n", chunk.size);
        unp_lenght = read_two_bytes(&chunk.response[chunk.size - 4]);
        in_fact = unp_lenght;
        ret_code = unpack(&chunk.response[chunk.size + 1], &in_fact,
                          chunk.response + 10, &chunk.size);
        printf("[black_lace]: unp_length: %u bytes, in_fact: %u bytes\n",
               unp_lenght, in_fact);
        doc = yyjson_read(&chunk.response[chunk.size + 1], in_fact, 0);
    } else {
        printf("Response code: %ld  Time: %.1f seconds\n",
               response_code, total);
        printf("%d bytes retrieved.\n", chunk.size);
        doc = yyjson_read(chunk.response, chunk.size, 0);
    }

    free(chunk.response);
    download_the_attachments(curl, doc, &response_code, argv[3]);
    yyjson_doc_free(doc);
    curl_cleaning_up(curl, list);
    return 0;
}
