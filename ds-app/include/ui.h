/*
 * DSi Game Store - UI System
 * Handles screen rendering, touch input, button navigation
 */

#ifndef UI_H
#define UI_H

#include "common.h"

/* Button structure */
typedef struct {
    int x, y, w, h;
    char label[32];
    bool pressed;
    bool visible;
    u16 color_normal;
    u16 color_pressed;
} UIButton;

/* Text alignment */
typedef enum {
    TEXT_LEFT = 0,
    TEXT_CENTER,
    TEXT_RIGHT
} TextAlign;

/* UI initialization */
void ui_init(void);

/* Screen update/render functions */
void screen_splash_update(void);
void screen_splash_render(void);
void screen_main_menu_update(void);
void screen_main_menu_render(void);
void screen_catalog_update(void);
void screen_catalog_render(void);
void screen_categories_update(void);
void screen_categories_render(void);
void screen_search_update(void);
void screen_search_render(void);
void screen_game_detail_update(void);
void screen_game_detail_render(void);
void screen_download_update(void);
void screen_download_render(void);
void screen_download_queue_update(void);
void screen_download_queue_render(void);
void screen_library_update(void);
void screen_library_render(void);
void screen_favorites_update(void);
void screen_favorites_render(void);
void screen_settings_update(void);
void screen_settings_render(void);

/* Drawing primitives */
void ui_draw_rect(int x, int y, int w, int h, u16 color);
void ui_draw_text(int x, int y, const char *text, u16 color, TextAlign align);
void ui_draw_button(UIButton *btn);
void ui_draw_cover(int x, int y, const Game *game, int size);
void ui_draw_progress_bar(int x, int y, int w, int h, float progress, u16 fg, u16 bg);
void ui_draw_scrollbar(int x, int y, int h, int total, int visible, int offset);

/* Input helpers */
bool ui_touch_in_rect(int x, int y, int w, int h);
int ui_get_touched_button(UIButton *buttons, int count);

/* String utilities */
const char *genre_to_string(GameGenre genre);
const char *download_state_to_string(DownloadState state);

#endif /* UI_H */
