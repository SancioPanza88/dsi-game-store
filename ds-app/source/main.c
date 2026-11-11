/*
 * DSi Game Store - Main Entry Point
 * Nintendo DSi Homebrew Application
 *
 * A digital game store for Nintendo DSi, similar to Nintendo eShop.
 * Supports browsing, searching, downloading authorized content to SD card.
 *
 * Build: make (requires devkitPro, devkitARM, libnds, libfat, dswifi)
 * Output: dsi-game-store.nds
 */

#include "common.h"
#include <dswifi9.h>
#include <fat.h>
/* console.h not needed - using custom rendering */
/* sprite.h not needed for text mode */





/* Module includes */
#include "../include/ui.h"
#include "../include/network.h"
#include "../include/storage.h"
#include "../include/catalog.h"
#include "../include/download.h"

/* Global state */
AppState app;

/* Screen transition state */
static TransitionType current_transition = TRANS_NONE;
static int transition_progress = 0;
static bool transition_active = false;

/* Frame counter for animations */
static int frame_counter = 0;

/* Initialize application */
void app_init(void) {
    /* Initialize DS hardware */
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    
    /* Initialize VRAM */
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_SUB_SPRITE);
    
    /* Initialize backgrounds */
    bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    bgInitSub(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    
    /* Initialize sprites */
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    oamInit(&oamSub, SpriteMapping_1D_32, false);
    
    /* Initialize input */
    touchInit();
    
    /* Initialize SD card */
    if (!fatInitDefault()) {
        /* SD card not available */
        app.sd_mounted = false;
    } else {
        app.sd_mounted = true;
    }
    
    /* Initialize Wi-Fi */
    app.wifi_connected = false;
    
    /* Initialize app state */
    app.current_screen = SCREEN_SPLASH;
    app.previous_screen = SCREEN_SPLASH;
    app.selected_game_index = 0;
    app.catalog_scroll_offset = 0;
    app.catalog_selected_row = 0;
    app.filter_genre = GENRE_COUNT; /* All genres */
    app.total_games = 0;
    app.downloaded_count = 0;
    app.favorite_count = 0;
    app.download_in_progress = false;
    app.active_download_index = -1;
    app.download_queue_count = 0;
    app.settings_selected = 0;
    app.sound_enabled = true;
    app.brightness = 100;
    
    /* Default server URL */
    strcpy(app.server_url, "http://192.168.1.100:3000");
    
    /* Initialize UI */
    ui_init();
    
    /* Initialize storage directories */
    storage_init();
    
    /* Initialize catalog */
    catalog_init();
    
    /* Initialize download manager */
    download_init();
}

/* Main update loop */
void app_update(void) {
    frame_counter++;
    
    /* Scan input */
    scanKeys();
    
    /* Update Wi-Fi status */
    if (app.wifi_connected) {
        /* Check if still connected */
        if (!wifi_check_connection()) {
            app.wifi_connected = false;
        }
    }
    
    /* Update current screen */
    switch (app.current_screen) {
        case SCREEN_SPLASH:
            screen_splash_update();
            break;
        case SCREEN_MAIN_MENU:
            screen_main_menu_update();
            break;
        case SCREEN_CATALOG:
            screen_catalog_update();
            break;
        case SCREEN_CATEGORIES:
            screen_categories_update();
            break;
        case SCREEN_SEARCH:
            screen_search_update();
            break;
        case SCREEN_GAME_DETAIL:
            screen_game_detail_update();
            break;
        case SCREEN_DOWNLOAD:
            screen_download_update();
            break;
        case SCREEN_DOWNLOAD_QUEUE:
            screen_download_queue_update();
            break;
        case SCREEN_LIBRARY:
            screen_library_update();
            break;
        case SCREEN_FAVORITES:
            screen_favorites_update();
            break;
        case SCREEN_SETTINGS:
            screen_settings_update();
            break;
        default:
            break;
    }
    
    /* Update download manager */
    download_update();
    
    /* Update transition */
    if (transition_active) {
        transition_progress += 4;
        if (transition_progress >= 256) {
            transition_progress = 256;
            transition_active = false;
        }
    }
}

/* Main render loop */
void app_render(void) {
    /* Clear screens */
    /* (Handled by individual screen renderers) */
    
    /* Render current screen */
    switch (app.current_screen) {
        case SCREEN_SPLASH:
            screen_splash_render();
            break;
        case SCREEN_MAIN_MENU:
            screen_main_menu_render();
            break;
        case SCREEN_CATALOG:
            screen_catalog_render();
            break;
        case SCREEN_CATEGORIES:
            screen_categories_render();
            break;
        case SCREEN_SEARCH:
            screen_search_render();
            break;
        case SCREEN_GAME_DETAIL:
            screen_game_detail_render();
            break;
        case SCREEN_DOWNLOAD:
            screen_download_render();
            break;
        case SCREEN_DOWNLOAD_QUEUE:
            screen_download_queue_render();
            break;
        case SCREEN_LIBRARY:
            screen_library_render();
            break;
        case SCREEN_FAVORITES:
            screen_favorites_render();
            break;
        case SCREEN_SETTINGS:
            screen_settings_render();
            break;
        default:
            break;
    }
    
    /* Update OAM */
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);
}

/* Cleanup */
void app_cleanup(void) {
    /* Save state */
    storage_save_state();
    
    /* Cleanup network */
    network_cleanup();
    
    /* Unmount SD */
    if (app.sd_mounted) {
        /* fatUnmount handled by libfat */
    }
}

/* Navigate to a screen */
void navigate_to(ScreenID screen) {
    app.previous_screen = app.current_screen;
    app.current_screen = screen;
    transition_active = true;
    transition_progress = 0;
    current_transition = TRANS_FADE;
}

/* Go back to previous screen */
void navigate_back(void) {
    ScreenID temp = app.current_screen;
    app.current_screen = app.previous_screen;
    app.previous_screen = temp;
    transition_active = true;
    transition_progress = 0;
    current_transition = TRANS_SLIDE_RIGHT;
}

/* Main function */
int main(int argc, char **argv) {
    /* Initialize */
    app_init();
    
    /* Main loop */
    while (1) {
        /* Wait for VBlank */
        swiWaitForVBlank();
        
        /* Update */
        app_update();
        
        /* Render */
        app_render();
    }
    
    /* Cleanup (never reached in normal operation) */
    app_cleanup();
    return 0;
}
