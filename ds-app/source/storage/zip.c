/*
 * DSi Game Store - ZIP Extraction
 * Uses zlib for DEFLATE decompression
 */

#include "../include/common.h"
#include "../include/zip.h"
#include <zlib.h>

/* ZIP constants */
#define ZIP_LOCAL_HEADER_SIG    0x04034b50
#define ZIP_CENTRAL_DIR_SIG     0x02014b50
#define ZIP_END_CENTRAL_DIR_SIG 0x06054b50

#define ZIP_METHOD_STORED   0
#define ZIP_METHOD_DEFLATE  8

/* Read a 32-bit little-endian value */
static unsigned int read_u32(const unsigned char *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Read a 16-bit little-endian value */
static unsigned int read_u16(const unsigned char *p) {
    return p[0] | (p[1] << 8);
}

/* Check if file is a ZIP archive */
bool zip_is_zip(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    
    unsigned char sig[4];
    if (fread(sig, 1, 4, f) != 4) {
        fclose(f);
        return false;
    }
    fclose(f);
    
    unsigned int s = read_u32(sig);
    return (s == ZIP_LOCAL_HEADER_SIG);
}

/* List files in ZIP */
int zip_list_files(const char *zip_path, char files[][MAX_PATH_LEN], int max_files) {
    /* Simplified: scan for local file headers */
    FILE *f = fopen(zip_path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size < 30) {
        fclose(f);
        return -1;
    }
    
    unsigned char *data = (unsigned char *)malloc(file_size);
    if (!data) {
        fclose(f);
        return -1;
    }
    
    fread(data, 1, file_size, f);
    fclose(f);
    
    int count = 0;
    long pos = 0;
    
    while (pos < file_size - 30 && count < max_files) {
        if (read_u32(data + pos) != ZIP_LOCAL_HEADER_SIG) {
            pos++;
            continue;
        }
        
        unsigned int filename_len = read_u16(data + pos + 26);
        unsigned int extra_len = read_u16(data + pos + 28);
        
        if (filename_len > 0 && filename_len < MAX_PATH_LEN && pos + 30 + filename_len <= file_size) {
            memcpy(files[count], data + pos + 30, filename_len);
            files[count][filename_len] = '\0';
            count++;
        }
        
        /* Calculate next header position */
        unsigned int comp_size = read_u32(data + pos + 18);
        pos += 30 + filename_len + extra_len + comp_size;
    }
    
    free(data);
    return count;
}

/* Extract .nds file from ZIP */
int zip_extract_nds(const char *zip_path, const char *extract_dir, char *extracted_path, int max_path_len) {
    if (!zip_is_zip(zip_path)) return -1;
    
    FILE *f = fopen(zip_path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *data = (unsigned char *)malloc(file_size);
    if (!data) {
        fclose(f);
        return -1;
    }
    
    fread(data, 1, file_size, f);
    fclose(f);
    
    /* Find .nds file in archive */
    long pos = 0;
    char nds_filename[MAX_PATH_LEN] = "";
    long nds_data_offset = -1;
    unsigned int nds_comp_size = 0;
    unsigned int nds_uncomp_size = 0;
    unsigned int nds_method = 0;
    
    while (pos < file_size - 30) {
        if (read_u32(data + pos) != ZIP_LOCAL_HEADER_SIG) {
            pos++;
            continue;
        }
        
        unsigned int method = read_u16(data + pos + 8);
        unsigned int comp_size = read_u32(data + pos + 18);
        unsigned int uncomp_size = read_u32(data + pos + 22);
        unsigned int filename_len = read_u16(data + pos + 26);
        unsigned int extra_len = read_u16(data + pos + 28);
        
        if (filename_len > 0 && filename_len < MAX_PATH_LEN && pos + 30 + filename_len <= (unsigned long)file_size) {
            char filename[MAX_PATH_LEN];
            memcpy(filename, data + pos + 30, filename_len);
            filename[filename_len] = '\0';
            
            /* Check if this is a .nds file */
            char *ext = strrchr(filename, '.');
            if (ext && strcasecmp(ext, ".nds") == 0) {
                strncpy(nds_filename, filename, MAX_PATH_LEN - 1);
                nds_data_offset = pos + 30 + filename_len + extra_len;
                nds_comp_size = comp_size;
                nds_uncomp_size = uncomp_size;
                nds_method = method;
                break;
            }
        }
        
        pos += 30 + filename_len + extra_len + comp_size;
    }
    
    if (nds_data_offset < 0 || nds_data_offset + nds_comp_size > (unsigned long)file_size) {
        free(data);
        return -1; /* No .nds file found */
    }
    
    /* Create extract directory if needed */
    /* (Assume it exists) */
    
    /* Build output path */
    /* Sanitize filename */
    char safe_name[MAX_PATH_LEN];
    strcpy(safe_name, nds_filename);
    for (char *c = safe_name; *c; c++) {
        if (*c == '/' || *c == '\\') *c = '_';
    }
    
    snprintf(extracted_path, max_path_len, "%s/%s", extract_dir, safe_name);
    
    /* Extract the file */
    FILE *out = fopen(extracted_path, "wb");
    if (!out) {
        free(data);
        return -1;
    }
    
    if (nds_method == ZIP_METHOD_STORED) {
        /* Just copy */
        fwrite(data + nds_data_offset, 1, nds_comp_size, out);
    } else if (nds_method == ZIP_METHOD_DEFLATE) {
        /* Inflate using zlib */
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        
        strm.next_in = data + nds_data_offset;
        strm.avail_in = nds_comp_size;
        
        /* Allocate output buffer */
        unsigned char *out_buf = (unsigned char *)malloc(nds_uncomp_size);
        if (!out_buf) {
            fclose(out);
            free(data);
            return -1;
        }
        
        strm.next_out = out_buf;
        strm.avail_out = nds_uncomp_size;
        
        /* Initialize inflate with -15 (raw deflate, no zlib header) */
        if (inflateInit2(&strm, -15) != Z_OK) {
            free(out_buf);
            fclose(out);
            free(data);
            return -1;
        }
        
        int ret = inflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK) {
            inflateEnd(&strm);
            free(out_buf);
            fclose(out);
            free(data);
            return -1;
        }
        
        fwrite(out_buf, 1, strm.total_out, out);
        free(out_buf);
        inflateEnd(&strm);
    } else {
        /* Unsupported compression method */
        fclose(out);
        remove(extracted_path);
        free(data);
        return -1;
    }
    
    fclose(out);
    free(data);
    return 0;
}
