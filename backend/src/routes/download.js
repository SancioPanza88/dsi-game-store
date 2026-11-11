const express = require('express');
const router = express.Router();
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const cloudscraper = require('cloudscraper');

const CACHE_DIR = path.join(__dirname, '../../cache');
const TEMP_DIR = path.join(__dirname, '../../temp');

[ CACHE_DIR, TEMP_DIR ].forEach(dir => {
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
});

/**
 * Resolve direct download URL from romsfun.com download page
 * @param {string} downloadPageUrl - e.g. https://romsfun.com/download/game-name-123
 * @param {number} regionIndex - which region/version to download (1-based)
 * @returns {Promise<string>} direct download URL
 */
async function resolveDirectUrl(downloadPageUrl, regionIndex = 1) {
    // Visit the specific region page
    const regionUrl = `${downloadPageUrl}/${regionIndex}`;
    const response = await new Promise((resolve, reject) => {
        cloudscraper.get(regionUrl, (err, resp, body) => {
            if (err) reject(err);
            else resolve({ resp, body });
        });
    });
    
    const body = response.body;
    
    // Find direct download URL (sto.romsfast.com or romsfast.com)
    const match = body.match(/https?:\/\/sto\.romsfast\.com\/[^\s"'<>]+/i) ||
                 body.match(/https?:\/\/romsfast\.com\/[^\s"'<>]+\.zip[^\s"'<>]*/i);
    
    if (!match) {
        throw new Error('Direct download URL not found on page');
    }
    
    // Decode HTML entities (&#038; -> &)
    let url = match[0]
        .replace(/&#038;/g, '&')
        .replace(/&amp;/g, '&');
    
    return url;
}

/**
 * Get list of available regions/versions from download page
 */
async function getAvailableRegions(downloadPageUrl) {
    const response = await new Promise((resolve, reject) => {
        scraper.get(downloadPageUrl, (err, resp, body) => {
            if (err) reject(err);
            else resolve({ resp, body });
        });
    });
    
    const regions = [];
    const seen = new Set();
    const regex = /href="([^"]*\/download\/[^"']+\/(\d+))"[^>]*>([^<]*)/g;
    let match;
    while ((match = regex.exec(response.body)) !== null) {
        const idx = parseInt(match[2]);
        const name = match[3].trim();
        if (!seen.has(idx)) {
            seen.add(idx);
            regions.push({ index: idx, name: name || `Region ${idx}` });
        }
    }
    
    return regions;
}

function downloadFile(url, destPath) {
    return new Promise((resolve, reject) => {
        const client = url.startsWith('https') ? https : http;
        const file = fs.createWriteStream(destPath);
        
        client.get(url, (response) => {
            if (response.statusCode === 301 || response.statusCode === 302) {
                downloadFile(response.headers.location, destPath)
                    .then(resolve).catch(reject);
                return;
            }
            if (response.statusCode !== 200) {
                reject(new Error(`HTTP ${response.statusCode}`));
                return;
            }
            response.pipe(file);
            file.on('finish', () => { file.close(); resolve(destPath); });
        }).on('error', (err) => {
            fs.unlink(destPath, () => {});
            reject(err);
        });
    });
}

function extractNdsFromZip(zipPath, extractDir) {
    try {
        const output = execSync(`unzip -l "${zipPath}"`, { encoding: 'utf8' });
        const ndsFiles = output.split('\n')
            .filter(line => line.toLowerCase().includes('.nds'))
            .map(line => {
                const parts = line.trim().split(/\s+/);
                return parts[parts.length - 1];
            })
            .filter(f => f.endsWith('.nds'));
        
        if (ndsFiles.length === 0) throw new Error('No .nds file found in zip');
        
        const targetFile = ndsFiles[0];
        execSync(`unzip -o "${zipPath}" "${targetFile}" -d "${extractDir}"`, { encoding: 'utf8' });
        return path.join(extractDir, targetFile);
    } catch (err) {
        throw new Error(`ZIP extraction failed: ${err.message}`);
    }
}

// GET /download/:id - Download game (resolves direct URL, downloads, extracts, serves)
router.get('/:id', async (req, res) => {
    const catalog = req.app.locals.catalog;
    const id = parseInt(req.params.id);
    const game = catalog.games.find(g => g.id === id);
    
    if (!game) return res.status(404).json({ error: 'Game not found' });
    if (!game.authorized_source) {
        return res.status(403).json({ 
            error: 'Download not authorized',
            message: 'Only authorized (homebrew/public domain) content can be downloaded.'
        });
    }
    
    // Check cache
    const cacheFile = path.join(CACHE_DIR, `game_${id}.nds`);
    if (fs.existsSync(cacheFile)) {
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Disposition', `attachment; filename="${game.title}.nds"`);
        res.setHeader('Content-Length', fs.statSync(cacheFile).size);
        return fs.createReadStream(cacheFile).pipe(res);
    }
    
    try {
        // Resolve direct URL from download page
        const downloadPageUrl = game.download_url || game.detail_url?.replace('/roms/', '/download/').replace('.html', '');
        
        if (!downloadPageUrl || !downloadPageUrl.includes('/download/')) {
            return res.status(404).json({ error: 'Download page not available' });
        }
        
        const regionIndex = parseInt(req.query.region) || 1;
        const directUrl = await resolveDirectUrl(downloadPageUrl, regionIndex);
        
        // Download the zip
        const zipPath = path.join(TEMP_DIR, `game_${id}.zip`);
        await downloadFile(directUrl, zipPath);
        
        // Extract .nds
        const extractDir = path.join(TEMP_DIR, `extract_${id}`);
        if (!fs.existsSync(extractDir)) fs.mkdirSync(extractDir, { recursive: true });
        
        const ndsPath = extractNdsFromZip(zipPath, extractDir);
        
        // Cache
        fs.copyFileSync(ndsPath, cacheFile);
        
        // Cleanup
        fs.unlinkSync(zipPath);
        fs.rmSync(extractDir, { recursive: true, force: true });
        
        // Serve
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Disposition', `attachment; filename="${game.title}.nds"`);
        res.setHeader('Content-Length', fs.statSync(cacheFile).size);
        fs.createReadStream(cacheFile).pipe(res);
    } catch (err) {
        console.error(`Download error for game ${id}:`, err.message);
        res.status(500).json({ error: 'Download failed', message: err.message });
    }
});

// GET /download/:id/regions - List available regions/versions
router.get('/:id/regions', async (req, res) => {
    const catalog = req.app.locals.catalog;
    const id = parseInt(req.params.id);
    const game = catalog.games.find(g => g.id === id);
    
    if (!game) return res.status(404).json({ error: 'Game not found' });
    
    const downloadPageUrl = game.download_url || game.detail_url?.replace('/roms/', '/download/').replace('.html', '');
    
    if (!downloadPageUrl || !downloadPageUrl.includes('/download/')) {
        return res.json({ regions: [] });
    }
    
    try {
        // Visit download page and find region links
        const response = await new Promise((resolve, reject) => {
            cloudscraper.get(downloadPageUrl, (err, resp, body) => {
                if (err) reject(err);
                else resolve(body);
            });
        });
        
        const regions = [];
        const seen = new Set();
        const regex = /\/download\/[^"']+\/(\d+)/g;
        let match;
        while ((match = regex.exec(response)) !== null) {
            const idx = parseInt(match[1]);
            if (!seen.has(idx)) {
                seen.add(idx);
                regions.push({ index: idx });
            }
        }
        
        res.json({ count: regions.length, regions });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// GET /download/:id/info
router.get('/:id/info', (req, res) => {
    const catalog = req.app.locals.catalog;
    const id = parseInt(req.params.id);
    const game = catalog.games.find(g => g.id === id);
    
    if (!game) return res.status(404).json({ error: 'Game not found' });
    
    res.json({
        id: game.id,
        title: game.title,
        file_size: game.file_size,
        authorized: game.authorized_source,
        download_page: game.download_url || null,
    });
});

module.exports = router;
