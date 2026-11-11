/*
 * DSi Game Store - ZIP Extraction Module
 * Minimal ZIP reader using zlib inflate
 */

#ifndef ZIP_H
#define ZIP_H

#include "common.h"

/* Extract a .nds file from a ZIP archive */
/* Returns 0 on success, -1 on error */
/* extracted_path: output path for the extracted .nds file */
int zip_extract_nds(const char *zip_path, const char *extract_dir, char *extracted_path, int max_path_len);

/* Check if a file is a ZIP archive */
bool zip_is_zip(const char *path);

/* List files in a ZIP archive */
int zip_list_files(const char *zip_path, char files[][MAX_PATH_LEN], int max_files);

#endif /* ZIP_H */
