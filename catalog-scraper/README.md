# DSi Game Store - Catalog Scraper

Python tool to scrape game metadata and generate `catalog.json` for the DSi Game Store.

## ⚠️ Legal Notice

This scraper collects **metadata only**:
- Game titles
- Cover images
- Genres and descriptions
- File sizes
- Detail page URLs

It does **NOT** download ROM files. Only links to legally/authorized distributed content are recorded. Commercial copyrighted ROMs are excluded.

## Installation

```bash
cd catalog-scraper
pip install -r requirements.txt
```

## Usage

### Full Scrape
```bash
python scraper.py
```

### Resume Interrupted Scrape
```bash
python scraper.py --resume
```

### Limit Pages
```bash
python scraper.py --max-pages 5
```

### Update Existing Catalog
```bash
python scraper.py --update
```

### Custom Delay
```bash
python scraper.py --delay 2.0
```

## Output

```
catalog/
├── catalog.json       # Generated catalog (DSi-compatible)
└── covers/
    ├── game_001.jpg   # Cover images
    ├── game_002.jpg
    └── ...
```

## catalog.json Format

Compatible with DSi Game Store parser:

```json
{
  "version": "1.0.0",
  "last_updated": "2026-11-11",
  "total_games": 10,
  "games": [
    {
      "id": 1,
      "title": "Game Title",
      "description": "Description...",
      "genre": 8,
      "genre_name": "Homebrew",
      "year": 2024,
      "file_size": 524288,
      "download_url": "https://...",
      "cover_url": "covers/game_001.jpg",
      "source_name": "romsfun.com (metadata)",
      "compatibility": "DSi / DSi XL",
      "authorized_source": true,
      "license": "See source",
      "added_date": 1700000000,
      "detail_url": "https://romsfun.com/..."
    }
  ]
}
```

## Integration with Backend

After scraping, copy the catalog to the backend:

```bash
cp catalog/catalog.json ../backend/data/catalog.json
cp -r catalog/covers ../backend/public/covers/
```

Or the scraper does this automatically.

## Automation

### Cron (Linux/Mac)
```bash
# Update catalog daily at 3 AM
0 3 * * * cd /path/to/catalog-scraper && python scraper.py --update >> logs/cron.log 2>&1
```

### Windows Task Scheduler
Create a task running `python scraper.py --update` daily.

## Error Handling

- Failed requests are retried 3 times with exponential backoff
- Progress is saved after each page
- Use `--resume` to continue interrupted scrapes
- Errors are logged to `logs/scraper.log` and `logs/errors.json`

## Notes

- The scraper respects the target site by adding delays between requests
- Only metadata is collected, no ROM files are downloaded
- Cover images are resized conceptually (DSi displays at 64x64)
- The DSi app downloads catalog.json from the backend server
