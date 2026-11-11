# Installation Guide - DSi Game Store

## Prerequisites

### Hardware
- Nintendo DSi or Nintendo DSi XL
- SD card (minimum 2GB recommended)
- Wi-Fi network access

### Software
- TWiLight Menu++ installed on SD card
- nds-bootstrap (included with TWiLight Menu++)

## Installation Steps

### 1. Prepare SD Card

Ensure your SD card has this structure:
```
sd:/
├── _nds/
│   └── TWiLightMenu/     # TWiLight Menu++ files
├── roms/
│   └── nds/              # DS ROMs folder
└── (other files)
```

### 2. Copy Application

Copy `dsi-game-store.nds` to your SD card:
```
sd:/roms/nds/dsi-game-store.nds
```

### 3. Configure Server URL

The application needs to know where your backend server is.

**Option A: Default Server**
The app defaults to `http://192.168.1.100:3000`. 
If your server is at this address, no configuration needed.

**Option B: Custom Server**
1. Launch the app
2. Go to Settings
3. Enter your server URL
4. Save settings

### 4. Launch Application

1. Insert SD card into DSi
2. Power on DSi
3. Open TWiLight Menu++
4. Navigate to `dsi-game-store.nds`
5. Press A to launch

### 5. First Run

On first launch:
- App creates `/roms/nds/` directory structure
- Attempts to connect to Wi-Fi
- Fetches catalog from server
- Downloads and caches cover images

## Troubleshooting

### Wi-Fi Connection Failed
- Check DSi Wi-Fi settings (System Settings → Internet)
- Ensure WPA/WPA2 network
- DSi does NOT support WPA3
- Try moving closer to router

### SD Card Not Detected
- Reformat SD card as FAT32
- Check SD card lock switch
- Try different SD card

### Catalog Not Loading
- Verify server is running
- Check server URL in settings
- Ensure DSi and server on same network
- Check firewall settings

### Download Failed
- Check available SD space
- Verify Wi-Fi connection stability
- Try pausing and resuming
- Check server download URL

### Game Not Appearing in TWiLight Menu++
- Ensure file is in `/roms/nds/`
- File must have `.nds` extension
- Refresh TWiLight Menu++ (press SELECT)
- Check file isn't corrupted

## Uninstallation

1. Delete `dsi-game-store.nds` from SD card
2. Optionally delete `/roms/nds/.store_state` and cache files
3. Downloaded games remain in `/roms/nds/`

## Updating

1. Download new `dsi-game-store.nds`
2. Replace old file on SD card
3. Settings and downloads are preserved
