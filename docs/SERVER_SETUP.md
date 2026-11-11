# Backend Server Setup Guide

## Overview

The DSi Game Store backend provides:
- Game catalog API
- Metadata and descriptions
- Cover image hosting
- Download URL management

## Requirements

- Node.js 16 or higher
- npm or yarn
- 100MB+ disk space for covers

## Installation

### 1. Install Dependencies

```bash
cd backend
npm install
```

### 2. Configure Catalog

Edit `data/catalog.json` to add your authorized games:

```json
{
  "version": "1.0.0",
  "games": [
    {
      "id": 1,
      "title": "Game Name",
      "description": "Game description...",
      "genre": 0,
      "genre_name": "Adventure",
      "year": 2024,
      "file_size": 1048576,
      "download_url": "https://your-server.com/files/game.nds",
      "cover_url": "/covers/game.png",
      "source_name": "Your Source",
      "compatibility": "DSi / DSi XL",
      "authorized_source": true,
      "license": "MIT",
      "added_date": 1700000000
    }
  ]
}
```

### 3. Add Cover Images

Place cover images in `public/covers/`:
- Format: PNG
- Recommended size: 64x64 pixels (DSi will scale)
- Naming: match `cover_url` in catalog

### 4. Start Server

```bash
npm start
# or for development:
npm run dev
```

Server runs on port 3000 by default.

### 5. Verify

```bash
curl http://localhost:3000/api/health
# Should return: {"status":"ok","version":"1.0.0","games":N}

curl http://localhost:3000/api/catalog
# Should return full catalog JSON
```

## Network Configuration

### Local Network (Recommended for DSi)

1. Find your computer's IP:
   ```bash
   # Linux/Mac
   ip addr show
   # or
   ifconfig
   
   # Windows
   ipconfig
   ```

2. Ensure DSi and computer are on same network

3. Configure firewall to allow port 3000

4. Set server URL in DSi app: `http://YOUR_IP:3000`

### Internet Hosting

For internet access:
1. Deploy to a VPS or cloud provider
2. Use a reverse proxy (nginx) for HTTPS
3. Note: DSi HTTP client supports HTTP only
4. For HTTPS, use an HTTP-to-HTTPS proxy

## Content Guidelines

### Authorized Content Only

Only include content you are legally authorized to distribute:
- ✅ Homebrew (open source, GPL, MIT, etc.)
- ✅ Public domain software
- ✅ Your own creations
- ✅ Backup copies with proper licensing
- ❌ Commercial ROMs
- ❌ Cracked/pirated software
- ❌ Content without clear licensing

### Adding Games

1. Verify you have distribution rights
2. Upload game file to your server
3. Create cover image (64x64 PNG)
4. Add entry to catalog.json
5. Server auto-reloads catalog

### Download URLs

Download URLs should point to:
- Your own file server
- Authorized distribution mirrors
- GitHub releases (for open source)
- Internet Archive (for public domain)

## Docker Deployment (Optional)

```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY backend/package*.json ./
RUN npm install --production
COPY backend/ .
EXPOSE 3000
CMD ["node", "src/server.js"]
```

```bash
docker build -t dsi-game-store-backend .
docker run -p 3000:3000 dsi-game-store-backend
```

## Monitoring

### Health Check Endpoint
```
GET /api/health
```

### Logs
Server logs to stdout. Use process manager (pm2) for production:
```bash
npm install -g pm2
pm2 start src/server.js --name dsi-store
pm2 logs dsi-store
```
