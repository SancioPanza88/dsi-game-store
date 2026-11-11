# DSi Game Store - Nintendo DSi Homebrew Digital Store

A complete digital game store for Nintendo DSi, similar to Nintendo eShop.
Browse, search, and download authorized homebrew games directly to your SD card.

## ⚠️ Important Legal Notice

This application is designed for **authorized content only**:
- Homebrew games (open source, freely distributable)
- Public domain software
- Backup copies the user is legally authorized to use

**This application does NOT support downloading commercial copyrighted ROMs from unauthorized distribution sites.**

## Project Structure

```
dsi-game-store/
├── ds-app/                    # Nintendo DSi homebrew application (C)
│   ├── source/                # C source code
│   │   ├── main.c            # Entry point
│   │   ├── ui/               # UI screens and rendering
│   │   ├── network/          # Wi-Fi and HTTP client
│   │   ├── storage/          # SD card and file management
│   │   ├── catalog/          # Game catalog management
│   │   └── download/         # Download manager
│   ├── include/              # Header files
│   ├── data/                 # Graphics, sounds, banners
│   ├── Makefile              # Build configuration
│   └── README.md
├── backend/                   # Backend server (Node.js)
│   ├── src/                  # Server source
│   ├── public/               # Static assets (covers, icons)
│   ├── data/                 # Catalog JSON data
│   └── package.json
├── docs/                      # Documentation
│   ├── INSTALLATION.md       # Installation guide
│   ├── SERVER_SETUP.md       # Backend server setup
│   ├── TECHNICAL.md          # Technical documentation
│   └── ARCHITECTURE.md       # System architecture
└── scripts/                   # Build and utility scripts
```

## Features

### DSi Application
- ✅ Browse game catalog
- ✅ Search by name
- ✅ Filter by genre/category
- ✅ Game detail view with cover, description, size
- ✅ Download to SD card with progress bar
- ✅ Download pause/resume/cancel
- ✅ Download queue management
- ✅ Favorites system
- ✅ Recent games
- ✅ Downloaded games library
- ✅ Settings (Wi-Fi, sound, brightness)
- ✅ Touch + button navigation
- ✅ Both screens used (top: info, bottom: navigation)

### Backend Server
- ✅ REST API for catalog
- ✅ Search and filter endpoints
- ✅ Category listing
- ✅ Game details
- ✅ Health check
- ✅ Hot-reload catalog (edit JSON, no restart needed)

## Quick Start

### Backend Server
```bash
cd backend
npm install
npm start
# Server runs on http://localhost:3000
```

### DSi Application
```bash
cd ds-app
# Requires devkitPro installed
make
# Output: dsi-game-store.nds
```

### Installation on DSi
1. Copy `dsi-game-store.nds` to your SD card
2. Launch via TWiLight Menu++
3. Configure server URL in Settings
4. Browse and download games!

## Requirements

### Development
- devkitPro with devkitARM
- libnds, libfat, dswifi
- Node.js 16+ (for backend)

### Runtime (DSi)
- Nintendo DSi or DSi XL
- TWiLight Menu++ installed
- SD card with sufficient space
- Wi-Fi connection

## License

This project is open source. See individual files for license information.

## Contributing

Contributions welcome! Please ensure all content is legally distributable.
