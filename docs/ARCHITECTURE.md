# System Architecture - DSi Game Store

## Component Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                    Nintendo DSi (Client)                      │
│                                                              │
│  ┌────────────┐  ┌────────────┐  ┌─────────────────────┐    │
│  │   UI Layer │  │  Catalog   │  │  Download Manager   │    │
│  │  (Screens) │  │  (Parser)  │  │  (Queue + Resume)   │    │
│  └─────┬──────┘  └─────┬──────┘  └──────────┬──────────┘    │
│        │               │                      │              │
│  ┌─────┴───────────────┴──────────────────────┴──────────┐   │
│  │              Application Core (main.c)                 │   │
│  └─────┬───────────────┬──────────────────────┬──────────┘   │
│        │               │                      │              │
│  ┌─────┴──────┐  ┌─────┴──────┐  ┌──────────┴──────────┐   │
│  │  Network   │  │  Storage   │  │  Wi-Fi (dswifi)     │   │
│  │  (HTTP)    │  │  (libfat)  │  │                     │   │
│  └─────┬──────┘  └─────┬──────┘  └──────────┬──────────┘   │
│        │               │                      │              │
└────────┼───────────────┼──────────────────────┼──────────────┘
         │               │                      │
         │ HTTP          │ File I/O             │ 802.11
         ▼               ▼                      ▼
┌─────────────────┐ ┌──────────────┐    ┌──────────────┐
│  Backend Server │ │  SD Card     │    │  Wi-Fi Router │
│  (Node.js)      │ │  /roms/nds/  │    │              │
│                 │ │              │    └──────────────┘
│  /api/catalog   │ │  *.nds       │
│  /api/search    │ │  covers/     │
│  /download/:id  │ │  .state      │
│  /covers/*      │ │              │
└────────┬────────┘ └──────────────┘
         │
         ▼
┌─────────────────┐
│  Catalog Data   │
│  (JSON file)    │
│  + Cover Images │
│  + Download     │
│    Sources       │
└─────────────────┘
```

## Data Flow

### Catalog Loading

```
1. App starts
2. Check SD for cached catalog (.catalog_cache)
3. If cache exists and valid → load from cache
4. If no cache or stale → fetch from server
5. Parse JSON → populate app.games[]
6. Load covers from SD cache
7. Update download status for each game
```

### Download Flow

```
1. User selects game → presses DOWNLOAD
2. Validate Wi-Fi connection
3. Validate SD space (file_size + 1MB buffer)
4. Create download task in queue
5. Create temp file path
6. HTTP GET download URL
7. Stream response → temp file
8. Update progress (downloaded/total, speed)
9. On completion:
   a. Compute hash of temp file
   b. Compare with expected hash
   c. If match: rename temp → final
   d. If mismatch: delete temp, error
10. Update game.is_downloaded = true
11. Save state
12. Notify user: "DOWNLOAD COMPLETE!"
```

### Search Flow

```
1. User enters search query
2. Client-side filter of app.games[]
3. Case-insensitive substring match on title
4. Results displayed in catalog view
5. Clear search → show all games
```

## Security Considerations

### Content Authorization

- Backend only serves authorized content
- `authorized_source` flag in catalog
- Download URLs point to verified sources
- No automatic ROM scraping

### Network Security

- HTTP only (DSi limitation)
- No sensitive data transmitted
- Server URL configurable by user
- No authentication required (local network)

### File System Safety

- Temp files used for downloads (atomic commit)
- Never overwrite without confirmation
- Path sanitization (no directory traversal)
- SD space checked before download

## Scalability

### Catalog Size

- Max 500 games (memory constraint)
- Pagination: 6 per page
- Lazy cover loading
- Binary search for large catalogs (future)

### Concurrent Downloads

- Max 1 active download (DSi network limitation)
- Queue up to 8 downloads
- Sequential processing

### Server Load

- Stateless API (easy to scale)
- Static file serving (covers)
- Catalog cached on client (reduces requests)
- Gzip compression enabled
