/*
 * DSi Game Store - Storage Module
 * SD card access via libfat
 */

#include "../include/common.h"
#include "../include/storage.h"
#include <fat.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>

/* Initialize storage and create directory structure */
void storage_init(void) {
    /* fatInitDefault already called in app_init */
    if (!app.sd_mounted) return;
    
    /* Create directory structure */
    storage_create_dir(ROMS_DIR);
    storage_create_dir(NDS_ROMS_DIR);
    storage_create_dir(COVERS_DIR);
    storage_create_dir(TEMP_DIR);
    
    /* Load saved state */
    storage_load_state();
    
    /* Scan for downloaded games */
    /* (populated in catalog module) */
}

/* Check if storage is available */
bool storage_check_available(void) {
    return app.sd_mounted;
}

/* Get free space in bytes */
unsigned long storage_get_free_space(void) {
    if (!app.sd_mounted) return 0;
    
    /* Use statfs or similar */
    /* Simplified: return estimated free space */
    struct statvfs st;
    if (statvfs("sd:/", &st) == 0) {
        return (unsigned long)st.f_bavail * st.f_frsize;
    }
    return 0;
}

/* Get total space in bytes */
unsigned long storage_get_total_space(void) {
    if (!app.sd_mounted) return 0;
    
    struct statvfs st;
    if (statvfs("sd:/", &st) == 0) {
        return (unsigned long)st.f_blocks * st.f_frsize;
    }
    return 0;
}

/* Create directory */
bool storage_create_dir(const char *path) {
    if (!path) return false;
    
    /* Check if exists first */
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    /* Create directory */
    if (mkdir(path, 0777) == 0) {
        return true;
    }
    
    /* Try creating parent directories */
    char temp_path[MAX_PATH_LEN];
    strncpy(temp_path, path, MAX_PATH_LEN - 1);
    
    char *p = temp_path;
    if (*p == '/') p++;
    
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp_path, 0777);
            *p = '/';
        }
        p++;
    }
    mkdir(temp_path, 0777);
    
    return storage_dir_exists(path);
}

/* Check if directory exists */
bool storage_dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

/* Check if file exists */
bool storage_file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

/* Delete file */
bool storage_delete_file(const char *path) {
    if (!path) return false;
    return (remove(path) == 0);
}

/* Rename file */
bool storage_rename_file(const char *old_path, const char *new_path) {
    if (!old_path || !new_path) return false;
    return (rename(old_path, new_path) == 0);
}

/* Get file size */
unsigned long storage_get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (unsigned long)st.st_size;
    }
    return 0;
}

/* Save game info to SD */
bool storage_save_game_info(const Game *game) {
    if (!game || !app.sd_mounted) return false;
    
    /* Create info file: /roms/nds/.game_<id>.info */
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s.game_%d.info", NDS_ROMS_DIR, game->id);
    
    FILE *f = fopen(path, "w");
    if (!f) return false;
    
    fprintf(f, "id=%d\n", game->id);
    fprintf(f, "title=%s\n", game->title);
    fprintf(f, "genre=%d\n", game->genre);
    fprintf(f, "year=%d\n", game->year);
    fprintf(f, "size=%lu\n", game->file_size);
    fprintf(f, "path=%s\n", game->local_path);
    fprintf(f, "source=%s\n", game->source_name);
    
    fclose(f);
    return true;
}

/* Load game info from SD */
bool storage_load_game_info(Game *game, int id) {
    if (!game || !app.sd_mounted) return false;
    
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s.game_%d.info", NDS_ROMS_DIR, id);
    
    FILE *f = fopen(path, "r");
    if (!f) return false;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64], value[192];
        if (sscanf(line, "%63[^=]=%191[^\n]", key, value) == 2) {
            if (strcmp(key, "id") == 0) game->id = atoi(value);
            else if (strcmp(key, "title") == 0) strcpy(game->title, value);
            else if (strcmp(key, "genre") == 0) game->genre = atoi(value);
            else if (strcmp(key, "year") == 0) game->year = atoi(value);
            else if (strcmp(key, "size") == 0) game->file_size = atol(value);
            else if (strcmp(key, "path") == 0) strcpy(game->local_path, value);
            else if (strcmp(key, "source") == 0) strcpy(game->source_name, value);
        }
    }
    
    fclose(f);
    game->is_downloaded = storage_file_exists(game->local_path);
    return true;
}

/* List downloaded games */
int storage_list_downloaded_games(Game *games, int max) {
    if (!games || max <= 0 || !app.sd_mounted) return 0;
    
    DIR *dir = opendir(NDS_ROMS_DIR);
    if (!dir) return 0;
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL && count < max) {
        /* Look for .nds files */
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".nds") == 0) {
            /* Found a game file */
            memset(&games[count], 0, sizeof(Game));
            games[count].is_downloaded = true;
            snprintf(games[count].local_path, MAX_PATH_LEN, "%s%s", NDS_ROMS_DIR, entry->d_name);
            games[count].file_size = storage_get_file_size(games[count].local_path);
            
            /* Try to load info */
            /* Extract title from filename */
            char name[64];
            strncpy(name, entry->d_name, sizeof(name) - 1);
            char *dot = strrchr(name, '.');
            if (dot) *dot = '\0';
            strncpy(games[count].title, name, MAX_TITLE_LEN - 1);
            
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

/* Delete game */
bool storage_delete_game(const Game *game) {
    if (!game) return false;
    
    /* Delete the .nds file */
    if (storage_file_exists(game->local_path)) {
        storage_delete_file(game->local_path);
    }
    
    /* Delete info file */
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s.game_%d.info", NDS_ROMS_DIR, game->id);
    storage_delete_file(path);
    
    /* Delete cover */
    snprintf(path, sizeof(path), "%scover_%d.bin", COVERS_DIR, game->id);
    storage_delete_file(path);
    
    return true;
}

/* Save app state */
bool storage_save_state(void) {
    if (!app.sd_mounted) return false;
    
    FILE *f = fopen(STATE_FILE, "w");
    if (!f) return false;
    
    fprintf(f, "version=%s\n", APP_VERSION);
    fprintf(f, "server_url=%s\n", app.server_url);
    fprintf(f, "sound_enabled=%d\n", app.sound_enabled);
    fprintf(f, "brightness=%d\n", app.brightness);
    fprintf(f, "catalog_version=%s\n", app.catalog_version);
    
    /* Save favorites */
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].is_favorite) {
            fprintf(f, "favorite=%d\n", app.games[i].id);
        }
    }
    
    /* Save download queue state */
    for (int i = 0; i < app.download_queue_count; i++) {
        DownloadTask *t = &app.download_queue[i];
        fprintf(f, "download=%d|%s|%s|%lu|%lu|%d\n",
                t->game_id, t->game_title, t->dest_path,
                t->total_size, t->downloaded_size, t->state);
    }
    
    fclose(f);
    return true;
}

/* Load app state */
bool storage_load_state(void) {
    if (!app.sd_mounted) return false;
    if (!storage_file_exists(STATE_FILE)) return false;
    
    FILE *f = fopen(STATE_FILE, "r");
    if (!f) return false;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char key[64], value[448];
        if (sscanf(line, "%63[^=]=%447[^\n]", key, value) == 2) {
            if (strcmp(key, "server_url") == 0) {
                strcpy(app.server_url, value);
            } else if (strcmp(key, "sound_enabled") == 0) {
                app.sound_enabled = atoi(value);
            } else if (strcmp(key, "brightness") == 0) {
                app.brightness = atoi(value);
            } else if (strcmp(key, "catalog_version") == 0) {
                strcpy(app.catalog_version, value);
            } else if (strcmp(key, "favorite") == 0) {
                int id = atoi(value);
                for (int i = 0; i < app.total_games; i++) {
                    if (app.games[i].id == id) {
                        app.games[i].is_favorite = true;
                        app.favorite_count++;
                    }
                }
            }
        }
    }
    
    fclose(f);
    return true;
}

/* Save cover image data */
bool storage_save_cover(int game_id, const unsigned char *data, int size) {
    if (!data || size <= 0 || !app.sd_mounted) return false;
    
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%scover_%d.bin", COVERS_DIR, game_id);
    
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    
    fwrite(data, 1, size, f);
    fclose(f);
    return true;
}

/* Load cover image data */
bool storage_load_cover(int game_id, unsigned char *data, int max_size) {
    if (!data || max_size <= 0 || !app.sd_mounted) return false;
    
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%scover_%d.bin", COVERS_DIR, game_id);
    
    if (!storage_file_exists(path)) return false;
    
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    
    int read = fread(data, 1, max_size, f);
    fclose(f);
    
    return (read > 0);
}

/* Create temp file for download */
bool storage_create_temp_file(const char *filename, char *out_path) {
    if (!filename || !out_path) return false;
    
    snprintf(out_path, MAX_PATH_LEN, "%s%s.tmp", TEMP_DIR, filename);
    return true;
}

/* Commit temp file (rename to final) */
bool storage_commit_temp_file(const char *temp_path, const char *final_path) {
    if (!temp_path || !final_path) return false;
    return storage_rename_file(temp_path, final_path);
}

/* Cleanup temp files */
bool storage_cleanup_temp(void) {
    if (!app.sd_mounted) return false;
    
    DIR *dir = opendir(TEMP_DIR);
    if (!dir) return false;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s%s", TEMP_DIR, entry->d_name);
        storage_delete_file(path);
    }
    
    closedir(dir);
    return true;
}
