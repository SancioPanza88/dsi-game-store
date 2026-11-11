# Technical Documentation - DSi Game Store

## Architecture

### System Overview

```
┌─────────────────┐     HTTP      ┌─────────────────┐
│  Nintendo DSi   │◄────────────►│  Backend Server  │
│  (Homebrew App) │              │  (Node.js)      │
│                 │              │                 │
│  - libnds       │              │  - Express      │
│  - libfat (SD)  │              │  - JSON catalog │
│  - dswifi       │              │  - Static files │
└─────────────────┘              └─────────────────┘
         │
         ▼
┌─────────────────┐
│   SD Card       │
│  /roms/nds/     │
│  - *.nds files  │
│  - covers/      │
│  - .state       │
└─────────────────┘
```

### DSi Application Modules

| Module | Purpose | Key Files |
|--------|---------|-----------|
| Main | Entry point, main loop | main.c |
| UI | Screen rendering, input | screens.c, render.c |
| Network | Wi-Fi, HTTP client | wifi.c |
| Storage | SD card, file I/O | sdcard.c |
| Catalog | Game data, search, filter | parser.c |
| Download | File transfer, resume | manager.c |

### Backend Modules

| Module | Purpose | Key Files |
|--------|---------|-----------|
| Server | Express app, middleware | server.js |
| Catalog | Game data, JSON | data/catalog.json |
| Covers | Image hosting | public/covers/ |

## Hardware Constraints

### Nintendo DSi Specifications

| Resource | DS Mode | DSi Mode |
|----------|---------|----------|
| CPU (ARM9) | 67 MHz | 133 MHz |
| CPU (ARM7) | 33 MHz | 33 MHz |
| RAM | 4 MB | 16 MB |
| VRAM | 656 KB | 656 KB |
| Screen | 256x192 x2 | 256x192 x2 |

### Memory Budget (DSi Mode)

| Component | Allocation |
|-----------|------------|
| System/OS | ~2 MB |
| App code | ~1 MB |
| Game catalog (500 games) | ~2 MB |
| Cover cache (on-demand) | ~1 MB |
| Network buffers | ~1 MB |
| UI buffers | ~1 MB |
| Free heap | ~8 MB |

### Network Limitations

- **Protocol**: HTTP only (no native HTTPS on DSi)
- **Wi-Fi**: 802.11b/g (max ~54 Mbps theoretical, ~10-20 Mbps practical)
- **TLS**: Not supported by dswifi/libnds
- **Workaround**: Use HTTP backend or HTTPS proxy

## Data Formats

### Catalog JSON Schema

```json
{
  "version": "string",
  "last_updated": "ISO date",
  "games": [
    {
      "id": "integer",
      "title": "string (max 64)",
      "description": "string (max 512)",
      "genre": "integer (0-9)",
      "genre_name": "string",
      "year": "integer",
      "file_size": "integer (bytes)",
      "download_url": "string (URL)",
      "cover_url": "string (path)",
      "source_name": "string",
      "compatibility": "string",
      "authorized_source": "boolean",
      "license": "string",
      "added_date": "unix timestamp"
    }
  ]
}
```

### Cover Image Format

- Format: RGB565 (16-bit)
- Size: 64x64 pixels = 8,192 bytes
- Storage: Binary file on SD card
- Cache: Loaded on-demand, freed when not visible

### State File Format

```
version=1.0.0
server_url=http://192.168.1.100:3000
sound_enabled=1
brightness=100
catalog_version=1.0.0
favorite=1
favorite=3
download=1|Game Title|sd:/roms/nds/.tmp/game.nds.tmp|1048576|524288|2
```

## Download Protocol

### Flow

1. User selects game → Download
2. App checks Wi-Fi connection
3. App checks SD space
4. App creates temp file: `/roms/nds/.tmp/<name>.nds.tmp`
5. HTTP GET to download URL
6. Stream data to temp file
7. Progress reported to UI (percentage, speed)
8. On completion: verify hash
9. Rename temp → final: `/roms/nds/<name>.nds`
10. Update catalog (is_downloaded = true)
11. Save state

### Resume Support

- Uses HTTP Range header: `Range: bytes=<offset>-`
- Temp file preserved on interruption
- On resume: check existing file size, send Range request
- If server doesn't support Range: restart download

### Integrity Verification

- Simple hash (djb2 variant) computed during download
- Compared with expected hash from catalog
- If mismatch: delete temp file, report error
- Hash stored in DownloadTask for verification

### Error Handling

- Connection timeout: retry up to 3 times
- Partial download: preserve temp file for resume
- SD full: abort, cleanup temp, notify user
- Hash mismatch: delete, notify user

## Wi-Fi Implementation

### dswifi Library

The app uses dswifi for Wi-Fi connectivity:
- Supports WEP, WPA, WPA2
- Does NOT support WPA3
- Uses DSi's saved connection settings
- Automatic AP connection

### HTTP Client

Custom HTTP client built on BSD sockets:
- GET requests only (sufficient for catalog + download)
- No HTTPS support (DSi limitation)
- Connection: close (no keep-alive)
- User-Agent: DSiGameStore/1.0

### Known Limitations

1. **No HTTPS**: DSi cannot verify TLS certificates. Use HTTP backend or proxy.
2. **No HTTP/2**: Only HTTP/1.1
3. **Limited concurrent connections**: Single connection at a time
4. **DNS resolution**: Basic gethostbyname only

## UI System

### Screen Layout

**Top Screen (256x192)**:
- Game cover (large)
- Game title
- Genre, year, size
- Description (scrollable)

**Bottom Screen (256x192)**:
- Navigation list / buttons
- Touch targets
- Progress bars
- On-screen keyboard (search)

### Input Methods

1. **D-pad + Buttons**: Full navigation
2. **Touch screen**: Direct selection
3. **L/R buttons**: Page navigation

### Navigation Flow

```
Splash → Main Menu → [Catalog | Categories | Search | Library | Favorites | Settings]
                      Catalog → Game Detail → Download
                      Categories → Catalog (filtered)
                      Search → Catalog (search results)
```

## Build System

### Prerequisites

- devkitPro (latest)
- devkitARM
- libnds
- libfat
- dswifi

### Build Commands

```bash
# Set environment
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM

# Build
cd ds-app
make

# Clean
make clean
```

### Output

- `dsi-game-store.nds` - Executable ROM file
- Size: ~500KB - 1MB (depending on assets)

## Testing

### Emulator Testing

1. Use DeSmuME or melonDS
2. Enable DSi mode in emulator
3. Mount SD card image
4. Load .nds file

### Real Hardware Testing

1. Copy .nds to SD card
2. Launch via TWiLight Menu++
3. Test all features:
   - Wi-Fi connection
   - Catalog loading
   - Search and filter
   - Game detail view
   - Download (small file first)
   - Resume after interruption
   - SD card write verification
   - TWiLight Menu++ recognition

### Test Checklist

- [ ] App launches on DSi
- [ ] Wi-Fi connects
- [ ] Catalog loads from server
- [ ] Covers display correctly
- [ ] Search works
- [ ] Filter by genre works
- [ ] Game detail shows all info
- [ ] Download completes successfully
- [ ] File written to correct path
- [ ] Downloaded game appears in TWiLight Menu++
- [ ] Game launches from TWiLight Menu++
- [ ] Pause/resume works
- [ ] Cancel works
- [ ] Favorites persist
- [ ] Settings persist
- [ ] Low memory conditions handled
- [ ] Network error handling works

## Performance Optimization

### Cover Loading

- Covers loaded on-demand (only visible games)
- Cached on SD card (binary format)
- Freed from RAM when not visible
- Thumbnail size: 32x32 for lists, 64x64 for detail

### Catalog Management

- Pagination: 6 games per page
- Lazy loading of game details
- Search is client-side (after catalog loaded)
- Filter is client-side

### Memory Management

- Static arrays (no dynamic allocation in hot paths)
- Cover data freed when screen changes
- Network buffers reused
- Temp files cleaned on startup

### Frame Rate

- Target: 60 FPS
- VBlank sync for rendering
- Non-blocking download (async via state machine)
- UI remains responsive during downloads
