//==============================================================================
// items.mjs - classname -> Itembild, und die Buchfuehrung ueber das, was fehlt.
//
// Der Brief ist an einer Stelle unmissverstaendlich:
//
//   "Fehlende Itembilder sollen deutlich erkannt und beim Generieren
//    ausgegeben werden. Keinen stillen Platzhalter verwenden, ohne darauf
//    hinzuweisen."
//
// Deshalb zwei Dinge: der Ersatz ist SICHTBAR ein Ersatz (rote Kante, der
// Klassenname als Kuerzel), und jede Verwendung wird gezaehlt und am Ende
// berichtet. Ein Lauf, der Bilder vermisst, sagt das - er sieht nicht aus wie
// ein Lauf, dem nichts fehlt.
//
// STAND 31.08.2026: das Repo enthaelt KEINE Item-PNGs. Die 52 .paa sind
// Modelltexturen, keine Inventarsymbole, und .paa ist kein Format, das ein
// Browser oder ein SVG-Betrachter oeffnet. Deshalb ist item-images.json heute
// weitgehend leer und der Bericht entsprechend lang. Das ist kein Defekt des
// Werkzeugs, sondern sein Zweck: es benennt die Luecke, statt sie zu kaschieren.
//==============================================================================

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));

export class ItemImages {
  constructor(mapPath = path.join(HERE, 'item-images.json')) {
    this.mapPath = mapPath;
    this.map = {};
    this.missing = new Map();   // Schluessel -> Anzahl Verwendungen (kein Eintrag)
    this.broken = new Map();    // Schluessel -> Pfad, der nicht existiert
    this.used = new Set();
    this.glyphs = new Map();    // Schluessel -> Familie, als Vektorsymbol gezeichnet
    this.blank = new Map();     // Schluessel -> Anzahl, weder Bild noch Familie

    if (fs.existsSync(mapPath)) {
      try {
        const raw = JSON.parse(fs.readFileSync(mapPath, 'utf8'));
        // Kommentarschluessel (_comment) werden ignoriert.
        for (const [k, v] of Object.entries(raw)) {
          if (k.startsWith('_')) continue;
          this.map[k] = v;
        }
      } catch (e) {
        throw new Error(`item-images.json ist kein gueltiges JSON: ${e.message}`);
      }
    }
  }

  /**
   * Liefert { href } wenn ein Bild existiert, sonst null - und merkt sich den
   * Fall. Der Aufrufer zeichnet dann den sichtbaren Ersatz.
   */
  resolve(className) {
    if (!className) return null;
    const rel = this.map[className];
    if (!rel) {
      this.missing.set(className, (this.missing.get(className) || 0) + 1);
      return null;
    }
    const abs = path.isAbsolute(rel) ? rel : path.join(HERE, rel);
    if (!fs.existsSync(abs)) {
      this.broken.set(className, rel);
      return null;
    }
    this.used.add(className);
    // Als data: URI einbetten - eine SVG mit externen Bildpfaden zerfaellt,
    // sobald sie irgendwohin kopiert wird, und GitHub laedt sie ohnehin nicht.
    const ext = path.extname(abs).toLowerCase();
    const mime = ext === '.png' ? 'image/png'
               : ext === '.jpg' || ext === '.jpeg' ? 'image/jpeg'
               : ext === '.webp' ? 'image/webp'
               : ext === '.svg' ? 'image/svg+xml'
               : null;
    if (!mime) {
      this.broken.set(className, `${rel} (Format ${ext} wird nicht eingebettet)`);
      return null;
    }
    const b64 = fs.readFileSync(abs).toString('base64');
    return { href: `data:${mime};base64,${b64}` };
  }

  /** Eine Zelle wurde als Vektorsymbol gezeichnet - Ersatz, kein Itemfoto. */
  noteGlyph(key, family) { if (key) this.glyphs.set(key, family); }

  /** Weder Bild noch passende Familie - der rote Kasten. */
  noteBlank(key) { if (key) this.blank.set(key, (this.blank.get(key) || 0) + 1); }

  /** Der Bericht. Wird IMMER gedruckt, auch wenn nichts fehlt. */
  report() {
    const lines = [];
    const miss = [...this.missing.entries()].sort((a, b) => b[1] - a[1]);
    const brk = [...this.broken.entries()].sort();

    lines.push(`Itembilder: ${this.used.size} echte Bilder, ` +
               `${this.glyphs.size} als Vektorsymbol gezeichnet, ` +
               `${this.blank.size} ganz ohne Darstellung, ${brk.length} mit totem Pfad.`);

    if (brk.length) {
      lines.push('');
      lines.push('PFAD ZEIGT INS LEERE (Eintrag da, Datei nicht):');
      for (const [cls, p] of brk) lines.push(`  ${cls.padEnd(34)} ${p}`);
    }
    if (this.blank.size) {
      lines.push('');
      lines.push('WEDER BILD NOCH SYMBOL (roter Kasten - hier fehlt am meisten):');
      for (const [cls, n] of [...this.blank.entries()].sort((a, b) => b[1] - a[1])) {
        lines.push(`  ${cls.padEnd(34)} ${n}x verwendet`);
      }
    }
    if (miss.length) {
      lines.push('');
      lines.push(`KEIN EINTRAG in ${path.basename(this.mapPath)} - ein Vektorsymbol steht ` +
                 `stellvertretend, es ist KEIN Itemfoto:`);
      for (const [cls, n] of miss) {
        const fam = this.glyphs.get(cls);
        lines.push(`  ${cls.padEnd(34)} ${String(n + 'x').padEnd(6)} ${fam ? 'Symbol: ' + fam : '(kein Symbol)'}`);
      }
    }
    return lines.join('\n');
  }

  get hasGaps() { return this.missing.size > 0 || this.broken.size > 0; }
}
