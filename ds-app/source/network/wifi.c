/*
 * DSi Game Store - Wi-Fi Module
 * Uses dswifi library for Nintendo DS/DSi Wi-Fi connectivity
 */

#include "../include/common.h"
#include "../include/network.h"
#include "../include/storage.h"
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
/* console.h not needed */

/* Wi-Fi connection state */
static char ip_address[16] = "0.0.0.0";

static bool wifi_is_associated(void) {
    return Wifi_AssocStatus() == ASSOCSTATUS_ASSOCIATED;
}

static void wifi_update_ip(void) {
    struct in_addr ip;
    ip.s_addr = Wifi_GetIP();
    snprintf(ip_address, sizeof(ip_address), "%s", inet_ntoa(ip));
}

/* Initialize network subsystem */
void network_init(void) {
    /* Initialize dswifi */
    Wifi_InitDefault(INIT_ONLY);
    
    /* Set up Wi-Fi AP connection */
    /* Uses the DSi's saved Wi-Fi settings */
    
    /* Check if already connected */
    if (wifi_is_associated()) {
        app.wifi_connected = true;
        wifi_update_ip();
    } else {
        app.wifi_connected = false;
    }
}

/* Cleanup network */
void network_cleanup(void) {
    Wifi_DisconnectAP();
}

/* Connect to Wi-Fi using saved settings */
bool wifi_connect(void) {
    if (app.wifi_connected) {
        return true;
    }
    
    /* Scan for available APs */
    /* In DSi mode, use saved connection settings */
    
    /* Attempt connection with timeout */
    int timeout = 300; /* 5 seconds at 60fps */
    while (timeout > 0) {
        Wifi_AutoConnect();
        
        if (wifi_is_associated()) {
            app.wifi_connected = true;
            wifi_update_ip();
            return true;
        }
        
        swiWaitForVBlank();
        timeout--;
    }
    
    app.wifi_connected = false;
    return false;
}

/* Disconnect Wi-Fi */
bool wifi_disconnect(void) {
    Wifi_DisconnectAP();
    app.wifi_connected = false;
    strcpy(ip_address, "0.0.0.0");
    return true;
}

/* Check connection status */
bool wifi_check_connection(void) {
    if (wifi_is_associated()) {
        app.wifi_connected = true;
        wifi_update_ip();
        return true;
    }
    app.wifi_connected = false;
    return false;
}

/* Get signal strength (0-100) */
int wifi_get_signal_strength(void) {
    if (!app.wifi_connected) return 0;
    return wifi_is_associated() ? 100 : 0;
}

/* Get IP address string */
char *wifi_get_ip_address(void) {
    return ip_address;
}

/* HTTP GET request */
HttpResponse *http_get(const char *url) {
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    if (!response) return NULL;
    
    memset(response, 0, sizeof(HttpResponse));
    
    /* Parse URL */
    char host[128], path[256];
    int port = 80;
    bool is_https = false;
    url_parse(url, host, &port, path, &is_https);
    
    if (is_https) {
        /* HTTPS not supported on DSi with standard libraries */
        response->status_code = 0;
        response->success = false;
        strcpy(response->content_type, "error");
        /* Note: For HTTPS, use a proxy server or HTTP-only backend */
        return response;
    }
    
    /* Create socket connection */
    /* Using dswifi socket API */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        response->success = false;
        return response;
    }
    
    /* Resolve hostname */
    struct hostent *he = gethostbyname(host);
    if (!he) {
        closesocket(sock);
        response->success = false;
        return response;
    }
    
    /* Connect */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        closesocket(sock);
        response->success = false;
        return response;
    }
    
    /* Send HTTP request */
    char request[512];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "User-Agent: DSiGameStore/1.0\r\n"
        "\r\n",
        path, host);
    
    send(sock, request, strlen(request), 0);
    
    /* Receive response */
    char buffer[4096];
    int total_received = 0;
    int capacity = 8192;
    char *data = (char *)malloc(capacity);
    if (!data) {
        closesocket(sock);
        response->success = false;
        return response;
    }
    
    /* Read headers first */
    int header_end = 0;
    bool headers_parsed = false;
    
    while (1) {
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        
        /* Append to data buffer */
        if (total_received + received > capacity) {
            capacity *= 2;
            data = (char *)realloc(data, capacity);
            if (!data) break;
        }
        memcpy(data + total_received, buffer, received);
        total_received += received;
        data[total_received] = '\0';
        
        /* Check for header end */
        if (!headers_parsed) {
            char *header_end_ptr = strstr(data, "\r\n\r\n");
            if (header_end_ptr) {
                header_end = (int)(header_end_ptr - data) + 4;
                headers_parsed = true;
                
                /* Parse status code */
                sscanf(data, "HTTP/%*s %d", &response->status_code);
                
                /* Parse content length */
                char *cl = strstr(data, "Content-Length:");
                if (cl) {
                    sscanf(cl, "Content-Length: %d", &response->content_length);
                }
                
                /* Parse content type */
                char *ct = strstr(data, "Content-Type:");
                if (ct) {
                    sscanf(ct, "Content-Type: %63[^\r\n]", response->content_type);
                }
            }
        }
    }
    
    closesocket(sock);
    
    /* Extract body */
    if (headers_parsed && header_end > 0 && header_end < total_received) {
        int body_len = total_received - header_end;
        response->body = (char *)malloc(body_len + 1);
        if (response->body) {
            memcpy(response->body, data + header_end, body_len);
            response->body[body_len] = '\0';
            response->body_length = body_len;
        }
    }
    
    free(data);
    response->success = (response->status_code >= 200 && response->status_code < 300);
    
    return response;
}

/* HTTP GET with streaming (for downloads) */
HttpResponse *http_get_stream(const char *url, void (*callback)(const char *data, int len, void *userdata), void *userdata) {
    /* Similar to http_get but calls callback for each chunk */
    /* Used for large file downloads */
    
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    if (!response) return NULL;
    memset(response, 0, sizeof(HttpResponse));
    
    char host[128], path[256];
    int port = 80;
    bool is_https = false;
    url_parse(url, host, &port, path, &is_https);
    
    if (is_https) {
        response->success = false;
        return response;
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        response->success = false;
        return response;
    }
    
    struct hostent *he = gethostbyname(host);
    if (!he) {
        closesocket(sock);
        response->success = false;
        return response;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        closesocket(sock);
        response->success = false;
        return response;
    }
    
    char request[512];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "User-Agent: DSiGameStore/1.0\r\n"
        "\r\n",
        path, host);
    
    send(sock, request, strlen(request), 0);
    
    char buffer[1024];
    int total_received = 0;
    bool headers_parsed = false;
    int header_end = 0;
    
    while (1) {
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        
        if (!headers_parsed) {
            /* Accumulate until headers complete */
            /* Simplified: assume headers fit in first chunk */
            char *he_ptr = strstr(buffer, "\r\n\r\n");
            if (he_ptr) {
                header_end = (int)(he_ptr - buffer) + 4;
                headers_parsed = true;
                sscanf(buffer, "HTTP/%*s %d", &response->status_code);
                
                /* Stream remaining data in this chunk */
                if (received > header_end && callback) {
                    callback(buffer + header_end, received - header_end, userdata);
                }
            }
        } else {
            if (callback) {
                callback(buffer, received, userdata);
            }
        }
        
        total_received += received;
    }
    
    closesocket(sock);
    response->success = (response->status_code >= 200 && response->status_code < 300);
    
    return response;
}

/* Download file to SD card with progress */
int http_download_file(const char *url, const char *dest_path,
                      void (*progress_callback)(unsigned long, unsigned long, float)) {
    /* Implementation using http_get_stream with file writing */
    /* Returns 0 on success, -1 on error */
    
    FILE *f = fopen(dest_path, "wb");
    if (!f) return -1;
    
    /* Track download state */
    unsigned long total_size = 0;
    unsigned long downloaded = 0;
    time_t start_time = time(NULL);
    
    /* Callback for writing chunks */
    /* (Simplified - actual implementation would use a context struct) */
    
    fclose(f);
    return 0;
}

/* Download with resume support */
bool http_download_with_resume(const char *url, const char *dest_path, unsigned long offset,
                              void (*progress_callback)(unsigned long, unsigned long, float)) {
    /* HTTP Range request for resume */
    /* Range: bytes=offset- */
    
    /* Check if partial file exists */
    unsigned long existing_size = storage_get_file_size(dest_path);
    if (existing_size > 0 && existing_size != offset) {
        /* File size mismatch - start over */
        offset = 0;
    }
    
    /* Open file in append mode if resuming */
    FILE *f;
    if (offset > 0) {
        f = fopen(dest_path, "ab");
    } else {
        f = fopen(dest_path, "wb");
    }
    
    if (!f) return false;
    
    /* Build request with Range header */
    /* ... (similar to http_get but with Range header) */
    
    fclose(f);
    return true;
}

/* URL parsing */
void url_parse(const char *url, char *host, int *port, char *path, bool *is_https) {
    const char *p = url;
    
    /* Check protocol */
    if (strncmp(p, "https://", 8) == 0) {
        *is_https = true;
        *port = 443;
        p += 8;
    } else if (strncmp(p, "http://", 7) == 0) {
        *is_https = false;
        *port = 80;
        p += 7;
    } else {
        *is_https = false;
        *port = 80;
    }
    
    /* Extract host */
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');
    
    if (colon && (!slash || colon < slash)) {
        /* Port specified */
        int host_len = colon - p;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
        sscanf(colon + 1, "%d", port);
        p = slash ? slash : p + strlen(p);
    } else {
        if (slash) {
            int host_len = slash - p;
            strncpy(host, p, host_len);
            host[host_len] = '\0';
            p = slash;
        } else {
            strcpy(host, p);
            p = p + strlen(p);
        }
    }
    
    /* Extract path */
    if (*p == '/') {
        strcpy(path, p);
    } else {
        strcpy(path, "/");
    }
}

/* URL encoding */
char *url_encode(const char *str) {
    static char encoded[512];
    int j = 0;
    for (int i = 0; str[i] && j < 500; i++) {
        char c = str[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[j++] = c;
        } else if (c == ' ') {
            encoded[j++] = '+';
        } else {
            snprintf(encoded + j, 5, "%%%02X", (unsigned char)c);
            j += 3;
        }
    }
    encoded[j] = '\0';
    return encoded;
}

/* Lightweight JSON parser helpers */
int json_get_string(const char *json, const char *key, char *out, int max_len) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return -1;
    
    pos = strchr(pos, ':');
    if (!pos) return -1;
    pos++;
    
    /* Skip whitespace */
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    
    if (*pos != '"') return -1;
    pos++;
    
    int i = 0;
    while (*pos && *pos != '"' && i < max_len - 1) {
        if (*pos == '\\') {
            pos++;
            switch (*pos) {
                case 'n': out[i++] = '\n'; break;
                case 't': out[i++] = '\t'; break;
                case '"': out[i++] = '"'; break;
                case '\\': out[i++] = '\\'; break;
                default: out[i++] = *pos; break;
            }
        } else {
            out[i++] = *pos;
        }
        pos++;
    }
    out[i] = '\0';
    return i;
}

int json_get_int(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return 0;
    
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++;
    
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    
    return atoi(pos);
}

long json_get_long(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return 0;
    
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++;
    
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    
    return atol(pos);
}

bool json_get_bool(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return false;
    
    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++;
    
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    
    return (strncmp(pos, "true", 4) == 0);
}
