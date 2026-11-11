/*
 * DSi Game Store - UI Rendering Primitives
 * Drawing functions for screens, buttons, text, progress bars
 */

#include "../include/common.h"
#include "../include/ui.h"

/* Top/bottom screen buffers */
/* In libnds, we draw to the framebuffer or use tile-based backgrounds */

/* Draw filled rectangle */
void ui_draw_rect(int x, int y, int w, int h, u16 color) {
    /* Draw to main screen framebuffer */
    /* Simplified: would use DMA or direct VRAM writes */
    for (int py = y; py < y + h && py < SCREEN_HEIGHT; py++) {
        for (int px = x; px < x + w && px < SCREEN_WIDTH; px++) {
            /* VRAM write - placeholder */
        }
    }
}

/* Draw text string */
void ui_draw_text(int x, int y, const char *text, u16 color, TextAlign align) {
    if (!text) return;
    
    int text_width = strlen(text) * 8; /* Assume 8px char width */
    
    int start_x = x;
    if (align == TEXT_CENTER) {
        start_x = x - text_width / 2;
    } else if (align == TEXT_RIGHT) {
        start_x = x - text_width;
    }
    
    /* Draw text using console or bitmap font */
    /* Placeholder: would render bitmap font glyphs */
}

/* Draw button */
void ui_draw_button(UIButton *btn) {
    if (!btn || !btn->visible) return;
    
    u16 color = btn->pressed ? btn->color_pressed : btn->color_normal;
    
    /* Draw button background */
    ui_draw_rect(btn->x, btn->y, btn->w, btn->h, color);
    
    /* Draw button border */
    /* (simplified) */
    
    /* Draw label centered */
    int text_x = btn->x + btn->w / 2;
    int text_y = btn->y + btn->h / 2 - 4;
    ui_draw_text(text_x, text_y, btn->label, COLOR_TEXT, TEXT_CENTER);
}

/* Draw cover image */
void ui_draw_cover(int x, int y, const Game *game, int size) {
    if (!game) return;
    
    if (game->cover_loaded) {
        /* Draw cover from cover_data (RGB565) */
        /* Scale to requested size */
    } else {
        /* Draw placeholder */
        ui_draw_rect(x, y, size, size, COLOR_BG_LIGHT);
        ui_draw_text(x + size/2, y + size/2, "?", COLOR_TEXT_DIM, TEXT_CENTER);
    }
}

/* Draw progress bar */
void ui_draw_progress_bar(int x, int y, int w, int h, float progress, u16 fg, u16 bg) {
    /* Background */
    ui_draw_rect(x, y, w, h, bg);
    
    /* Foreground (progress) */
    int fill_width = (int)(w * progress / 100.0f);
    if (fill_width > w) fill_width = w;
    if (fill_width < 0) fill_width = 0;
    
    ui_draw_rect(x, y, fill_width, h, fg);
    
    /* Border */
    /* (simplified) */
}

/* Draw scrollbar */
void ui_draw_scrollbar(int x, int y, int h, int total, int visible, int offset) {
    if (total <= visible) return;
    
    /* Track */
    ui_draw_rect(x, y, 4, h, COLOR_BG_LIGHT);
    
    /* Thumb */
    int thumb_h = (int)((float)visible / total * h);
    if (thumb_h < 10) thumb_h = 10;
    
    int thumb_y = y + (int)((float)offset / (total - visible) * (h - thumb_h));
    
    ui_draw_rect(x, thumb_y, 4, thumb_h, COLOR_HIGHLIGHT);
}

/* Touch in rect check */
bool ui_touch_in_rect(int x, int y, int w, int h) {
    touchPosition touch;
    touchRead(&touch);
    
    if (touch.px >= x && touch.px < x + w &&
        touch.py >= y && touch.py < y + h) {
        return true;
    }
    return false;
}

/* Get touched button index */
int ui_get_touched_button(UIButton *buttons, int count) {
    touchPosition touch;
    touchRead(&touch);
    
    if (touch.px == 0 && touch.py == 0) return -1;
    
    for (int i = 0; i < count; i++) {
        if (buttons[i].visible &&
            touch.px >= buttons[i].x && touch.px < buttons[i].x + buttons[i].w &&
            touch.py >= buttons[i].y && touch.py < buttons[i].y + buttons[i].h) {
            return i;
        }
    }
    return -1;
}
