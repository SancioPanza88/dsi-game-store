/*
 * Seed script for DSi Game Store catalog
 * Adds sample authorized homebrew games to catalog.json
 */

const fs = require('fs');
const path = require('path');

const catalogPath = path.join(__dirname, '../data/catalog.json');

const seedData = {
    version: '1.0.0',
    last_updated: new Date().toISOString().split('T')[0],
    description: 'DSi Game Store Catalog - Authorized content only',
    games: [
        {
            id: 1,
            title: 'DSi Homebrew Demo',
            description: 'A demonstration homebrew application showcasing DSi capabilities. Open source and freely distributable.',
            genre: 8,
            genre_name: 'Homebrew',
            year: 2024,
            file_size: 524288,
            download_url: 'https://github.com/example/dsi-demo/releases/download/v1.0/demo.nds',
            cover_url: '/covers/dsi-demo.png',
            source_name: 'Homebrew Community',
            compatibility: 'DSi / DSi XL',
            authorized_source: true,
            license: 'MIT',
            added_date: Math.floor(Date.now() / 1000)
        }
    ]
};

fs.writeFileSync(catalogPath, JSON.stringify(seedData, null, 2));
console.log(`Catalog seeded with ${seedData.games.length} games`);
console.log(`File: ${catalogPath}`);
