/*
 * DSi Game Store - Catalog Module
 * Game catalog management, parsing, filtering, search
 */

#ifndef CATALOG_H
#define CATALOG_H

#include "common.h"

/* Catalog functions */
void catalog_init(void);
bool catalog_load_from_sd_card(void);
bool catalog_load_from_cache(void);
bool catalog_fetch_from_server(void);
bool catalog_parse_json(const char *json_data);
bool catalog_save_to_cache(void);

/* Game access */
int catalog_get_game_count(void);
Game *catalog_get_game(int index);
Game *catalog_get_game_by_id(int id);
int catalog_find_game_index(int id);

/* Filtering */
void catalog_set_genre_filter(GameGenre genre);
void catalog_clear_filter(void);
int catalog_get_filtered_count(void);
Game *catalog_get_filtered_game(int index);

/* Search */
bool catalog_is_search_active(void);
bool catalog_is_filter_active(void);
void catalog_search(const char *query);
void catalog_clear_search(void);
int catalog_get_search_count(void);
Game *catalog_get_search_result(int index);

/* Sorting */
void catalog_sort_by_title(void);
void catalog_sort_by_date(void);
void catalog_sort_by_size(void);

/* Favorites */
void catalog_toggle_favorite(int game_index);
int catalog_get_favorite_count(void);
Game *catalog_get_favorite(int index);

/* Recent */
void catalog_add_recent(int game_index);
int catalog_get_recent_count(void);
Game *catalog_get_recent(int index);

/* Genres */
const char *catalog_genre_name(GameGenre genre);
int catalog_get_genre_count(GameGenre genre);

#endif /* CATALOG_H */
