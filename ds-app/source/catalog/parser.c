/*
 * DSi Game Store - Catalog Module
 * Game catalog management, parsing, filtering, search
 */

#include "../include/common.h"
#include "../include/catalog.h"
#include "../include/network.h"
#include "../include/storage.h"

/* Internal state */
static Game filtered_games[MAX_GAMES];
static int filtered_count = 0;
static Game search_results[MAX_GAMES];
static int search_count = 0;
static bool search_active = false;
static bool filter_active = false;

static int recent_indices[MAX_GAMES];
static int recent_count = 0;

/* Genre names */
static const char *genre_names[GENRE_COUNT] = {
    "Adventure", "Action", "Platform", "Racing", "RPG",
    "Puzzle", "Sports", "Strategy", "Homebrew", "Other"
};

/* Parse a size string like "32 MB" / "1.2 GB" / "512 KB" to bytes */
static unsigned long parse_size_string(const char *s) {
    if (!s) return 0;
    while (*s == ' ') s++;
    double val = atof(s);
    while (*s && ((*s >= '0' && *s <= '9') || *s == '.' || *s == ' ')) s++;
    unsigned long mult = 1;
    if (strncasecmp(s, "GB", 2) == 0) mult = 1024UL * 1024UL * 1024UL;
    else if (strncasecmp(s, "MB", 2) == 0) mult = 1024UL * 1024UL;
    else if (strncasecmp(s, "KB", 2) == 0) mult = 1024UL;
    return (unsigned long)(val * mult);
}

/* Initialize catalog */
void catalog_init(void) {
    app.total_games = 0;
    filtered_count = 0;
    search_count = 0;
    search_active = false;
    filter_active = false;
    recent_count = 0;
    
    /* Serverless-first: load catalog.json directly from SD card.
       Fall back to cached copy, then to network if Wi-Fi is up. */
    if (catalog_load_from_sd_card()) {
        /* Loaded from SD card */
    } else if (catalog_load_from_cache()) {
        /* Cache loaded successfully */
    } else if (app.wifi_connected) {
        catalog_fetch_from_server();
    }
}

/* Load catalog directly from SD card (no server required) */
bool catalog_load_from_sd_card(void) {
    if (!app.sd_mounted) return false;
    const char *path = "sd:/roms/nds/catalog.json";
    if (!storage_file_exists(path)) return false;
    
    FILE *f = fopen(path, "r");
    if (!f) return false;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0 || size > 2 * 1024 * 1024) {
        fclose(f);
        return false;
    }
    
    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }
    
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    
    bool result = catalog_parse_json(buffer);
    free(buffer);
    
    /* Load downloaded status */
    for (int i = 0; i < app.total_games; i++) {
        storage_load_game_info(&app.games[i], app.games[i].id);
    }
    
    return result;
}

/* Load catalog from local cache */
bool catalog_load_from_cache(void) {
    if (!app.sd_mounted) return false;
    if (!storage_file_exists(CATALOG_CACHE)) return false;
    
    FILE *f = fopen(CATALOG_CACHE, "r");
    if (!f) return false;
    
    /* Read file into buffer */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0 || size > 256 * 1024) { /* Max 256KB cache */
        fclose(f);
        return false;
    }
    
    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }
    
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    
    bool result = catalog_parse_json(buffer);
    free(buffer);
    
    /* Load covers from cache */
    for (int i = 0; i < app.total_games; i++) {
        if (storage_load_cover(app.games[i].id, app.games[i].cover_data, 
                              COVER_WIDTH * COVER_HEIGHT * 2)) {
            app.games[i].cover_loaded = true;
        }
    }
    
    /* Load downloaded status */
    for (int i = 0; i < app.total_games; i++) {
        storage_load_game_info(&app.games[i], app.games[i].id);
    }
    
    return result;
}

/* Fetch catalog from server */
bool catalog_fetch_from_server(void) {
    if (!app.wifi_connected) return false;
    
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/catalog", app.server_url);
    
    HttpResponse *resp = http_get(url);
    if (!resp || !resp->success) {
        if (resp) free(resp);
        return false;
    }
    
    bool result = catalog_parse_json(resp->body);
    
    /* Save to cache */
    if (result) {
        catalog_save_to_cache();
    }
    
    /* Free response */
    if (resp->body) free(resp->body);
    free(resp);
    
    return result;
}

/* Parse JSON catalog */
bool catalog_parse_json(const char *json_data) {
    if (!json_data) return false;
    
    /* Find games array */
    const char *games_start = strstr(json_data, "\"games\"");
    if (!games_start) return false;
    
    games_start = strchr(games_start, '[');
    if (!games_start) return false;
    games_start++;
    
    /* Parse each game object */
    app.total_games = 0;
    
    const char *p = games_start;
    while (*p && app.total_games < MAX_GAMES) {
        /* Find next object */
        const char *obj_start = strchr(p, '{');
        if (!obj_start) break;
        
        /* Find matching closing brace */
        int depth = 0;
        const char *obj_end = obj_start;
        while (*obj_end) {
            if (*obj_end == '{') depth++;
            else if (*obj_end == '}') {
                depth--;
                if (depth == 0) break;
            }
            obj_end++;
        }
        
        if (depth != 0) break;
        
        /* Extract game object as string */
        int obj_len = obj_end - obj_start + 1;
        char *obj_str = (char *)malloc(obj_len + 1);
        if (!obj_str) break;
        
        memcpy(obj_str, obj_start, obj_len);
        obj_str[obj_len] = '\0';
        
        /* Parse game fields */
        Game *g = &app.games[app.total_games];
        memset(g, 0, sizeof(Game));
        
        g->id = json_get_int(obj_str, "id");
        json_get_string(obj_str, "title", g->title, MAX_TITLE_LEN);
        json_get_string(obj_str, "description", g->description, MAX_DESC_LEN);
        /* Genre can be string or int */
        char genre_str[64];
        if (json_get_string(obj_str, "genre", genre_str, sizeof(genre_str)) > 0) {
            /* Map genre string to ID */
            if (strcasecmp(genre_str, "adventure") == 0) g->genre = GENRE_ADVENTURE;
            else if (strcasecmp(genre_str, "action") == 0) g->genre = GENRE_ACTION;
            else if (strcasecmp(genre_str, "platform") == 0) g->genre = GENRE_PLATFORM;
            else if (strcasecmp(genre_str, "racing") == 0) g->genre = GENRE_RACING;
            else if (strcasecmp(genre_str, "rpg") == 0) g->genre = GENRE_RPG;
            else if (strcasecmp(genre_str, "puzzle") == 0) g->genre = GENRE_PUZZLE;
            else if (strcasecmp(genre_str, "sports") == 0) g->genre = GENRE_SPORTS;
            else if (strcasecmp(genre_str, "strategy") == 0) g->genre = GENRE_STRATEGY;
            else if (strcasecmp(genre_str, "homebrew") == 0) g->genre = GENRE_HOMEBREW;
            else g->genre = GENRE_OTHER;
        } else {
            g->genre = json_get_int(obj_str, "genre");
        }
        g->year = json_get_int(obj_str, "year");
        g->file_size = json_get_long(obj_str, "file_size");
        json_get_string(obj_str, "download_url", g->download_url, MAX_URL_LEN);
        json_get_string(obj_str, "cover", g->cover_url, MAX_URL_LEN);
        json_get_string(obj_str, "source_name", g->source_name, 64);
        json_get_string(obj_str, "compatibility", g->compatibility, 64);
        
        /* Parse versions array */
        g->num_versions = 0;
        g->selected_version = 0;
        const char *ver_key = strstr(obj_str, "\"versions\"");
        if (ver_key) {
            const char *varr = strchr(ver_key, '[');
            if (varr) {
                const char *vp = varr + 1;
                while (*vp && g->num_versions < MAX_VERSIONS) {
                    const char *vstart = strchr(vp, '{');
                    if (!vstart) break;
                    int vdepth = 0;
                    const char *vend = vstart;
                    while (*vend) {
                        if (*vend == '{') vdepth++;
                        else if (*vend == '}') { vdepth--; if (vdepth == 0) break; }
                        vend++;
                    }
                    if (vdepth != 0) break;
                    int vlen = vend - vstart + 1;
                    char *vobj = (char *)malloc(vlen + 1);
                    if (!vobj) break;
                    memcpy(vobj, vstart, vlen);
                    vobj[vlen] = '\0';
                    
                    GameVersion *v = &g->versions[g->num_versions];
                    memset(v, 0, sizeof(GameVersion));
                    json_get_string(vobj, "name", v->name, sizeof(v->name));
                    json_get_string(vobj, "region", v->region, sizeof(v->region));
                    json_get_string(vobj, "format", v->format, sizeof(v->format));
                    json_get_string(vobj, "download_url", v->download_url, MAX_URL_LEN);
                    char sz[32];
                    if (json_get_string(vobj, "size", sz, sizeof(sz)) > 0) {
                        v->size = parse_size_string(sz);
                    }
                    v->download_available = json_get_bool(vobj, "download_available");
                    free(vobj);
                    g->num_versions++;
                    vp = vend + 1;
                }
            }
        }
        
        /* Choose first available version for quick size/url */
        if (g->num_versions > 0) {
            for (int vi = 0; vi < g->num_versions; vi++) {
                if (g->versions[vi].download_available && g->versions[vi].download_url[0]) {
                    g->selected_version = vi;
                    g->file_size = g->versions[vi].size;
                    strcpy(g->download_url, g->versions[vi].download_url);
                    break;
                }
            }
        }
        
        /* Clamp genre */
        if (g->genre < 0 || g->genre >= GENRE_COUNT) {
            g->genre = GENRE_OTHER;
        }
        
        /* Check if already downloaded */
        char filename[128];
        snprintf(filename, sizeof(filename), "%s%s.nds", NDS_ROMS_DIR, g->title);
        /* Sanitize filename */
        for (char *c = filename; *c; c++) {
            if (*c == ' ' || *c == '/' || *c == '\\') *c = '_';
        }
        strcpy(g->local_path, filename);
        g->is_downloaded = storage_file_exists(g->local_path);
        
        app.total_games++;
        free(obj_str);
        
        p = obj_end + 1;
    }
    
    /* Check catalog version */
    json_get_string(json_data, "version", app.catalog_version, sizeof(app.catalog_version));
    
    return (app.total_games > 0);
}

/* Save catalog to cache */
bool catalog_save_to_cache(void) {
    if (!app.sd_mounted) return false;
    
    /* Fetch fresh catalog and save */
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/api/catalog", app.server_url);
    
    HttpResponse *resp = http_get(url);
    if (!resp || !resp->success) {
        if (resp) free(resp);
        return false;
    }
    
    FILE *f = fopen(CATALOG_CACHE, "w");
    if (!f) {
        if (resp->body) free(resp->body);
        free(resp);
        return false;
    }
    
    fwrite(resp->body, 1, resp->body_length, f);
    fclose(f);
    
    if (resp->body) free(resp->body);
    free(resp);
    return true;
}

/* Get game count */
int catalog_get_game_count(void) {
    if (search_active) return search_count;
    if (filter_active) return filtered_count;
    return app.total_games;
}

/* Get game by index */
Game *catalog_get_game(int index) {
    if (index < 0 || index >= app.total_games) return NULL;
    return &app.games[index];
}

/* Get game by ID */
Game *catalog_get_game_by_id(int id) {
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].id == id) return &app.games[i];
    }
    return NULL;
}

/* Find game index by ID */
int catalog_find_game_index(int id) {
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].id == id) return i;
    }
    return -1;
}

/* Set genre filter */
void catalog_set_genre_filter(GameGenre genre) {
    filter_active = (genre != GENRE_COUNT);
    app.filter_genre = genre;
    
    if (!filter_active) {
        filtered_count = 0;
        return;
    }
    
    filtered_count = 0;
    for (int i = 0; i < app.total_games && filtered_count < MAX_GAMES; i++) {
        if (app.games[i].genre == genre) {
            filtered_games[filtered_count++] = app.games[i];
        }
    }
}

/* Clear filter */
void catalog_clear_filter(void) {
    filter_active = false;
    filtered_count = 0;
}

/* Get filtered count */
int catalog_get_filtered_count(void) {
    return filtered_count;
}

/* Get filtered game */
Game *catalog_get_filtered_game(int index) {
    if (index < 0 || index >= filtered_count) return NULL;
    return &filtered_games[index];
}

/* Search games by name */
void catalog_search(const char *query) {
    if (!query || strlen(query) == 0) {
        catalog_clear_search();
        return;
    }
    
    search_active = true;
    search_count = 0;
    
    /* Case-insensitive search */
    char lower_query[64];
    strncpy(lower_query, query, sizeof(lower_query) - 1);
    for (char *c = lower_query; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') *c += 32;
    }
    
    for (int i = 0; i < app.total_games && search_count < MAX_GAMES; i++) {
        char lower_title[MAX_TITLE_LEN];
        strncpy(lower_title, app.games[i].title, sizeof(lower_title) - 1);
        for (char *c = lower_title; *c; c++) {
            if (*c >= 'A' && *c <= 'Z') *c += 32;
        }
        
        if (strstr(lower_title, lower_query) != NULL) {
            search_results[search_count++] = app.games[i];
        }
    }
    
    strncpy(app.search_query, query, sizeof(app.search_query) - 1);
}

/* Clear search */
void catalog_clear_search(void) {
    search_active = false;
    search_count = 0;
    app.search_query[0] = '\0';
}

/* Check if search is active */
bool catalog_is_search_active(void) {
    return search_active;
}

/* Check if filter is active */
bool catalog_is_filter_active(void) {
    return filter_active;
}

/* Get search count */
int catalog_get_search_count(void) {
    return search_count;
}

/* Get search result */
Game *catalog_get_search_result(int index) {
    if (index < 0 || index >= search_count) return NULL;
    return &search_results[index];
}

/* Sort by title (bubble sort - simple for DSi) */
void catalog_sort_by_title(void) {
    for (int i = 0; i < app.total_games - 1; i++) {
        for (int j = 0; j < app.total_games - i - 1; j++) {
            if (strcasecmp(app.games[j].title, app.games[j+1].title) > 0) {
                Game temp = app.games[j];
                app.games[j] = app.games[j+1];
                app.games[j+1] = temp;
            }
        }
    }
}

/* Sort by date */
void catalog_sort_by_date(void) {
    for (int i = 0; i < app.total_games - 1; i++) {
        for (int j = 0; j < app.total_games - i - 1; j++) {
            if (app.games[j].year > app.games[j+1].year) {
                Game temp = app.games[j];
                app.games[j] = app.games[j+1];
                app.games[j+1] = temp;
            }
        }
    }
}

/* Sort by size */
void catalog_sort_by_size(void) {
    for (int i = 0; i < app.total_games - 1; i++) {
        for (int j = 0; j < app.total_games - i - 1; j++) {
            if (app.games[j].file_size > app.games[j+1].file_size) {
                Game temp = app.games[j];
                app.games[j] = app.games[j+1];
                app.games[j+1] = temp;
            }
        }
    }
}

/* Toggle favorite */
void catalog_toggle_favorite(int game_index) {
    if (game_index < 0 || game_index >= app.total_games) return;
    
    app.games[game_index].is_favorite = !app.games[game_index].is_favorite;
    
    if (app.games[game_index].is_favorite) {
        app.favorite_count++;
    } else {
        app.favorite_count--;
    }
    
    storage_save_state();
}

/* Get favorite count */
int catalog_get_favorite_count(void) {
    int count = 0;
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].is_favorite) count++;
    }
    return count;
}

/* Get favorite by index */
Game *catalog_get_favorite(int index) {
    int count = 0;
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].is_favorite) {
            if (count == index) return &app.games[i];
            count++;
        }
    }
    return NULL;
}

/* Add to recent */
void catalog_add_recent(int game_index) {
    if (game_index < 0 || game_index >= app.total_games) return;
    
    /* Remove if already in recent */
    for (int i = 0; i < recent_count; i++) {
        if (recent_indices[i] == game_index) {
            /* Shift */
            for (int j = i; j < recent_count - 1; j++) {
                recent_indices[j] = recent_indices[j+1];
            }
            recent_count--;
            break;
        }
    }
    
    /* Add to front */
    if (recent_count < MAX_GAMES) {
        for (int i = recent_count; i > 0; i--) {
            recent_indices[i] = recent_indices[i-1];
        }
        recent_indices[0] = game_index;
        recent_count++;
    }
}

/* Get recent count */
int catalog_get_recent_count(void) {
    return recent_count;
}

/* Get recent by index */
Game *catalog_get_recent(int index) {
    if (index < 0 || index >= recent_count) return NULL;
    int gi = recent_indices[index];
    if (gi < 0 || gi >= app.total_games) return NULL;
    return &app.games[gi];
}

/* Genre name */
const char *catalog_genre_name(GameGenre genre) {
    if (genre < 0 || genre >= GENRE_COUNT) return "Unknown";
    return genre_names[genre];
}

/* Get genre count */
int catalog_get_genre_count(GameGenre genre) {
    int count = 0;
    for (int i = 0; i < app.total_games; i++) {
        if (app.games[i].genre == genre) count++;
    }
    return count;
}
