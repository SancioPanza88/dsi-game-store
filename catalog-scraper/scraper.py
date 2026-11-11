#!/usr/bin/env python3
"""
DSi Game Store - Catalog Scraper
================================

Scrapes ALL Nintendo DS games from romsfun.com and generates catalog.json
with versions, covers, and direct download links.

Output format matches DSi app expectations.

Usage:
    python scraper.py                    # Full scrape (all pages)
    python scraper.py --max-pages 5      # Limit pages
    python scraper.py --resume           # Resume interrupted
"""

import os
import sys
import json
import time
import logging
import argparse
import re
import shutil
from pathlib import Path
from urllib.parse import urljoin, urlparse, unquote
from datetime import datetime

import cloudscraper
from bs4 import BeautifulSoup
from tqdm import tqdm

BASE_URL = "https://romsfun.com"
CATALOG_URL = f"{BASE_URL}/roms/nintendo-ds/"
OUTPUT_DIR = Path(__file__).parent / "catalog"
COVERS_DIR = OUTPUT_DIR / "covers"
CATALOG_FILE = OUTPUT_DIR / "catalog.json"
STATE_FILE = OUTPUT_DIR / ".scraper_state.json"
LOG_FILE = Path(__file__).parent / "logs" / "scraper.log"
BACKEND_DIR = Path(__file__).parent.parent / "backend"

REQUEST_DELAY = 1.5
REQUEST_TIMEOUT = 30
MAX_RETRIES = 3
USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"

GENRE_KEYWORDS = {
    "racing": "Racing", "race": "Racing", "kart": "Racing",
    "puzzle": "Puzzle",
    "rpg": "RPG", "quest": "RPG", "pokemon": "RPG", "dragon quest": "RPG",
    "action": "Action", "castlevania": "Action", "ninja": "Action",
    "adventure": "Adventure", "zelda": "Adventure", "explorer": "Adventure",
    "platform": "Platform", "mario": "Platform", "sonic": "Platform",
    "sports": "Sports", "sport": "Sports", "football": "Sports", "soccer": "Sports",
    "strategy": "Strategy",
}

LOG_FILE.parent.mkdir(parents=True, exist_ok=True)
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[logging.FileHandler(LOG_FILE), logging.StreamHandler(sys.stdout)]
)
logger = logging.getLogger(__name__)


class CatalogScraper:
    def __init__(self, max_pages=None, resume=False):
        self.scraper = cloudscraper.create_scraper()
        self.scraper.headers.update({"User-Agent": USER_AGENT})
        self.max_pages = max_pages
        self.resume = resume
        self.games = []
        self.seen_titles = set()
        self.errors = []
        self.stats = {"pages": 0, "games": 0, "versions": 0, "covers": 0, "errors": 0}

        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        COVERS_DIR.mkdir(parents=True, exist_ok=True)

        self.existing = self._load_existing()
        self.state = self._load_state() if resume else {}
        for g in self.existing.get("games", []):
            self.seen_titles.add(g.get("title", "").lower())

    def _load_existing(self):
        if CATALOG_FILE.exists():
            try:
                with open(CATALOG_FILE, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception as e:
                logger.error(f"Load existing failed: {e}")
        return {"catalog_version": 1, "games": []}

    def _load_state(self):
        if STATE_FILE.exists():
            try:
                with open(STATE_FILE, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception as e:
                logger.error(f"Load state failed: {e}")
        return {}

    def _save_state(self, page, url):
        state = {"last_page": page, "last_url": url, "timestamp": datetime.now().isoformat(),
                 "games": len(self.games)}
        with open(STATE_FILE, "w", encoding="utf-8") as f:
            json.dump(state, f, indent=2)

    def _fetch(self, url, retries=MAX_RETRIES):
        for attempt in range(retries):
            try:
                time.sleep(REQUEST_DELAY)
                resp = self.scraper.get(url, timeout=REQUEST_TIMEOUT)
                resp.raise_for_status()
                return resp
            except Exception as e:
                logger.warning(f"Attempt {attempt+1}/{retries} failed: {url}: {e}")
                if attempt < retries - 1:
                    time.sleep(2 ** attempt)
                else:
                    self.errors.append({"url": url, "error": str(e)})
                    self.stats["errors"] += 1
                    return None
        return None

    def _parse_size(self, s):
        if not s:
            return "0 MB"
        s = s.strip()
        # Return as-is if already formatted (e.g. "32 MB")
        if re.match(r"[\d.]+\s*(?:MB|KB|GB|B)", s, re.IGNORECASE):
            return s
        return s

    def _map_genre(self, title, description):
        text = (title + " " + (description or "")).lower()
        for kw, genre in GENRE_KEYWORDS.items():
            if kw in text:
                return genre
        return "Other"

    def _download_cover(self, cover_url, game_id):
        if not cover_url:
            return None
        try:
            if not cover_url.startswith("http"):
                cover_url = urljoin(BASE_URL, cover_url)
            resp = self._fetch(cover_url)
            if not resp:
                return None
            parsed = urlparse(cover_url)
            ext = os.path.splitext(parsed.path)[1].lower()
            if ext not in [".jpg", ".jpeg", ".png", ".gif", ".webp"]:
                ext = ".jpg"
            filename = f"game_{game_id:03d}{ext}"
            filepath = COVERS_DIR / filename
            with open(filepath, "wb") as f:
                f.write(resp.content)
            self.stats["covers"] += 1
            return f"covers/{filename}"
        except Exception as e:
            logger.error(f"Cover download failed {cover_url}: {e}")
            return None

    def _parse_listing(self, soup):
        links = []
        seen = set()
        for a in soup.find_all("a", href=True):
            href = a["href"]
            title = a.get_text(strip=True)
            # Require non-empty title first, so the image anchor (empty text)
            # does not claim the URL in `seen` before the title anchor.
            if not title:
                continue
            if "/roms/nintendo-ds/" not in href or "/page/" in href:
                continue
            full = urljoin(BASE_URL, href)
            if full == CATALOG_URL or full.rstrip('/') == CATALOG_URL.rstrip('/'):
                continue
            if full in seen:
                continue
            seen.add(full)
            links.append({"url": full, "title": title})
        return links

    def _parse_detail(self, url):
        resp = self._fetch(url)
        if not resp:
            return None
        soup = BeautifulSoup(resp.text, "lxml")
        game = {"url": url, "title": "", "description": "", "genre": "",
                "year": 0, "cover_url": "", "download_page": ""}

        # Title
        h1 = soup.find("h1")
        if h1:
            game["title"] = h1.get_text(strip=True)
        else:
            og = soup.find("meta", property="og:title")
            if og:
                game["title"] = og.get("content", "").replace(" ROM - Nintendo DS Game", "").strip()

        # Description
        meta_desc = soup.find("meta", attrs={"name": "description"})
        if meta_desc:
            game["description"] = meta_desc.get("content", "")[:512]
        if not game["description"]:
            og_desc = soup.find("meta", property="og:description")
            if og_desc:
                game["description"] = og_desc.get("content", "")[:512]

        # Cover
        og_img = soup.find("meta", property="og:image")
        if og_img:
            game["cover_url"] = og_img.get("content", "")

        # Year from release date
        page_text = soup.get_text()
        ym = re.search(r"(?:Release Date|Released)[:\s]+.*?(\d{4})", page_text, re.IGNORECASE)
        if ym:
            game["year"] = int(ym.group(1))
        else:
            ym = re.search(r"\b(20\d{2}|19\d{2})\b", page_text)
            if ym:
                game["year"] = int(ym.group())

        # Download page link
        for a in soup.find_all("a", href=True):
            if "/download/" in a["href"]:
                game["download_page"] = urljoin(BASE_URL, a["href"])
                break

        # Genre from table
        for table in soup.find_all("table"):
            for row in table.find_all("tr"):
                cells = row.find_all(["td", "th"])
                if len(cells) >= 2:
                    label = cells[0].get_text(strip=True).lower()
                    value = cells[1].get_text(strip=True)
                    if "type" in label:
                        # Could be genre hint
                        pass

        if not game["genre"]:
            game["genre"] = self._map_genre(game["title"], game["description"])

        return game

    def _parse_versions(self, download_page_url):
        """Parse download page to get all versions/regions with direct links."""
        versions = []
        if not download_page_url:
            return versions

        # First, get the main download page
        resp = self._fetch(download_page_url)
        if not resp:
            return versions

        soup = BeautifulSoup(resp.text, "lxml")
        body_text = soup.get_text()

        # Find region/version links (they look like /download/game-name-123/1, /2, etc.)
        region_links = []
        seen_indices = set()
        for a in soup.find_all("a", href=True):
            href = a["href"]
            text = a.get_text(strip=True)
            # Match download page with trailing number
            m = re.search(r"/download/[^\"']+/(\d+)$", href)
            if m:
                idx = int(m.group(1))
                if idx not in seen_indices:
                    seen_indices.add(idx)
                    full_url = urljoin(BASE_URL, href)
                    region_links.append({"index": idx, "url": full_url, "name": text})

        # If no region links found, try the main page itself
        if not region_links:
            region_links.append({"index": 1, "url": download_page_url, "name": "Default"})

        # For each region, get direct download URL
        for rl in region_links:
            try:
                region_resp = self._fetch(rl["url"])
                if not region_resp:
                    continue

                region_html = region_resp.text
                # Find direct download URL
                direct_match = re.search(r"https?://sto\.romsfast\.com/[^\s\"'<>]+", region_html)
                if not direct_match:
                    direct_match = re.search(r"https?://romsfast\.com/[^\s\"'<>]+\.zip[^\s\"'<>]*", region_html)

                if direct_match:
                    direct_url = direct_match.group(0)
                    direct_url = direct_url.replace("&#038;", "&").replace("&amp;", "&")

                    # Extract region/version name from the link text or page
                    version_name = rl["name"] if rl["name"] else f"Version {rl['index']}"

                    # Try to extract region from version name
                    region = "Unknown"
                    name_lower = version_name.lower()
                    if "europe" in name_lower or " (eu" in name_lower or "pal" in name_lower:
                        region = "EU"
                    elif "usa" in name_lower or " (us" in name_lower or "ntsc" in name_lower or "america" in name_lower:
                        region = "US"
                    elif "japan" in name_lower or " (jp" in name_lower or "jpn" in name_lower:
                        region = "JP"
                    elif "world" in name_lower:
                        region = "World"
                    elif "asia" in name_lower:
                        region = "Asia"
                    elif "korea" in name_lower:
                        region = "KR"

                    # Try to find file size
                    size = "0 MB"
                    # Look for size near the download link
                    size_match = re.search(r"(\d+\.?\d*\s*(?:MB|KB|GB))", region_html)
                    if size_match:
                        size = size_match.group(1).strip()

                    # Determine format
                    fmt = "zip" if ".zip" in direct_url.lower() else "nds"
                    archive = "zip" if ".zip" in direct_url.lower() else "none"

                    versions.append({
                        "name": version_name,
                        "region": region,
                        "format": fmt,
                        "size": size,
                        "download_available": True,
                        "download_url": direct_url,
                        "archive": archive,
                    })
                    self.stats["versions"] += 1
            except Exception as e:
                logger.error(f"Version parse error {rl['url']}: {e}")

        return versions

    def _get_next_page(self, soup, current_url):
        next_link = soup.find("a", rel="next")
        if next_link and next_link.get("href"):
            return urljoin(BASE_URL, next_link["href"])
        for a in soup.find_all("a", href=True):
            if "page/" in a["href"]:
                text = a.get_text(strip=True).lower()
                if text in ["next", "»", ">", "next page"]:
                    return urljoin(BASE_URL, a["href"])
        match = re.search(r"page/(\d+)/", current_url)
        cur = int(match.group(1)) if match else 1
        for a in soup.find_all("a", href=True):
            m = re.search(r"page/(\d+)/", a["href"])
            if m and int(m.group(1)) == cur + 1:
                return urljoin(BASE_URL, a["href"])
        return None

    def scrape(self):
        logger.info("Starting full catalog scrape...")
        current_url = CATALOG_URL
        page_num = self.state.get("last_page", 0) + 1 if self.resume else 1
        existing_ids = [g.get("id", 0) for g in self.existing.get("games", []) if isinstance(g.get("id"), int)]
        next_id = max(existing_ids + [0]) + 1

        pbar = tqdm(desc="Scraping", unit="page")

        while current_url:
            if self.max_pages and page_num > self.max_pages:
                break

            logger.info(f"Page {page_num}: {current_url}")
            resp = self._fetch(current_url)
            if not resp:
                break

            soup = BeautifulSoup(resp.text, "lxml")
            self.stats["pages"] += 1
            game_links = self._parse_listing(soup)
            logger.info(f"Found {len(game_links)} games on page {page_num}")

            for link in game_links:
                title = link["title"]
                if title.lower() in self.seen_titles:
                    continue
                try:
                    detail = self._parse_detail(link["url"])
                    if not detail or not detail.get("title"):
                        detail = {"url": link["url"], "title": title, "description": "",
                                  "genre": "", "year": 0, "cover_url": "", "download_page": ""}

                    title = detail["title"] or title
                    if title.lower() in self.seen_titles:
                        continue

                    # Get versions
                    versions = self._parse_versions(detail.get("download_page", ""))

                    # Download cover
                    cover_path = None
                    if detail.get("cover_url"):
                        cover_path = self._download_cover(detail["cover_url"], next_id)

                    # Map genre
                    genre = detail.get("genre", "") or self._map_genre(title, detail.get("description", ""))

                    entry = {
                        "id": next_id,
                        "title": title,
                        "cover": cover_path or detail.get("cover_url", ""),
                        "description": detail.get("description", "") or f"Nintendo DS game: {title}",
                        "genre": genre,
                        "year": detail.get("year", 0),
                        "versions": versions,
                    }

                    self.games.append(entry)
                    self.seen_titles.add(title.lower())
                    self.stats["games"] += 1
                    next_id += 1
                    logger.info(f"Added: {title} ({len(versions)} versions)")

                except Exception as e:
                    logger.error(f"Error processing {link['url']}: {e}")
                    self.errors.append({"url": link["url"], "error": str(e)})
                    self.stats["errors"] += 1

            self._save_intermediate(next_id)
            self._save_state(page_num, current_url)
            current_url = self._get_next_page(soup, current_url)
            page_num += 1
            pbar.update(1)

        pbar.close()
        self._save_final(next_id)
        self._print_summary()

    def _save_intermediate(self, next_id):
        all_games = list(self.existing.get("games", []))
        existing_titles = {g["title"].lower() for g in all_games}
        for game in self.games:
            if game["title"].lower() not in existing_titles:
                all_games.append(game)
                existing_titles.add(game["title"].lower())

        catalog = {
            "catalog_version": 1,
            "last_update": datetime.now().strftime("%Y-%m-%d"),
            "total_games": len(all_games),
            "games": all_games,
        }
        with open(CATALOG_FILE, "w", encoding="utf-8") as f:
            json.dump(catalog, f, indent=2, ensure_ascii=False)
        logger.info(f"Saved: {len(all_games)} games")

    def _save_final(self, next_id):
        self._save_intermediate(next_id)
        # Copy to backend
        if BACKEND_DIR.exists():
            backend_catalog = BACKEND_DIR / "data" / "catalog.json"
            backend_catalog.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(CATALOG_FILE, backend_catalog)
            # Copy covers
            backend_covers = BACKEND_DIR / "public" / "covers"
            backend_covers.mkdir(parents=True, exist_ok=True)
            for cover in COVERS_DIR.iterdir():
                if cover.is_file():
                    shutil.copy2(cover, backend_covers / cover.name)
            logger.info("Copied to backend")
        if self.errors:
            error_file = Path(__file__).parent / "logs" / "errors.json"
            with open(error_file, "w", encoding="utf-8") as f:
                json.dump(self.errors, f, indent=2)

    def _print_summary(self):
        print("\n" + "=" * 60)
        print("SCRAPING SUMMARY")
        print("=" * 60)
        print(f"Pages scraped:  {self.stats['pages']}")
        print(f"Games found:    {self.stats['games']}")
        print(f"Versions found: {self.stats['versions']}")
        print(f"Covers saved:   {self.stats['covers']}")
        print(f"Errors:         {self.stats['errors']}")
        total = len(self.existing.get("games", [])) + len(self.games)
        print(f"Total in catalog: {total}")
        print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="DSi Game Store Catalog Scraper")
    parser.add_argument("--max-pages", type=int, default=None)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--delay", type=float, default=1.5)
    args = parser.parse_args()

    global REQUEST_DELAY
    REQUEST_DELAY = args.delay

    scraper = CatalogScraper(max_pages=args.max_pages, resume=args.resume)
    try:
        scraper.scrape()
    except KeyboardInterrupt:
        logger.info("Interrupted")
        scraper._save_state(scraper.state.get("last_page", 0), scraper.state.get("last_url", ""))
        scraper._save_intermediate(0)
        print("\nProgress saved. Use --resume to continue.")
    except Exception as e:
        logger.error(f"Fatal: {e}")
        raise


if __name__ == "__main__":
    main()
