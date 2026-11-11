/*
 * DSi Game Store - Network Module
 * Wi-Fi connection and HTTP communication
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

/* HTTP response */
typedef struct {
    int status_code;
    int content_length;
    char content_type[64];
    char *body;
    int body_length;
    bool success;
} HttpResponse;

/* Network initialization */
void network_init(void);
void network_cleanup(void);

/* Wi-Fi */
bool wifi_connect(void);
bool wifi_disconnect(void);
bool wifi_check_connection(void);
int wifi_get_signal_strength(void);
char *wifi_get_ip_address(void);

/* HTTP client */
HttpResponse *http_get(const char *url);
HttpResponse *http_get_stream(const char *url, void (*callback)(const char *data, int len, void *userdata), void *userdata);
int http_download_file(const char *url, const char *dest_path, 
                      void (*progress_callback)(unsigned long downloaded, unsigned long total, float speed));
bool http_download_with_resume(const char *url, const char *dest_path, unsigned long offset,
                              void (*progress_callback)(unsigned long downloaded, unsigned long total, float speed));

/* URL parsing */
void url_parse(const char *url, char *host, int *port, char *path, bool *is_https);
char *url_encode(const char *str);

/* JSON helpers for DSi (lightweight) */
int json_get_string(const char *json, const char *key, char *out, int max_len);
int json_get_int(const char *json, const char *key);
long json_get_long(const char *json, const char *key);
bool json_get_bool(const char *json, const char *key);

#endif /* NETWORK_H */
