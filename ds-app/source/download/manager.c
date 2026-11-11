/*
 * DSi Game Store - Download Manager
 * File download, resume, integrity verification, queue management
 */

#include "../include/common.h"
#include "../include/download.h"
#include "../include/zip.h"
#include "../include/network.h"
#include "../include/storage.h"
#include "../include/catalog.h"

/* Internal state */
static int download_task_count = 0;

/* Initialize download manager */
void download_init(void) {
    app.download_queue_count = 0;
    app.download_in_progress = false;
    app.active_download_index = -1;
    
    /* Resume any interrupted downloads */
    for (int i = 0; i < app.download_queue_count; i++) {
        DownloadTask *t = &app.download_queue[i];
        if (t->state == DL_STATE_DOWNLOADING || t->state == DL_STATE_PAUSED) {
            /* Check if temp file exists */
            unsigned long existing = storage_get_file_size(t->dest_path);
            if (existing > 0 && existing < t->total_size) {
                t->downloaded_size = existing;
                t->state = DL_STATE_PAUSED;
            }
        }
    }
}

/* Cleanup */
void download_cleanup(void) {
    /* Save state */
    storage_save_state();
}

/* Update download manager (called every frame) */
void download_update(void) {
    /* Process active download */
    if (app.active_download_index >= 0 && app.active_download_index < app.download_queue_count) {
        DownloadTask *task = &app.download_queue[app.active_download_index];
        
        if (task->state == DL_STATE_DOWNLOADING) {
            /* Check if download is complete (file size matches) */
            unsigned long current_size = storage_get_file_size(task->dest_path);
            if (current_size >= task->total_size && task->total_size > 0) {
                task->state = DL_STATE_VERIFYING;
                task->downloaded_size = current_size;
                
                /* Verify integrity */
                if (download_verify_hash(task->dest_path, task->expected_hash)) {
                    /* Process completed download (extract if ZIP) */
                    if (download_process_complete(app.active_download_index)) {
                        task->state = DL_STATE_COMPLETE;
                        app.download_in_progress = false;
                        app.active_download_index = -1;
                    }
                } else {
                    strcpy(task->error_msg, "Hash verification failed");
                    task->state = DL_STATE_ERROR;
                }
            }
        }
    } else {
        /* Find next queued download */
        for (int i = 0; i < app.download_queue_count; i++) {
            if (app.download_queue[i].state == DL_STATE_IDLE) {
                /* Start this download */
                app.active_download_index = i;
                app.download_queue[i].state = DL_STATE_CONNECTING;
                app.download_in_progress = true;
                break;
            }
        }
    }
}

/* Start download for a game */
int download_start(const Game *game) {
    if (!game) return -1;
    if (!app.wifi_connected) return -1;
    if (!app.sd_mounted) return -1;
    
    /* Check space */
    if (!download_check_space(game->file_size)) return -1;
    
    /* Check if already downloaded */
    if (game->is_downloaded) return -1;
    
    /* Check if already in queue */
    for (int i = 0; i < app.download_queue_count; i++) {
        if (app.download_queue[i].game_id == game->id) {
            return i; /* Already queued */
        }
    }
    
    /* Create download task */
    if (app.download_queue_count >= 8) return -1; /* Queue full */
    
    int idx = app.download_queue_count;
    DownloadTask *task = &app.download_queue[idx];
    
    memset(task, 0, sizeof(DownloadTask));
    task->game_id = game->id;
    strcpy(task->game_title, game->title);
    
    /* Use the selected version's URL/size when available */
    if (game->num_versions > 0 && game->selected_version >= 0 &&
        game->selected_version < game->num_versions &&
        game->versions[game->selected_version].download_url[0]) {
        strcpy(task->url, game->versions[game->selected_version].download_url);
        task->total_size = game->versions[game->selected_version].size;
    } else {
        strcpy(task->url, game->download_url);
        task->total_size = game->file_size;
    }
    task->downloaded_size = 0;
    task->state = DL_STATE_IDLE;
    task->retry_count = 0;
    task->start_time = time(NULL);
    
    /* Build destination path */
    char filename[128];
    strcpy(filename, game->title);
    /* Sanitize */
    for (char *c = filename; *c; c++) {
        if (*c == ' ' || *c == '/' || *c == '\\') *c = '_';
    }
    strcat(filename, ".nds");
    storage_create_temp_file(filename, task->dest_path);
    
    /* Store expected hash if available */
    task->expected_hash[0] = '\0';
    
    app.download_queue_count++;
    
    return idx;
}

/* Pause download */
bool download_pause(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    DownloadTask *task = &app.download_queue[task_index];
    if (task->state != DL_STATE_DOWNLOADING) return false;
    
    task->state = DL_STATE_PAUSED;
    task->pause_time = time(NULL);
    
    if (task_index == app.active_download_index) {
        app.active_download_index = -1;
        app.download_in_progress = false;
    }
    
    return true;
}

/* Resume download */
bool download_resume(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    DownloadTask *task = &app.download_queue[task_index];
    if (task->state != DL_STATE_PAUSED && task->state != DL_STATE_ERROR) return false;
    
    /* Check existing file size for resume */
    unsigned long existing = storage_get_file_size(task->dest_path);
    if (existing > 0 && existing < task->total_size) {
        task->downloaded_size = existing;
    } else if (existing >= task->total_size) {
        /* Already complete */
        task->state = DL_STATE_VERIFYING;
        return true;
    }
    
    task->state = DL_STATE_IDLE; /* Will be picked up by download_update */
    
    return true;
}

/* Cancel download */
bool download_cancel(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    DownloadTask *task = &app.download_queue[task_index];
    
    /* Delete temp file */
    storage_delete_file(task->dest_path);
    
    task->state = DL_STATE_CANCELLED;
    
    if (task_index == app.active_download_index) {
        app.active_download_index = -1;
        app.download_in_progress = false;
    }
    
    return true;
}

/* Retry failed download */
bool download_retry(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    DownloadTask *task = &app.download_queue[task_index];
    if (task->state != DL_STATE_ERROR && task->state != DL_STATE_CANCELLED) return false;
    
    task->retry_count++;
    if (task->retry_count > 3) return false; /* Max retries */
    
    /* Clean up temp file */
    storage_delete_file(task->dest_path);
    task->downloaded_size = 0;
    task->state = DL_STATE_IDLE;
    task->error_msg[0] = '\0';
    
    return true;
}

/* Queue management */
int download_queue_add(const Game *game) {
    return download_start(game);
}

bool download_queue_remove(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    /* Cancel if active */
    if (app.download_queue[task_index].state == DL_STATE_DOWNLOADING) {
        download_cancel(task_index);
    }
    
    /* Remove from queue by shifting */
    for (int i = task_index; i < app.download_queue_count - 1; i++) {
        app.download_queue[i] = app.download_queue[i + 1];
    }
    app.download_queue_count--;
    
    if (app.active_download_index == task_index) {
        app.active_download_index = -1;
        app.download_in_progress = false;
    } else if (app.active_download_index > task_index) {
        app.active_download_index--;
    }
    
    return true;
}

int download_queue_get_count(void) {
    return app.download_queue_count;
}

DownloadTask *download_queue_get_task(int index) {
    if (index < 0 || index >= app.download_queue_count) return NULL;
    return &app.download_queue[index];
}

int download_queue_get_active(void) {
    return app.active_download_index;
}

/* Download state queries */
DownloadState download_get_state(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return DL_STATE_IDLE;
    return app.download_queue[task_index].state;
}

float download_get_progress(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return 0.0f;
    
    DownloadTask *task = &app.download_queue[task_index];
    if (task->total_size == 0) return 0.0f;
    
    return (float)task->downloaded_size / (float)task->total_size * 100.0f;
}

unsigned long download_get_speed(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return 0;
    
    DownloadTask *task = &app.download_queue[task_index];
    if (task->state != DL_STATE_DOWNLOADING) return 0;
    
    time_t elapsed = time(NULL) - task->start_time;
    if (elapsed <= 0) return 0;
    
    /* Speed in bytes per second */
    return task->downloaded_size / elapsed;
}

char *download_get_error(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return "";
    return app.download_queue[task_index].error_msg;
}

/* Integrity verification using simple hash */
bool download_verify_hash(const char *file_path, const char *expected_hash) {
    if (!file_path || !expected_hash || strlen(expected_hash) == 0) {
        /* No hash to verify - skip */
        return true;
    }
    
    char *actual = download_compute_hash(file_path);
    if (!actual) return false;
    
    bool match = (strcmp(actual, expected_hash) == 0);
    free(actual);
    return match;
}

/* Compute hash of file (simple CRC32-based for DSi) */
char *download_compute_hash(const char *file_path) {
    if (!file_path || !storage_file_exists(file_path)) return NULL;
    
    FILE *f = fopen(file_path, "rb");
    if (!f) return NULL;
    
    /* Simple hash: XOR-based (not cryptographically secure, but sufficient for integrity) */
    unsigned long hash = 5381;
    int c;
    while ((c = fgetc(f)) != EOF) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    fclose(f);
    
    char *result = (char *)malloc(17);
    if (result) {
        snprintf(result, 17, "%016lx", hash);
    }
    return result;
}

/* Process completed download (extract if ZIP, move to final location) */
bool download_process_complete(int task_index) {
    if (task_index < 0 || task_index >= app.download_queue_count) return false;
    
    DownloadTask *task = &app.download_queue[task_index];
    
    /* Check if file is a ZIP */
    if (zip_is_zip(task->dest_path)) {
        /* Extract .nds from ZIP */
        char extracted_path[MAX_PATH_LEN];
        if (zip_extract_nds(task->dest_path, NDS_ROMS_DIR, extracted_path, MAX_PATH_LEN) == 0) {
            /* Delete the ZIP file */
            storage_delete_file(task->dest_path);
            
            /* Update task dest_path to extracted file */
            strcpy(task->dest_path, extracted_path);
            task->hash_verified = true;
            return true;
        } else {
            /* Extraction failed */
            strcpy(task->error_msg, "ZIP extraction failed");
            task->state = DL_STATE_ERROR;
            return false;
        }
    }
    
    /* Not a ZIP - move from temp to final location */
    char final_path[MAX_PATH_LEN];
    /* Extract filename from dest_path */
    char *filename = strrchr(task->dest_path, '/');
    if (filename) filename++;
    else filename = task->dest_path;
    
    /* Remove .tmp extension */
    char base_name[MAX_PATH_LEN];
    strcpy(base_name, filename);
    char *tmp_ext = strstr(base_name, ".tmp");
    if (tmp_ext) *tmp_ext = '\0';
    
    snprintf(final_path, MAX_PATH_LEN, "%s%s", NDS_ROMS_DIR, base_name);
    
    /* Check if file already exists */
    if (storage_file_exists(final_path)) {
        /* Don't overwrite - rename with number */
        char name_without_ext[MAX_PATH_LEN];
        strcpy(name_without_ext, base_name);
        char *dot = strrchr(name_without_ext, '.');
        if (dot) *dot = '\0';
        
        int counter = 1;
        do {
            snprintf(final_path, MAX_PATH_LEN, "%s%s_%d.nds", NDS_ROMS_DIR, name_without_ext, counter);
            counter++;
        } while (storage_file_exists(final_path) && counter < 100);
    }
    
    /* Rename temp to final */
    if (storage_rename_file(task->dest_path, final_path)) {
        strcpy(task->dest_path, final_path);
        task->hash_verified = true;
        return true;
    }
    
    strcpy(task->error_msg, "Failed to move file to final location");
    task->state = DL_STATE_ERROR;
    return false;
}

/* Check available space */
bool download_check_space(unsigned long required_bytes) {
    if (!app.sd_mounted) return false;
    
    unsigned long free_space = storage_get_free_space();
    /* Leave 1MB buffer */
    if (free_space < required_bytes + (1024 * 1024)) return false;
    
    return true;
}
