//==============================================================================
// page.mjs - eine Seite: 16:9, tiefschwarz, Titel oben mittig, Karten im Raster.
//
// Der Brief: "RECIPES (1/3)", 4 Spalten x 3 Reihen, gleichmaessig ausgerichtet,
// sehr kompakt. Spalten, Reihen und Seitengroesse stehen in style.mjs und sind
// damit konfigurierbar, ohne diese Datei anzufassen.
//==============================================================================

import { PAGE, COLOR, FONT, CARD } from './style.mjs';
import { renderCard, esc, condText } from './card.mjs';
import { familyOf, renderGlyph, FAMILY_NAMES } from './icons.mjs';

// Die Legende. Ohne sie muss der Leser raten, ob ein kleines Gefaess Salz,
// Gewuerz oder Mehl ist - drei Symbole, die sich in 40 Pixeln aehneln. Sie
// steht im Fussraum, der sonst leer bliebe, und zeigt NUR die Familien, die
// auf dieser Seite wirklich vorkommen.
function collectFamilies(recipes) {
  const found = new Set();
  for (const r of recipes) {
    for (const c of r.cells) {
      const f = familyOf(c.imageKey, c.what);
      if (f) found.add(f);
    }
    const rf = familyOf(r.result.cls, r.name);
    if (rf) found.add(rf);
  }
  return [...found]
    .map(f => [f, FAMILY_NAMES[f] || f])
    .sort((a, b) => a[1].localeCompare(b[1]));
}

function renderLegend(fams, x, y, w) {
  if (!fams.length) return '';
  const s = [];
  const size = 15, pad = 6;
  // Breite je Eintrag aus dem laengsten Text, damit die Zeile nicht kollidiert.
  const each = Math.floor(w / fams.length);
  const chars = Math.max(4, Math.floor((each - size - pad - 4) / 4.6));
  s.push('<path d="M ' + x + ' ' + (y - 12) + ' H ' + (x + w) + '" stroke="' + COLOR.borderSoft + '" stroke-width="1"/>');
  fams.forEach(([fam, label], i) => {
    const ex = x + i * each;
    s.push(renderGlyph(fam, ex, y - 2, size, COLOR.glyph));
    const txt = label.length > chars ? label.slice(0, chars - 1) + '…' : label;
    s.push(condText(ex + size + 4, y + 10,
      'font-family="' + FONT.stack + '" font-size="9" fill="' + COLOR.textDim + '"',
      esc(txt.toUpperCase())));
  });
  return s.join('');
}

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
  const legendH = 26;
  const gridH = H - cfg.padTop - cfg.padBottom - legendH;
  const cardW = (gridW - (cols - 1) * cfg.gapX) / cols;
  const cardH = (gridH - (rows - 1) * cfg.gapY) / rows;

  const s = [];
  s.push('<?xml version="1.0" encoding="UTF-8"?>');
  s.push('<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" ' +
         'width="' + W + '" height="' + H + '" viewBox="0 0 ' + W + ' ' + H + '">');
  s.push('<rect width="' + W + '" height="' + H + '" fill="' + COLOR.bg + '"/>');

  // --- Titel oben mittig ---------------------------------------------------
  const title = 'RECIPES (' + pageNo + '/' + pageMax + ')';
  s.push(condText(W / 2, 58,
    'text-anchor="middle" font-family="' + FONT.stack +
    '" font-size="40" fill="' + COLOR.text + '" letter-spacing="6"', esc(title)));
  // Duenne Linie darunter - die einzige Dekoration, die die Seite bekommt.
  s.push('<path d="M ' + cfg.padX + ' 72 H ' + (W - cfg.padX) + '" stroke="' + COLOR.border + '" stroke-width="1"/>');

  // --- Karten --------------------------------------------------------------
  recipes.forEach((r, i) => {
    const col = i % cols, row = Math.floor(i / cols);
    const x = cfg.padX + col * (cardW + cfg.gapX);
    const y = cfg.padTop + row * (cardH + cfg.gapY);
    s.push(renderCard(r, x, y, cardW, cardH, images));
  });

  // --- Legende im Fussraum -------------------------------------------------
  s.push(renderLegend(collectFamilies(recipes), cfg.padX, H - cfg.padBottom + 2, gridW));

  s.push('</svg>');
  return s.join('\n');
}

/** Rezepte in Seiten schneiden. */
export function paginate(recipes, perPage) {
  const pages = [];
  for (let i = 0; i < recipes.length; i += perPage) pages.push(recipes.slice(i, i + perPage));
  return pages.length ? pages : [[]];
}
