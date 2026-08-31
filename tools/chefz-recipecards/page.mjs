//==============================================================================
// page.mjs - eine Seite: 16:9, tiefschwarz, Titel oben mittig, Karten im Raster.
//
// Der Brief: "RECIPES (1/3)", 4 Spalten x 3 Reihen, gleichmaessig ausgerichtet,
// sehr kompakt. Spalten, Reihen und Seitengroesse stehen in style.mjs und sind
// damit konfigurierbar, ohne diese Datei anzufassen.
//==============================================================================

import { PAGE, COLOR, FONT } from './style.mjs';
import { renderCard, esc } from './card.mjs';

/**
 * @param {Array} recipes - die Rezepte GENAU dieser Seite (hoechstens perPage)
 * @param {number} pageNo  - 1-basiert
 * @param {number} pageMax
 * @param {object} images  - ItemImages
 * @param {object} cfg     - optionaler Ersatz fuer PAGE (Seitengroesse, Raster)
 */
export function renderPage(recipes, pageNo, pageMax, images, cfg = PAGE) {
  const W = cfg.width, H = cfg.height;
  const cols = cfg.cols, rows = cfg.rows;

  const gridW = W - cfg.padX * 2;
  const gridH = H - cfg.padTop - cfg.padBottom;
  const cardW = (gridW - (cols - 1) * cfg.gapX) / cols;
  const cardH = (gridH - (rows - 1) * cfg.gapY) / rows;

  const s = [];
  s.push('<?xml version="1.0" encoding="UTF-8"?>');
  s.push('<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" ' +
         'width="' + W + '" height="' + H + '" viewBox="0 0 ' + W + ' ' + H + '">');
  s.push('<rect width="' + W + '" height="' + H + '" fill="' + COLOR.bg + '"/>');

  // --- Titel oben mittig ---------------------------------------------------
  const title = 'RECIPES (' + pageNo + '/' + pageMax + ')';
  s.push('<text x="' + (W / 2) + '" y="58" text-anchor="middle" font-family="' + FONT.stack +
         '" font-size="40" fill="' + COLOR.text + '" letter-spacing="6">' + esc(title) + '</text>');
  // Duenne Linie darunter - die einzige Dekoration, die die Seite bekommt.
  s.push('<path d="M ' + cfg.padX + ' 72 H ' + (W - cfg.padX) + '" stroke="' + COLOR.border + '" stroke-width="1"/>');

  // --- Karten --------------------------------------------------------------
  recipes.forEach((r, i) => {
    const col = i % cols, row = Math.floor(i / cols);
    const x = cfg.padX + col * (cardW + cfg.gapX);
    const y = cfg.padTop + row * (cardH + cfg.gapY);
    s.push(renderCard(r, x, y, cardW, cardH, images));
  });

  s.push('</svg>');
  return s.join('\n');
}

/** Rezepte in Seiten schneiden. */
export function paginate(recipes, perPage) {
  const pages = [];
  for (let i = 0; i < recipes.length; i += perPage) pages.push(recipes.slice(i, i + perPage));
  return pages.length ? pages : [[]];
}
