#ifndef FUNCS_H_SENTRY
#define FUNCS_H_SENTRY

static struct memory {
    char *response;
    int size;
};

static struct progress {
    char *private;
    int size;
};

static int progress_callback(void *clientp,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t ultotal,
                             curl_off_t ulnow)
{
    fflush(stdout);
    printf("Received [%lld] out of [%lld] bytes.\r", dlnow, dltotal);
    return 0; /* all is good */
}

static int write_data(void *ptr, int size, int nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static int cb(char *data, int size, int nmemb, void *clientp)
{
    int realsize;
    struct memory *mem;
    realsize = size * nmemb;
    mem = (struct memory *)clientp;

    memcpy(&mem->response[mem->size], data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    /*
    printf("mem->size is %d\n", mem->size);
    printf("&(mem->response[mem->size]) pointer located at %p addr.\n", &(mem->response[mem->size]));
    */
    return realsize;
}

static CURLcode curl_ez_setup(CURL *h, struct curl_slist *l,
                              char *get_link, struct memory *mem,
                              struct progress *bar)
{
    CURLcode response;

    if (bar) {
        curl_easy_setopt(h, CURLOPT_XFERINFODATA, bar);
        curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(h, CURLOPT_URL, get_link);
        /* curl_easy_setopt(h, CURLOPT_PROXY, "socks5h://localhost:9050"); */
        curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_data);
        return 1;
    }

    curl_easy_setopt(h, CURLOPT_URL, get_link);
    /* curl_easy_setopt(h, CURLOPT_PROXY, "socks5h://localhost:9050"); */
    curl_easy_setopt(h, CURLOPT_HTTPHEADER, l);
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    /* curl_easy_setopt(h, CURLOPT_VERBOSE, 1L); */
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, (void *)mem);

    response = curl_easy_perform(h);
    return response;
}

static int download_the_attachments(CURL *hndl,
                                    yyjson_doc *att,
                                    long *res_code,
                                    char *post_id)
{
    yyjson_val *root, *title, *attachments, *row, *k, *v;
    unsigned int idx, max, i, m;

    root = yyjson_doc_get_root(att);
    title = yyjson_ptr_get(root, "/post/title");
    attachments = yyjson_ptr_get(root, "/post/attachments");
    printf("\nTitle: [%s]\n", yyjson_get_str(title));
    printf("%zd files has been placed to the queue.\n\n", 
           yyjson_arr_size(attachments));
    yyjson_arr_foreach(attachments, idx, max, row)
    {
        yyjson_obj_foreach(row, i, m, k, v)
        {
            yyjson_val *name, *path;
            enum { JUNK_FILE = 170, /* Bytes */
                   MAX_LINK_SIZE = 256,
                   MAX_FILE_NAME = 512
            };
            char link[MAX_LINK_SIZE];
            char filename[MAX_FILE_NAME];
            struct progress data;
            int file_len;
            FILE *file_lookup, *file;

            name = yyjson_ptr_get(row, "/name");
            path = yyjson_ptr_get(row, "/path");
            stbsp_sprintf(link, "https://kemono.cr%.110s",
                          yyjson_get_str(path));
            printf("Now downloading: %s\n", yyjson_get_str(name));
            curl_ez_setup(hndl, NULL, link, NULL, &data);
            stbsp_sprintf(filename, "./%.9s/%.502s",
                          post_id, yyjson_get_str(name));

            /* Just in case if u want to redownload files again */
            /* So u can skip those who have > 1KB size */
            file_lookup = fopen(filename, "a+b");
            if (!file_lookup) {
                printf("Can't write data to %s", filename);
            }
            fseek(file_lookup, 0, SEEK_END);
            file_len = ftell(file_lookup);
            fclose(file_lookup);
            /* printf("[DEBUG]file_len: %d\n", file_len); */
            /* C Traps and Pitfalls; 2.6. The Dangling else Problem */
            if (file_len <= JUNK_FILE) {
                do {
                    file = fopen(filename, "wb");
                    if (!file)
                        printf("Can't write data to %s", filename);
                    else {
                        curl_easy_setopt(hndl, CURLOPT_WRITEDATA, file);
                        curl_easy_perform(hndl);
                        fclose(file);
                    }
                    curl_easy_getinfo(hndl, CURLINFO_RESPONSE_CODE, res_code);
                    /* Either 500 or 502 or (TODO?) after n- attempts */
                } while (res_code <= 503);
                printf("%s has been saved.\n\n", yyjson_get_str(name));
            } else
                printf("Already downloaded. Skipping...\n");
        i++;
        }
    }
    printf("----- Done. -----\n");
    return 1;
}

static void curl_cleaning_up(CURL *curl, struct curl_slist *list)
{
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}
#endif