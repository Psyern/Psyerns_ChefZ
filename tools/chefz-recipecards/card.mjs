//==============================================================================
// card.mjs - EINE Recipe Card. Die Komponente, die der Brief verlangt.
//
// Aufbau, genau in der Hierarchie des Briefs:
//
//        RECIPE NAME                     (flache Kopfleiste ueber der Karte)
//   [ Zutatenraster ]  -> [Symbol] ->  [ Ergebnis, deutlich groesser ]
//
// Jedes Mass kommt aus style.mjs. Hier steht keine Zahl aus der Luft.
//
// --- Warum das Raster rechnet und nicht zaehlt ----------------------------
// Die Rezepte reichen von EINER Zelle (Brot: nur Teig) bis ACHTZEHN
// (Chernarus Chili als Gruppenportion). Ein festes 3x3 laesst die grossen
// Karten unten aus dem Rahmen laufen - im ersten Entwurf tat es das auch.
// fitGrid() sucht deshalb je Karte die Spaltenzahl, die die groesste noch
// passende Zelle ergibt. Eine Karte kann nicht ueberlaufen, egal wie viele
// Zutaten ein kuenftiges Rezept bekommt.
//
// --- Der Platzhalter ------------------------------------------------------
// Fehlt ein Itembild, wird KEIN stiller Ersatz gezeichnet (Brief). Die Zelle
// bekommt eine rote gestrichelte Kante und ein Kuerzel. Man sieht der Karte
// an, dass ein Bild fehlt, und der Lauf berichtet es zusaetzlich.
//==============================================================================

import { COLOR, CARD, FONT, LABEL } from './style.mjs';
import { familyOf, renderGlyph } from './icons.mjs';

export function esc(s) {
  return String(s == null ? '' : s)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/** Kuerzel fuer eine Zelle ohne Bild: "Root Vegetables" -> "ROOT". */
function token(what) {
  const w = String(what || '?').replace(/[^A-Za-z0-9 ]/g, ' ').trim();
  if (!w) return '?';
  return w.split(/\s+/)[0].slice(0, 5).toUpperCase();
}

function clip(s, maxChars) {
  const t = String(s || '');
  return t.length <= maxChars ? t : t.slice(0, Math.max(1, maxChars - 1)) + '…';
}

// Labels muessen in eine Zelle passen. Ist sie zu schmal, wird gekuerzt -
// lieber "OPT" als drei ueberlappende "OPTIONAL".
const SHORT = { OPTIONAL: 'OPT', EXCEPT: 'EXC' };

/**
 * Sucht Spaltenzahl und Zellgroesse so, dass n Zellen sicher in (maxW x maxH)
 * passen und die Zelle dabei so gross wie moeglich bleibt.
 */
function fitGrid(n, maxW, maxH) {
  const gap = CARD.slotGap, lab = CARD.labelH;
  let best = { cols: 1, cell: 0, rows: n };
  for (let cols = 1; cols <= 8; cols++) {
    const rows = Math.ceil(n / cols);
    const byW = Math.floor((maxW - (cols - 1) * gap) / cols);
    const byH = Math.floor((maxH + gap - rows * (gap + lab)) / rows);
    const cell = Math.min(CARD.slot, byW, byH);
    if (cell > best.cell) best = { cols, cell, rows };
  }
  return best;
}

/** Das Prozesssymbol zwischen Zutaten und Ergebnis. Kantig, kein Webicon. */
function processGlyph(cx, cy, proc) {
  const p = [];
  if (proc.key === 'fire') {
    p.push('<path d="M ' + cx + ' ' + (cy - 14) + ' L ' + (cx + 8) + ' ' + (cy - 2) +
           ' L ' + (cx + 5) + ' ' + (cy - 4) + ' L ' + (cx + 7) + ' ' + (cy + 8) +
           ' L ' + cx + ' ' + (cy + 13) + ' L ' + (cx - 7) + ' ' + (cy + 8) +
           ' L ' + (cx - 5) + ' ' + (cy - 4) + ' L ' + (cx - 8) + ' ' + (cy - 2) +
           ' Z" fill="none" stroke="' + COLOR.accent + '" stroke-width="1.6"/>');
  } else if (proc.key === 'boil') {
    p.push('<path d="M ' + (cx - 9) + ' ' + (cy - 1) + ' h 18 v 10 a 3 3 0 0 1 -3 3 h -12 a 3 3 0 0 1 -3 -3 Z" fill="none" stroke="' + COLOR.accent + '" stroke-width="1.6"/>');
    p.push('<path d="M ' + (cx - 5) + ' ' + (cy - 6) + ' v -5 M ' + cx + ' ' + (cy - 8) +
           ' v -6 M ' + (cx + 5) + ' ' + (cy - 6) + ' v -5" stroke="' + COLOR.accent + '" stroke-width="1.4" fill="none"/>');
  } else {
    p.push('<path d="M ' + (cx - 8) + ' ' + (cy - 8) + ' L ' + (cx + 8) + ' ' + (cy + 8) +
           ' M ' + (cx + 8) + ' ' + (cy - 8) + ' L ' + (cx - 8) + ' ' + (cy + 8) +
           '" stroke="' + COLOR.accent + '" stroke-width="1.6"/>');
  }
  p.push('<path d="M ' + (cx - 11) + ' ' + (cy + 22) + ' h 18 m -5 -4 l 5 4 l -5 4" fill="none" stroke="' + COLOR.arrow + '" stroke-width="1.4"/>');
  return p.join('');
}

/**
 * Zeichnet eine Karte an (x,y) mit (w,h).
 * @param {object} recipe - ein Eintrag aus model.buildModel().recipes
 * @param {object} images - eine ItemImages-Instanz
 */
export function renderCard(recipe, x, y, w, h, images) {
  const s = [];
  const P = 8;

  s.push('<g>');
  s.push('<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h +
         '" fill="' + COLOR.cardBg + '" stroke="' + COLOR.border + '" stroke-width="1"/>');

  // --- 1. Kopfleiste: flach, ueber die ganze Karte -------------------------
  s.push('<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + CARD.headerH +
         '" fill="' + COLOR.headerBg + '" stroke="' + COLOR.border + '" stroke-width="1"/>');
  const title = recipe.variant ? recipe.name.toUpperCase() + ' · ' + recipe.variant
                               : recipe.name.toUpperCase();
  // Lange Titel werden GESTAUCHT, nicht abgeschnitten: ein Name mit "..." am
  // Ende ist fuer den Leser wertlos, ein schmalerer Schriftzug nicht.
  const maxTitleW = w - 16;
  const estW = title.length * CARD.headerFont * 0.55;
  const fit = estW > maxTitleW
    ? ' textLength="' + maxTitleW.toFixed(1) + '" lengthAdjust="spacingAndGlyphs"'
    : ' letter-spacing="1.1"';
  s.push('<text x="' + (x + w / 2) + '" y="' + (y + CARD.headerH - 8) +
         '" text-anchor="middle" font-family="' + FONT.stack + '" font-size="' + CARD.headerFont +
         '" fill="' + COLOR.text + '"' + fit + '>' + esc(title) + '</text>');

  // --- Aufteilung des Rumpfes ----------------------------------------------
  const bodyY = y + CARD.headerH + 6;
  const bodyH = h - CARD.headerH - 12 - 14;          // 14 = Zeile fuer die Fusszeile
  const rs = CARD.resultSize;
  const gridMaxW = w - P * 2 - CARD.arrowW - rs - 12;
  const gridMaxH = bodyH - CARD.labelH;

  const n = Math.max(1, recipe.cells.length);
  const { cols, cell, rows } = fitGrid(n, gridMaxW, gridMaxH);
  const gap = CARD.slotGap;
  const rowH = cell + gap + CARD.labelH;
  const blockH = rows * rowH - gap;
  const gridX = x + P;
  const gridY = bodyY + CARD.labelH + Math.max(0, (bodyH - blockH) / 2);

  // --- 2. Zutatenraster links ---------------------------------------------
  for (let i = 0; i < recipe.cells.length; i++) {
    const c = recipe.cells[i];
    const cx = gridX + (i % cols) * (cell + gap);
    const cy = gridY + Math.floor(i / cols) * rowH;

    s.push('<rect x="' + cx + '" y="' + cy + '" width="' + cell + '" height="' + cell +
           '" fill="' + COLOR.slotBg + '" stroke="' + COLOR.borderSoft + '" stroke-width="1"/>');

    const img = images.resolve(c.imageKey);
    if (img) {
      // Bild deutlich groesser als die Zelle - es darf ueber die Linien ragen.
      const size = cell * CARD.itemScale;
      const off = (size - cell) / 2;
      s.push('<image x="' + (cx - off) + '" y="' + (cy - off) + '" width="' + size +
             '" height="' + size + '" href="' + img.href + '" preserveAspectRatio="xMidYMid meet"/>');
    } else {
      const fam = familyOf(c.imageKey, c.what);
      if (fam) {
        images.noteGlyph(c.imageKey, fam);
        const pad = cell * 0.10;
        s.push(renderGlyph(fam, cx + pad, cy + pad, cell - pad * 2, COLOR.glyph));
      } else {
        images.noteBlank(c.imageKey);
        s.push('<rect x="' + (cx + 3) + '" y="' + (cy + 3) + '" width="' + (cell - 6) +
               '" height="' + (cell - 6) + '" fill="none" stroke="' + COLOR.missing +
               '" stroke-width="1" stroke-dasharray="3 2"/>');
        const fs = Math.max(7, Math.min(11, Math.round(cell / 4)));
        s.push('<text x="' + (cx + cell / 2) + '" y="' + (cy + cell / 2 + fs / 3) +
               '" text-anchor="middle" font-family="' + FONT.stack + '" font-size="' + fs +
               '" fill="' + COLOR.textDim + '">' + esc(token(c.what)) + '</text>');
      }
    }

    // Kleines farbiges Label ueber der Zelle - nie breiter als sie.
    if (c.label) {
      const L = LABEL[c.label] || LABEL.DEFAULT;
      const full = c.label, shortForm = SHORT[c.label] || c.label;
      const useShort = (full.length * 5.4 + 8) > cell;
      const txt = useShort ? shortForm : full;
      const lw = Math.min(cell, Math.max(20, txt.length * 5.4 + 8));
      s.push('<rect x="' + cx + '" y="' + (cy - CARD.labelH + 1) + '" width="' + lw +
             '" height="' + (CARD.labelH - 1) + '" fill="' + L.fill + '" stroke="' + L.stroke + '" stroke-width="0.8"/>');
      s.push('<text x="' + (cx + lw / 2) + '" y="' + (cy - 3) +
             '" text-anchor="middle" font-family="' + FONT.stack + '" font-size="' + CARD.labelFont +
             '" fill="' + L.text + '" letter-spacing="0.3">' + esc(txt) + '</text>');
    }
    if (c.span && cell >= 34) {
      s.push('<text x="' + (cx + cell - 3) + '" y="' + (cy + cell - 4) +
             '" text-anchor="end" font-family="' + FONT.mono + '" font-size="9" fill="' +
             COLOR.accent + '">' + esc(c.span) + '</text>');
    }
  }

  // --- 3. Symbol und Ergebnis rechts, senkrecht mittig ---------------------
  const midY = bodyY + bodyH / 2;
  const arrowX = x + P + gridMaxW + 6;
  s.push(processGlyph(arrowX + CARD.arrowW / 2, midY - 8, recipe.process));

  const resX = arrowX + CARD.arrowW + 6;
  const resY = midY - rs / 2;
  s.push('<rect x="' + resX + '" y="' + resY + '" width="' + rs + '" height="' + rs +
         '" fill="' + COLOR.slotBg + '" stroke="' + COLOR.accent + '" stroke-width="1.4"/>');
  const rimg = images.resolve(recipe.result.cls);
  if (rimg) {
    const size = rs * CARD.itemScale;
    const off = (size - rs) / 2;
    s.push('<image x="' + (resX - off) + '" y="' + (resY - off) + '" width="' + size +
           '" height="' + size + '" href="' + rimg.href + '" preserveAspectRatio="xMidYMid meet"/>');
  } else {
    const rfam = familyOf(recipe.result.cls, recipe.name);
    if (rfam) {
      images.noteGlyph(recipe.result.cls, rfam);
      const pad = rs * 0.10;
      s.push(renderGlyph(rfam, resX + pad, resY + pad, rs - pad * 2, COLOR.glyphResult));
    } else {
      images.noteBlank(recipe.result.cls);
      s.push('<rect x="' + (resX + 4) + '" y="' + (resY + 4) + '" width="' + (rs - 8) +
             '" height="' + (rs - 8) + '" fill="none" stroke="' + COLOR.missing +
             '" stroke-width="1" stroke-dasharray="3 2"/>');
      s.push('<text x="' + (resX + rs / 2) + '" y="' + (resY + rs / 2 + 5) +
             '" text-anchor="middle" font-family="' + FONT.stack + '" font-size="14" fill="' +
             COLOR.textDim + '">' + esc(token(recipe.name)) + '</text>');
    }
  }
  if (recipe.result.quantity) {
    s.push('<text x="' + (resX + rs - 4) + '" y="' + (resY + rs - 5) +
           '" text-anchor="end" font-family="' + FONT.mono + '" font-size="10" fill="' +
           COLOR.accent + '">' + esc(recipe.result.quantity) + '</text>');
  }

  // --- Fusszeile: die Bedingungen, die kein eigenes Symbol tragen ----------
  const foot = [];
  if (recipe.process.label) foot.push(recipe.process.label);
  if (recipe.minTemperature) foot.push(recipe.minTemperature + '°C');
  if (recipe.cookSeconds) foot.push(Math.round(recipe.cookSeconds / 60) + ' MIN');
  if (recipe.container) foot.push(token(recipe.container.replace(/^ChefZ_/, '')));
  if (foot.length) {
    s.push('<text x="' + (x + P) + '" y="' + (y + h - 7) + '" font-family="' + FONT.stack +
           '" font-size="' + CARD.footFont + '" fill="' + COLOR.textDim + '" letter-spacing="0.6">' +
           esc(clip(foot.join('  ·  ').toUpperCase(), 52)) + '</text>');
  }

  s.push('</g>');
  return s.join('');
}
