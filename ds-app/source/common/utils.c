/*
 * DSi Game Store - Common Utilities
 */

#include "../include/common.h"
#include <ctype.h>

/* String utilities */
char *str_trim(char *str) {
    if (!str) return NULL;
    
    /* Trim leading */
    char *start = str;
    while (isspace(*start)) start++;
    
    /* Trim trailing */
    char *end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    /* Move trimmed string */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    return str;
}

/* Safe string copy */
void safe_strcpy(char *dest, const char *src, size_t size) {
    if (!dest || !src || size == 0) return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

/* Convert string to lowercase */
void str_to_lower(char *str) {
    if (!str) return;
    for (char *c = str; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') *c += 32;
    }
}

/* Sanitize filename (remove invalid chars) */
void sanitize_filename(char *filename) {
    if (!filename) return;
    for (char *c = filename; *c; c++) {
        if (*c == ' ' || *c == '/' || *c == '\\' || *c == ':' ||
            *c == '*' || *c == '?' || *c == '"' || *c == '<' ||
            *c == '>' || *c == '|') {
            *c = '_';
        }
    }
}

/* Format file size for display */
void format_file_size(unsigned long bytes, char *out, int max_len) {
    if (bytes < 1024) {
        snprintf(out, max_len, "%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(out, max_len, "%.1f KB", bytes / 1024.0f);
    } else {
        snprintf(out, max_len, "%.1f MB", bytes / (1024.0f * 1024.0f));
    }
}

/* Format speed for display */
void format_speed(unsigned long bytes_per_sec, char *out, int max_len) {
    if (bytes_per_sec < 1024) {
        snprintf(out, max_len, "%lu B/s", bytes_per_sec);
    } else if (bytes_per_sec < 1024 * 1024) {
        snprintf(out, max_len, "%.1f KB/s", bytes_per_sec / 1024.0f);
    } else {
        snprintf(out, max_len, "%.1f MB/s", bytes_per_sec / (1024.0f * 1024.0f));
    }
}

/* Simple delay (busy wait) */
void delay_ms(int ms) {
    /* Approximate delay - not precise on DSi */
    for (int i = 0; i < ms * 1000; i++) {
        __asm__ volatile ("nop");
    }
}
