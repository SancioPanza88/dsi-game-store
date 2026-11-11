/*
 * DSi Game Store - Common Definitions
 * Nintendo DSi Homebrew Application
 *
 * Platform: Nintendo DSi / DSi XL
 * Compiler: devkitARM (devkitPro)
 * Libraries: libnds, libfat, dswifi
 */

#ifndef COMMON_H
#define COMMON_H

#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* Application version */
#define APP_NAME        "DSi Game Store"
#define APP_VERSION     "1.0.0"
#define APP_AUTHOR      "Homebrew Community"

/* Screen dimensions */
#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   192

/* DSi specific */
#define DSi_MODE        true

/* Memory limits */
#define MAX_GAMES       500
#define MAX_CATEGORIES  16
#define MAX_TITLE_LEN   64
#define MAX_DESC_LEN    512
#define MAX_URL_LEN     256
#define MAX_PATH_LEN    256
#define COVER_WIDTH     64
#define COVER_HEIGHT    64
#define THUMB_WIDTH     32
#define THUMB_HEIGHT    32

/* Download states */
typedef enum {
    DL_STATE_IDLE = 0,
    DL_STATE_CONNECTING,
    DL_STATE_DOWNLOADING,
    DL_STATE_PAUSED,
    DL_STATE_VERIFYING,
    DL_STATE_COMPLETE,
    DL_STATE_ERROR,
    DL_STATE_CANCELLED
} DownloadState;

/* Screen IDs */
typedef enum {
    SCREEN_SPLASH = 0,
    SCREEN_MAIN_MENU,
    SCREEN_CATALOG,
    SCREEN_CATEGORIES,
    SCREEN_SEARCH,
    SCREEN_GAME_DETAIL,
    SCREEN_DOWNLOAD,
    SCREEN_DOWNLOAD_QUEUE,
    SCREEN_LIBRARY,
    SCREEN_FAVORITES,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} ScreenID;

/* Game genres */
typedef enum {
    GENRE_ADVENTURE = 0,
    GENRE_ACTION,
    GENRE_PLATFORM,
    GENRE_RACING,
    GENRE_RPG,
    GENRE_PUZZLE,
    GENRE_SPORTS,
    GENRE_STRATEGY,
    GENRE_HOMEBREW,
    GENRE_OTHER,
    GENRE_COUNT
} GameGenre;

/* Game version (region/edition) */
#define MAX_VERSIONS 6

typedef struct {
    char name[48];   /* e.g. "New Super Mario Bros. (Europe)" */
    char region[8];  /* EU, US, JP, World... */
    char format[8];  /* nds, zip */
    unsigned long size;
    bool download_available;
    char download_url[MAX_URL_LEN];
} GameVersion;

/* Game structure */
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char description[MAX_DESC_LEN];
    GameGenre genre;
    int year;
    unsigned long file_size;
    char download_url[MAX_URL_LEN];
    char cover_url[MAX_URL_LEN];
    char source_name[64];
    char compatibility[64];
    bool is_favorite;
    bool is_downloaded;
    char local_path[MAX_PATH_LEN];
    unsigned char cover_data[COVER_WIDTH * COVER_HEIGHT * 2]; /* RGB565 */
    bool cover_loaded;
    int num_versions;
    int selected_version;
    GameVersion versions[MAX_VERSIONS];
} Game;

/* Download task */
typedef struct {
    int game_id;
    char game_title[MAX_TITLE_LEN];
    char url[MAX_URL_LEN];
    char dest_path[MAX_PATH_LEN];
    unsigned long total_size;
    unsigned long downloaded_size;
    DownloadState state;
    float speed_kbps;
    time_t start_time;
    time_t pause_time;
    int retry_count;
    char error_msg[128];
    bool hash_verified;
    char expected_hash[65];
    char actual_hash[65];
} DownloadTask;

/* App state */
typedef struct {
    ScreenID current_screen;
    ScreenID previous_screen;
    int selected_game_index;
    int catalog_scroll_offset;
    int catalog_selected_row;
    char search_query[64];
    GameGenre filter_genre;
    bool wifi_connected;
    bool sd_mounted;
    int free_space_mb;
    int total_games;
    int downloaded_count;
    int favorite_count;
    bool download_in_progress;
    int active_download_index;
    int download_queue_count;
    DownloadTask download_queue[8];
    Game games[MAX_GAMES];
    int settings_selected;
    bool sound_enabled;
    int brightness;
    char server_url[MAX_URL_LEN];
    char catalog_version[32];
} AppState;

/* Global app state */
extern AppState app;

/* Utility macros */
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* Color definitions (RGB565) */
#define COLOR_BG_DARK       RGB15(4, 8, 16)
#define COLOR_BG_LIGHT      RGB15(20, 24, 32)
#define COLOR_ACCENT        RGB15(0, 12, 24)
#define COLOR_HIGHLIGHT     RGB15(31, 20, 0)
#define COLOR_TEXT          RGB15(31, 31, 31)
#define COLOR_TEXT_DIM      RGB15(16, 16, 16)
#define COLOR_SUCCESS       RGB15(0, 24, 8)
#define COLOR_ERROR         RGB15(31, 8, 8)
#define COLOR_WARNING       RGB15(31, 24, 0)
#define COLOR_BUTTON        RGB15(8, 16, 28)
#define COLOR_BUTTON_PRESS  RGB15(12, 20, 32)

/* Screen transition */
typedef enum {
    TRANS_NONE = 0,
    TRANS_FADE,
    TRANS_SLIDE_LEFT,
    TRANS_SLIDE_RIGHT
} TransitionType;

/* Navigation functions */
void navigate_to(ScreenID screen);
void navigate_back(void);

/* Function declarations */
void app_init(void);
void app_update(void);
void app_render(void);
void app_cleanup(void);

#endif /* COMMON_H */
