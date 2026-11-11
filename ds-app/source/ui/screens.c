/*
 * DSi Game Store - UI Screens
 * All screen update and render functions
 */

#include "../include/common.h"
#include "../include/ui.h"
#include "../include/catalog.h"
#include "../include/download.h"
#include "../include/storage.h"
#include "../include/network.h"

/* UI state */
static int menu_selected = 0;
static int catalog_row = 0;
static int catalog_col = 0;
static int category_selected = 0;
static bool search_keyboard_active = false;
static int search_cursor = 0;
static int detail_scroll = 0;
static int settings_row = 0;

/* Button definitions */
static UIButton btn_download = {180, 150, 70, 25, "DOWNLOAD", false, true, RGB15(0,12,24), RGB15(0,16,32)};
static UIButton btn_back = {5, 170, 50, 15, "BACK", false, true, RGB15(8,8,8), RGB15(12,12,12)};
static UIButton btn_favorite = {200, 5, 50, 15, "FAV", false, true, RGB15(24,20,0), RGB15(28,24,0)};

/* Genre color mapping */
static u16 genre_colors[GENRE_COUNT] = {
    RGB15(8, 16, 28),   /* Adventure - blue */
    RGB15(28, 8, 8),    /* Action - red */
    RGB15(8, 24, 8),    /* Platform - green */
    RGB15(28, 20, 0),   /* Racing - orange */
    RGB15(16, 8, 24),   /* RPG - purple */
    RGB15(24, 24, 8),   /* Puzzle - yellow */
    RGB15(8, 20, 24),   /* Sports - cyan */
    RGB15(20, 20, 20),  /* Strategy - gray */
    RGB15(0, 24, 16),   /* Homebrew - teal */
    RGB15(16, 16, 16)   /* Other - dark gray */
};

/* ===== SPLASH SCREEN ===== */
void screen_splash_update(void) {
    /* Auto-advance after 2 seconds (120 frames) */
    static int splash_timer = 0;
    splash_timer++;
    
    if (splash_timer > 120 || (keysDown() & KEY_START)) {
        splash_timer = 0;
        navigate_to(SCREEN_MAIN_MENU);
    }
    
    /* Check for touch */
    touchPosition touch;
    touchRead(&touch);
    if (touch.px > 0 || touch.py > 0) {
        splash_timer = 0;
        navigate_to(SCREEN_MAIN_MENU);
    }
}

void screen_splash_render(void) {
    /* Clear top screen */
    /* Draw logo */
    /* For DSi, use text-based rendering since no graphics assets yet */
    
    /* Top screen: Logo and title */
    /* Use console for text output */
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    
    /* This is simplified - real implementation would use graphics */
}

/* ===== MAIN MENU ===== */
void screen_main_menu_update(void) {
    /* Handle D-pad navigation */
    u32 keys = keysDown();
    
    if (keys & KEY_UP) {
        if (menu_selected > 0) menu_selected--;
    }
    if (keys & KEY_DOWN) {
        if (menu_selected < 5) menu_selected++;
    }
    if (keys & KEY_A) {
        /* Select menu item */
        switch (menu_selected) {
            case 0: navigate_to(SCREEN_CATALOG); break;
            case 1: navigate_to(SCREEN_CATEGORIES); break;
            case 2: navigate_to(SCREEN_SEARCH); break;
            case 3: navigate_to(SCREEN_LIBRARY); break;
            case 4: navigate_to(SCREEN_FAVORITES); break;
            case 5: navigate_to(SCREEN_SETTINGS); break;
        }
    }
    if (keys & KEY_B) {
        navigate_to(SCREEN_SPLASH);
    }
    
    /* Touch handling */
    touchPosition touch;
    touchRead(&touch);
    if (touch.px > 0 && touch.py > 0) {
        /* Check which menu item was touched */
        int item_y = 40;
        for (int i = 0; i < 6; i++) {
            if (touch.py >= item_y && touch.py < item_y + 20 &&
                touch.px >= 20 && touch.px < 236) {
                menu_selected = i;
                /* Auto-select on touch */
                switch (menu_selected) {
                    case 0: navigate_to(SCREEN_CATALOG); break;
                    case 1: navigate_to(SCREEN_CATEGORIES); break;
                    case 2: navigate_to(SCREEN_SEARCH); break;
                    case 3: navigate_to(SCREEN_LIBRARY); break;
                    case 4: navigate_to(SCREEN_FAVORITES); break;
                    case 5: navigate_to(SCREEN_SETTINGS); break;
                }
                break;
            }
            item_y += 22;
        }
    }
}

void screen_main_menu_render(void) {
    /* Render main menu on bottom screen */
    /* Menu items */
    const char *menu_items[] = {
        "Catalog", "Categories", "Search",
        "Library", "Favorites", "Settings"
    };
    
    /* Top screen: Logo, selected game cover, info */
    /* Bottom screen: Menu grid */
    
    /* This would use actual graphics rendering */
    /* For now, text-based placeholder that compiles */
}

/* ===== CATALOG SCREEN ===== */
void screen_catalog_update(void) {
    u32 keys = keysDown();
    int visible_rows = 6;
    int total = catalog_get_game_count();
    
    if (keys & KEY_UP) {
        if (catalog_row > 0) catalog_row--;
        if (catalog_row < app.catalog_scroll_offset) {
            app.catalog_scroll_offset = catalog_row;
        }
    }
    if (keys & KEY_DOWN) {
        if (catalog_row < total - 1) catalog_row++;
        if (catalog_row >= app.catalog_scroll_offset + visible_rows) {
            app.catalog_scroll_offset = catalog_row - visible_rows + 1;
        }
    }
    if (keys & KEY_LEFT) {
        /* Previous page */
        app.catalog_scroll_offset -= visible_rows;
        if (app.catalog_scroll_offset < 0) app.catalog_scroll_offset = 0;
        catalog_row = app.catalog_scroll_offset;
    }
    if (keys & KEY_RIGHT) {
        /* Next page */
        app.catalog_scroll_offset += visible_rows;
        if (app.catalog_scroll_offset >= total) app.catalog_scroll_offset = MAX(0, total - visible_rows);
        catalog_row = app.catalog_scroll_offset;
    }
    if (keys & KEY_A) {
        /* Open game detail */
        Game *g = NULL;
        if (catalog_is_search_active()) g = catalog_get_search_result(catalog_row);
        else if (catalog_is_filter_active()) g = catalog_get_filtered_game(catalog_row);
        else g = catalog_get_game(catalog_row);
        
        if (g) {
            app.selected_game_index = catalog_find_game_index(g->id);
            catalog_add_recent(app.selected_game_index);
            navigate_to(SCREEN_GAME_DETAIL);
        }
    }
    if (keys & KEY_B) {
        catalog_clear_search();
        catalog_clear_filter();
        navigate_back();
    }
    if (keys & KEY_X) {
        /* Toggle favorite */
        Game *g = NULL;
        if (catalog_is_search_active()) g = catalog_get_search_result(catalog_row);
        else if (catalog_is_filter_active()) g = catalog_get_filtered_game(catalog_row);
        else g = catalog_get_game(catalog_row);
        
        if (g) {
            int idx = catalog_find_game_index(g->id);
            if (idx >= 0) catalog_toggle_favorite(idx);
        }
    }
    if (keys & KEY_Y) {
        /* Open search */
        navigate_to(SCREEN_SEARCH);
    }
    if (keys & KEY_START) {
        /* Open categories */
        navigate_to(SCREEN_CATEGORIES);
    }
    
    /* Touch handling */
    touchPosition touch;
    touchRead(&touch);
    if (touch.px > 0 && touch.py > 0) {
        /* Map touch to catalog row */
        int row_height = 28;
        int start_y = 10;
        int touched_row = (touch.py - start_y) / row_height;
        if (touched_row >= 0 && touched_row < visible_rows) {
            int actual_row = app.catalog_scroll_offset + touched_row;
            if (actual_row < total) {
                catalog_row = actual_row;
                /* Double-tap to open? Single tap selects */
                if (keysDown() & KEY_TOUCH) {
                    /* Open on touch */
                    Game *g = NULL;
                    if (catalog_is_search_active()) g = catalog_get_search_result(catalog_row);
                    else if (catalog_is_filter_active()) g = catalog_get_filtered_game(catalog_row);
                    else g = catalog_get_game(catalog_row);
                    
                    if (g) {
                        app.selected_game_index = catalog_find_game_index(g->id);
                        catalog_add_recent(app.selected_game_index);
                        navigate_to(SCREEN_GAME_DETAIL);
                    }
                }
            }
        }
    }
}

void screen_catalog_render(void) {
    /* Render catalog list */
    /* Top screen: selected game cover and info */
    /* Bottom screen: scrollable list */
    
    int total = catalog_get_game_count();
    
    /* Update top screen with selected game info */
    Game *selected = NULL;
    if (catalog_is_search_active()) selected = catalog_get_search_result(catalog_row);
    else if (catalog_is_filter_active()) selected = catalog_get_filtered_game(catalog_row);
    else selected = catalog_get_game(catalog_row);
    
    if (selected) {
        /* Display cover, title, description on top screen */
    }
    
    /* Bottom screen: list items */
    /* Each row: cover thumbnail, title, genre, size */
}

/* ===== CATEGORIES SCREEN ===== */
void screen_categories_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_UP) {
        if (category_selected > 0) category_selected--;
    }
    if (keys & KEY_DOWN) {
        if (category_selected < GENRE_COUNT) category_selected++;
    }
    if (keys & KEY_A) {
        if (category_selected == GENRE_COUNT) {
            /* All genres */
            catalog_clear_filter();
        } else {
            catalog_set_genre_filter((GameGenre)category_selected);
        }
        catalog_row = 0;
        app.catalog_scroll_offset = 0;
        navigate_to(SCREEN_CATALOG);
    }
    if (keys & KEY_B) {
        navigate_back();
    }
    
    /* Touch handling */
    touchPosition touch;
    touchRead(&touch);
    if (touch.px > 0 && touch.py > 0 && (keys & KEY_TOUCH)) {
        int item_height = 20;
        int start_y = 10;
        int touched = (touch.py - start_y) / item_height;
        if (touched >= 0 && touched <= GENRE_COUNT) {
            category_selected = touched;
            if (touched == GENRE_COUNT) {
                catalog_clear_filter();
            } else {
                catalog_set_genre_filter((GameGenre)touched);
            }
            catalog_row = 0;
            app.catalog_scroll_offset = 0;
            navigate_to(SCREEN_CATALOG);
        }
    }
}

void screen_categories_render(void) {
    /* Render category list */
    /* "All" + all genres */
}

/* ===== SEARCH SCREEN ===== */
void screen_search_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        catalog_clear_search();
        navigate_back();
        return;
    }
    
    /* Simple keyboard input via D-pad + A */
    /* In a real implementation, this would be an on-screen keyboard */
    
    if (keys & KEY_A) {
        /* Perform search with current query */
        if (strlen(app.search_query) > 0) {
            catalog_search(app.search_query);
            navigate_to(SCREEN_CATALOG);
        }
    }
    
    /* Touch-based keyboard would go here */
    touchPosition touch;
    touchRead(&touch);
    
    /* For now, handle L/R to clear */
    if (keys & KEY_L) {
        app.search_query[0] = '\0';
        search_cursor = 0;
    }
}

void screen_search_render(void) {
    /* Render search interface with on-screen keyboard */
    /* Show current query */
    /* Show keyboard */
}

/* ===== GAME DETAIL SCREEN ===== */
void screen_game_detail_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        navigate_back();
        return;
    }
    
    if (keys & KEY_A) {
        /* Download */
        Game *g = &app.games[app.selected_game_index];
        if (!g->is_downloaded) {
            download_start(g);
            navigate_to(SCREEN_DOWNLOAD);
        }
    }
    
    if (keys & KEY_X) {
        /* Toggle favorite */
        catalog_toggle_favorite(app.selected_game_index);
    }
    
    if (keys & KEY_Y) {
        /* Cycle through available versions */
        Game *g = &app.games[app.selected_game_index];
        if (g && g->num_versions > 1) {
            g->selected_version = (g->selected_version + 1) % g->num_versions;
        }
    }
    
    if (keys & KEY_UP) {
        if (detail_scroll > 0) detail_scroll--;
    }
    if (keys & KEY_DOWN) {
        detail_scroll++;
    }
    
    /* Touch handling for buttons */
    touchPosition touch;
    touchRead(&touch);
    if (touch.px > 0 && touch.py > 0 && (keys & KEY_TOUCH)) {
        /* Check download button */
        if (ui_touch_in_rect(btn_download.x, btn_download.y, btn_download.w, btn_download.h)) {
            Game *g = &app.games[app.selected_game_index];
            if (!g->is_downloaded) {
                download_start(g);
                navigate_to(SCREEN_DOWNLOAD);
            }
        }
        /* Check favorite button */
        if (ui_touch_in_rect(btn_favorite.x, btn_favorite.y, btn_favorite.w, btn_favorite.h)) {
            catalog_toggle_favorite(app.selected_game_index);
        }
        /* Check back button */
        if (ui_touch_in_rect(btn_back.x, btn_back.y, btn_back.w, btn_back.h)) {
            navigate_back();
        }
    }
}

void screen_game_detail_render(void) {
    Game *g = &app.games[app.selected_game_index];
    if (!g) return;
    
    /* Top screen: Large cover, title, genre, year, size */
    /* Bottom screen: Description, download button, favorite button */
    
    /* Render cover */
    ui_draw_cover(10, 10, g, COVER_WIDTH * 2);
    
    /* Render title */
    ui_draw_text(10, 10 + COVER_HEIGHT * 2 + 5, g->title, COLOR_TEXT, TEXT_LEFT);
    
    /* Render genre and year */
    char info[128];
    snprintf(info, sizeof(info), "%s | %d | %lu KB", 
             catalog_genre_name(g->genre), g->year, g->file_size / 1024);
    ui_draw_text(10, 10 + COVER_HEIGHT * 2 + 20, info, COLOR_TEXT_DIM, TEXT_LEFT);
    
    /* Render version selector */
    if (g->num_versions > 0) {
        int vi = g->selected_version;
        if (vi < 0 || vi >= g->num_versions) vi = 0;
        GameVersion *v = &g->versions[vi];
        char ver[128];
        snprintf(ver, sizeof(ver), "Ver %d/%d [%s] %s %s", vi + 1, g->num_versions,
                 v->region[0] ? v->region : "--", v->format[0] ? v->format : "nds",
                 v->download_available ? "" : "(n/d)");
        ui_draw_text(10, 10 + COVER_HEIGHT * 2 + 35, ver, COLOR_HIGHLIGHT, TEXT_LEFT);
    }
    
    /* Render description */
    ui_draw_text(10, 10 + COVER_HEIGHT * 2 + 40, g->description, COLOR_TEXT, TEXT_LEFT);
    
    /* Render buttons */
    ui_draw_button(&btn_download);
    ui_draw_button(&btn_favorite);
    ui_draw_button(&btn_back);
}

/* ===== DOWNLOAD SCREEN ===== */
void screen_download_update(void) {
    u32 keys = keysDown();
    
    int active = download_queue_get_active();
    if (active < 0) {
        /* No active download - go back */
        navigate_back();
        return;
    }
    
    DownloadTask *task = download_queue_get_task(active);
    if (!task) {
        navigate_back();
        return;
    }
    
    if (keys & KEY_B) {
        /* Pause */
        download_pause(active);
    }
    
    if (keys & KEY_A) {
        /* Resume if paused */
        if (task->state == DL_STATE_PAUSED) {
            download_resume(active);
        }
    }
    
    if (keys & KEY_X) {
        /* Cancel */
        download_cancel(active);
        navigate_back();
    }
    
    /* Check completion */
    if (task->state == DL_STATE_COMPLETE) {
        /* Show completion message, then return */
        static int complete_timer = 0;
        complete_timer++;
        if (complete_timer > 120) {
            complete_timer = 0;
            navigate_to(SCREEN_LIBRARY);
        }
    }
    
    if (task->state == DL_STATE_ERROR) {
        /* Show error, allow retry */
        if (keys & KEY_Y) {
            download_retry(active);
        }
    }
}

void screen_download_render(void) {
    int active = download_queue_get_active();
    if (active < 0) return;
    
    DownloadTask *task = download_queue_get_task(active);
    if (!task) return;
    
    /* Render download progress */
    /* Top screen: game name, size, progress bar, speed */
    /* Bottom screen: cancel/pause/resume buttons */
    
    char title[128];
    snprintf(title, sizeof(title), "Downloading: %s", task->game_title);
    ui_draw_text(10, 10, title, COLOR_TEXT, TEXT_LEFT);
    
    char size_info[128];
    snprintf(size_info, sizeof(size_info), "%lu / %lu KB", 
             task->downloaded_size / 1024, task->total_size / 1024);
    ui_draw_text(10, 30, size_info, COLOR_TEXT_DIM, TEXT_LEFT);
    
    /* Progress bar */
    float progress = download_get_progress(active);
    ui_draw_progress_bar(10, 50, 236, 15, progress, COLOR_SUCCESS, COLOR_BG_LIGHT);
    
    char percent[32];
    snprintf(percent, sizeof(percent), "%.1f%%", progress);
    ui_draw_text(128, 50, percent, COLOR_TEXT, TEXT_CENTER);
    
    /* Speed */
    unsigned long speed = download_get_speed(active);
    char speed_str[64];
    snprintf(speed_str, sizeof(speed_str), "Speed: %lu KB/s", speed / 1024);
    ui_draw_text(10, 75, speed_str, COLOR_TEXT_DIM, TEXT_LEFT);
    
    /* State */
    const char *state_str = download_state_to_string(task->state);
    ui_draw_text(10, 95, state_str, COLOR_TEXT, TEXT_LEFT);
    
    /* Error message */
    if (task->state == DL_STATE_ERROR) {
        ui_draw_text(10, 115, task->error_msg, COLOR_ERROR, TEXT_LEFT);
    }
}

/* ===== DOWNLOAD QUEUE SCREEN ===== */
void screen_download_queue_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        navigate_back();
    }
}

void screen_download_queue_render(void) {
    /* Render list of downloads in queue */
    for (int i = 0; i < app.download_queue_count; i++) {
        DownloadTask *task = &app.download_queue[i];
        /* Render each task with progress */
    }
}

/* ===== LIBRARY SCREEN ===== */
void screen_library_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        navigate_back();
    }
    
    /* Refresh downloaded games */
    static int refresh_timer = 0;
    refresh_timer++;
    if (refresh_timer > 300) { /* Refresh every 5 seconds */
        refresh_timer = 0;
        /* Re-scan SD for .nds files */
    }
}

void screen_library_render(void) {
    /* Render list of downloaded games */
    /* Each: title, size, launch button */
}

/* ===== FAVORITES SCREEN ===== */
void screen_favorites_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        navigate_back();
    }
    
    int fav_count = catalog_get_favorite_count();
    
    if (keys & KEY_UP) {
        if (catalog_row > 0) catalog_row--;
    }
    if (keys & KEY_DOWN) {
        if (catalog_row < fav_count - 1) catalog_row++;
    }
    if (keys & KEY_A) {
        Game *g = catalog_get_favorite(catalog_row);
        if (g) {
            app.selected_game_index = catalog_find_game_index(g->id);
            navigate_to(SCREEN_GAME_DETAIL);
        }
    }
}

void screen_favorites_render(void) {
    /* Render favorites list */
}

/* ===== SETTINGS SCREEN ===== */
void screen_settings_update(void) {
    u32 keys = keysDown();
    
    if (keys & KEY_B) {
        navigate_back();
        storage_save_state();
        return;
    }
    
    if (keys & KEY_UP) {
        if (settings_row > 0) settings_row--;
    }
    if (keys & KEY_DOWN) {
        if (settings_row < 4) settings_row++;
    }
    
    if (keys & KEY_LEFT || keys & KEY_RIGHT) {
        switch (settings_row) {
            case 0: /* Sound toggle */
                app.sound_enabled = !app.sound_enabled;
                break;
            case 1: /* Brightness */
                if (keys & KEY_LEFT) app.brightness = MAX(10, app.brightness - 10);
                else app.brightness = MIN(100, app.brightness + 10);
                break;
            case 2: /* Wi-Fi connect */
                if (keys & KEY_A) wifi_connect();
                break;
            case 3: /* Refresh catalog */
                if (keys & KEY_A) catalog_fetch_from_server();
                break;
            case 4: /* About */
                break;
        }
    }
    
    if (keys & KEY_A) {
        switch (settings_row) {
            case 2:
                wifi_connect();
                break;
            case 3:
                catalog_fetch_from_server();
                break;
        }
    }
}

void screen_settings_render(void) {
    /* Render settings */
    /* Sound: ON/OFF */
    /* Brightness: slider */
    /* Wi-Fi: status + connect button */
    /* Refresh catalog button */
    /* About */
}

/* ===== HELPER FUNCTIONS ===== */

void ui_init(void) {
    /* Initialize UI subsystem */
}

const char *genre_to_string(GameGenre genre) {
    return catalog_genre_name(genre);
}

const char *download_state_to_string(DownloadState state) {
    switch (state) {
        case DL_STATE_IDLE: return "Queued";
        case DL_STATE_CONNECTING: return "Connecting...";
        case DL_STATE_DOWNLOADING: return "Downloading...";
        case DL_STATE_PAUSED: return "Paused";
        case DL_STATE_VERIFYING: return "Verifying...";
        case DL_STATE_COMPLETE: return "Complete!";
        case DL_STATE_ERROR: return "Error";
        case DL_STATE_CANCELLED: return "Cancelled";
        default: return "Unknown";
    }
}
