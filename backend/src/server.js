/*
 * DSi Game Store - Backend Server
 * Provides catalog API, metadata, and authorized download endpoints
 * 
 * Runs on HTTP (DSi compatible). For production, use a reverse proxy for HTTPS.
 */

const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const compression = require('compression');
const morgan = require('morgan');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 3000;

/* Middleware */
app.use(helmet({
    contentSecurityPolicy: false, /* Allow for DSi compatibility */
}));
app.use(cors());
app.use(compression());
app.use(morgan('combined'));
app.use(express.json());

/* Serve static files (covers, icons) */
app.use('/covers', express.static(path.join(__dirname, '../public/covers')));
app.use('/icons', express.static(path.join(__dirname, '../public/icons')));

/* Load catalog data */
let catalog = { version: '1.0.0', games: [] };
const catalogPath = path.join(__dirname, '../data/catalog.json');

function loadCatalog() {
    try {
        const data = fs.readFileSync(catalogPath, 'utf8');
        catalog = JSON.parse(data);
        console.log(`Catalog loaded: ${catalog.games.length} games`);
    } catch (err) {
        console.error('Failed to load catalog:', err.message);
        catalog = { version: '1.0.0', games: [] };
    }
}

loadCatalog();

/* Watch for catalog changes */
fs.watchFile(catalogPath, () => {
    console.log('Catalog file changed, reloading...');
    loadCatalog();
});

/* Pass catalog to app locals for routes */
app.locals.catalog = catalog;

/* Reload catalog function accessible to routes */
app.reloadCatalog = loadCatalog;

/* ===== API ROUTES ===== */

/* GET /api/catalog - Full catalog */
app.get('/api/catalog', (req, res) => {
    res.json(catalog);
});

/* GET /api/catalog/version - Catalog version */
app.get('/api/catalog/version', (req, res) => {
    res.json({ version: catalog.catalog_version || catalog.version || '1.0.0', count: catalog.games.length });
});

/* GET /api/game/:id - Single game details */
app.get('/api/game/:id', (req, res) => {
    const id = parseInt(req.params.id);
    const game = catalog.games.find(g => g.id === id);
    
    if (!game) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    res.json(game);
});

/* GET /api/categories - List categories with counts */
app.get('/api/categories', (req, res) => {
    const categories = {};
    catalog.games.forEach(game => {
        const genre = game.genre_name || 'Other';
        if (!categories[genre]) {
            categories[genre] = { name: genre, count: 0, genre_id: game.genre };
        }
        categories[genre].count++;
    });
    
    res.json(Object.values(categories));
});

/* GET /api/search - Search games */
app.get('/api/search', (req, res) => {
    const query = (req.query.q || '').toLowerCase();
    const genre = req.query.genre;
    
    let results = catalog.games;
    
    if (query) {
        results = results.filter(g => 
            g.title.toLowerCase().includes(query) ||
            g.description.toLowerCase().includes(query)
        );
    }
    
    if (genre !== undefined) {
        const genreId = parseInt(genre);
        results = results.filter(g => g.genre === genreId);
    }
    
    res.json({ query, count: results.length, games: results });
});

/* GET /api/recent - Recently added games */
app.get('/api/recent', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const recent = catalog.games
        .sort((a, b) => (b.added_date || 0) - (a.added_date || 0))
        .slice(0, limit);
    
    res.json({ count: recent.length, games: recent });
});

/* GET /download/:id - Download game file */
app.get('/download/:id', (req, res) => {
    const id = parseInt(req.params.id);
    const game = catalog.games.find(g => g.id === id);
    
    if (!game) {
        return res.status(404).json({ error: 'Game not found' });
    }
    
    /* Check if download source is authorized */
    if (!game.authorized_source) {
        return res.status(403).json({ error: 'Download not authorized' });
    }
    
    /* In production, this would stream from authorized source */
    /* For now, redirect to source URL */
    if (game.download_url) {
        return res.redirect(game.download_url);
    }
    
    res.status(404).json({ error: 'Download URL not available' });
});

/* Download routes */
const downloadRouter = require('./routes/download');
app.use('/download', downloadRouter);

/* Health check */
app.get('/api/health', (req, res) => {
    res.json({ status: 'ok', version: '1.0.0', games: catalog.games.length });
});

/* ===== ERROR HANDLING ===== */
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({ error: 'Internal server error' });
});

/* ===== START SERVER ===== */
app.listen(PORT, '0.0.0.0', () => {
    console.log(`DSi Game Store Backend running on port ${PORT}`);
    console.log(`Catalog: ${catalog.games.length} games`);
    console.log(`API: http://localhost:${PORT}/api/catalog`);
});

module.exports = app;
