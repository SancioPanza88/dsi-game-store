/*
 * DSi Game Store - Download Manager
 * File download, resume, integrity verification, queue management
 */

#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "common.h"

/* Download functions */
void download_init(void);
void download_cleanup(void);
void download_update(void);

/* Download control */
int download_start(const Game *game);
bool download_pause(int task_index);
bool download_resume(int task_index);
bool download_cancel(int task_index);
bool download_retry(int task_index);

/* Queue management */
int download_queue_add(const Game *game);
bool download_queue_remove(int task_index);
int download_queue_get_count(void);
DownloadTask *download_queue_get_task(int index);
int download_queue_get_active(void);

/* Download info */
DownloadState download_get_state(int task_index);
float download_get_progress(int task_index);
unsigned long download_get_speed(int task_index);
char *download_get_error(int task_index);

/* Integrity */
bool download_verify_hash(const char *file_path, const char *expected_hash);
bool download_process_complete(int task_index);
char *download_compute_hash(const char *file_path);

/* Space check */
bool download_check_space(unsigned long required_bytes);

#endif /* DOWNLOAD_H */
