/*
 * DSi Game Store - Storage Module
 * SD card access, file management, directory structure
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"

/* Storage paths */
#define SD_ROOT         "sd:/"
#define ROMS_DIR        "sd:/roms/"
#define NDS_ROMS_DIR    "sd:/roms/nds/"
#define COVERS_DIR      "sd:/roms/nds/covers/"
#define TEMP_DIR        "sd:/roms/nds/.tmp/"
#define STATE_FILE      "sd:/roms/nds/.store_state"
#define CATALOG_CACHE   "sd:/roms/nds/.catalog_cache"

/* Storage functions */
void storage_init(void);
bool storage_check_available(void);
unsigned long storage_get_free_space(void); /* in bytes */
unsigned long storage_get_total_space(void);

/* Directory operations */
bool storage_create_dir(const char *path);
bool storage_dir_exists(const char *path);
bool storage_file_exists(const char *path);

/* File operations */
bool storage_delete_file(const char *path);
bool storage_rename_file(const char *old_path, const char *new_path);
unsigned long storage_get_file_size(const char *path);

/* Game management */
bool storage_save_game_info(const Game *game);
bool storage_load_game_info(Game *game, int id);
int storage_list_downloaded_games(Game *games, int max);
bool storage_delete_game(const Game *game);

/* State persistence */
bool storage_save_state(void);
bool storage_load_state(void);

/* Cover cache */
bool storage_save_cover(int game_id, const unsigned char *data, int size);
bool storage_load_cover(int game_id, unsigned char *data, int max_size);

/* Temp file management */
bool storage_create_temp_file(const char *filename, char *out_path);
bool storage_commit_temp_file(const char *temp_path, const char *final_path);
bool storage_cleanup_temp(void);

#endif /* STORAGE_H */
