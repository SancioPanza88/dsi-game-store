"""
Configuration for DSi Game Store Catalog Scraper
Edit these values to customize scraping behavior.
"""

# Target site
BASE_URL = "https://romsfun.com"
CATALOG_PATH = "/roms/nintendo-ds/"

# Output paths (relative to scraper directory)
OUTPUT_DIR = "catalog"
COVERS_SUBDIR = "covers"
CATALOG_FILENAME = "catalog.json"

# Request settings
REQUEST_DELAY = 1.5       # Seconds between requests
REQUEST_TIMEOUT = 30     # Request timeout in seconds
MAX_RETRIES = 3          # Max retry attempts per request

# Scraping limits
MAX_PAGES = None          # None = all pages, or set a number
MAX_GAMES = 500           # Maximum games in catalog (DSi memory limit)

# Cover image settings
COVER_MAX_WIDTH = 64      # Pixels (DSi cover size)
COVER_MAX_HEIGHT = 64
COVER_QUALITY = 85        # JPEG quality

# Authorized content filter
# Only games matching these criteria are included
REQUIRE_AUTHORIZED = True  # Skip non-authorized content
AUTHORIZED_KEYWORDS = [
    "homebrew", "open source", "public domain", "freeware",
    "demo", "prototype", "beta", "fan-made", "fan made",
    "indie", "community", "cc0", "cc-by", "mit", "gpl",
    "apache", "bsd", "creative commons", "open-source",
]

# Genre mapping
GENRE_MAP = {
    "action": 1,
    "adventure": 0,
    "platform": 2,
    "platformer": 2,
    "racing": 3,
    "rpg": 4,
    "role-playing": 4,
    "puzzle": 5,
    "sports": 6,
    "strategy": 7,
    "homebrew": 8,
}

# DSi genre names
GENRE_NAMES = [
    "Adventure", "Action", "Platform", "Racing", "RPG",
    "Puzzle", "Sports", "Strategy", "Homebrew", "Other"
]
