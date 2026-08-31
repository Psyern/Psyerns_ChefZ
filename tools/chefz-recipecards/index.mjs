//==============================================================================
// index.mjs - der Generator. Ein Befehl, alle Wiki-Grafiken neu.
//
//   node tools/chefz-recipecards/index.mjs
//   node tools/chefz-recipecards/index.mjs --sort id --per-page 9 --cols 3
//   node tools/chefz-recipecards/index.mjs --png
//   node tools/chefz-recipecards/index.mjs --strict      (fehlende Bilder = Fehler)
//
// Ablauf, in dieser Reihenfolge:
//   1. Rezepte laden (ueber den Parser der Validatoren)
//   2. ungueltige Rezeptdaten melden
//   3. auf Seiten verteilen
//   4. SVG je Seite schreiben
//   5. optional PNG rastern (Chrome/Edge headless, wenn vorhanden)
//   6. Wiki-Markdown schreiben
//   7. fehlende Itembilder berichten
//
// --- Exit-Code, und warum er so ist --------------------------------------
//   1  ungueltige Rezeptdaten (eine Karte konnte nicht gebaut werden) - das
//      ist ein Defekt und soll einen Build anhalten.
//   0  fehlende Itembilder. Sie werden LAUT berichtet, sind aber heute der
//      Normalzustand (das Repo fuehrt keine Inventarsymbole), und ein Werkzeug,
//      das immer rot ist, wird nicht gelesen. Wer es hart will: --strict.
//==============================================================================

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';

import { PAGE } from './style.mjs';
import { buildModel } from './model.mjs';
import { renderPage, paginate } from './page.mjs';
import { ItemImages } from './items.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(HERE, '..', '..');

// --- Argumente ---------------------------------------------------------------
function parseArgs(argv) {
  const a = {
    sort: 'name', png: false, strict: false,
    outDir: path.join(REPO, 'ChefZ_Wiki', 'images', 'recipes'),
    wiki: path.join(REPO, 'ChefZ_Wiki', 'Recipe-Cards.md'),
    cfg: { ...PAGE },
  };
  for (let i = 0; i < argv.length; i++) {
    const k = argv[i], v = argv[i + 1];
    switch (k) {
      case '--sort':     a.sort = v; i++; break;
      case '--per-page': a.cfg.perPageOverride = Number(v); i++; break;
      case '--cols':     a.cfg.cols = Number(v); i++; break;
      case '--rows':     a.cfg.rows = Number(v); i++; break;
      case '--width':    a.cfg.width = Number(v); i++; break;
      case '--height':   a.cfg.height = Number(v); i++; break;
      case '--out':      a.outDir = path.resolve(v); i++; break;
      case '--wiki':     a.wiki = path.resolve(v); i++; break;
      case '--png':      a.png = true; break;
      case '--strict':   a.strict = true; break;
      case '--force':    a.force = true; break;
      case '--help': case '-h': a.help = true; break;
      default:
        if (k.startsWith('--')) throw new Error('Unbekannte Option: ' + k);
    }
  }
  return a;
}

const HELP = [
  'chefz-recipecards - Rezept-Infografiken fuer das Wiki',
  '',
  '  --sort <name|id|slots>  Reihenfolge der Rezepte      (Vorgabe: name)',
  '  --cols <n> --rows <n>   Raster je Seite              (Vorgabe: 4x3)',
  '  --per-page <n>          Karten je Seite, ueberschreibt cols*rows',
  '  --width <px> --height <px>  Seitengroesse            (Vorgabe: 1920x1080)',
  '  --out <dir>             Zielverzeichnis der Grafiken',
  '  --wiki <datei>          Wiki-Markdown, das aktualisiert wird',
  '  --png                   zusaetzlich PNG rastern (braucht Chrome oder Edge)',
  '  --strict                fehlende Itembilder als Fehler werten (Exit 1)',
  '  --force                 eine fremde Wiki-Datei trotzdem ueberschreiben',
].join('\n');

// --- PNG: Rasterizer suchen und benutzen -------------------------------------
function findBrowser() {
  const cands = [
    process.env.CHEFZ_CHROME,
    'C:/Program Files/Google/Chrome/Application/chrome.exe',
    'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe',
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
    'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  ].filter(Boolean);
  for (const c of cands) if (fs.existsSync(c)) return c;
  return null;
}

function rasterize(browser, svgPath, pngPath, w, h) {
  // Chrome schreibt den Screenshot ins ARBEITSVERZEICHNIS, wenn der Pfad
  // relativ ist - deshalb absolut, und danach pruefen, dass die Datei da ist.
  execFileSync(browser, [
    '--headless=new', '--disable-gpu', '--hide-scrollbars',
    '--default-background-color=00000000',
    '--force-device-scale-factor=1',
    '--window-size=' + w + ',' + h,
    '--screenshot=' + pngPath,
    'file:///' + svgPath.replace(/\\/g, '/'),
  ], { stdio: 'pipe', timeout: 60000 });
  if (!fs.existsSync(pngPath)) throw new Error('Chrome lief, schrieb aber keine Datei');
}

// --- Wiki-Markdown -----------------------------------------------------------
// Die Markierung in Zeile 1 ist eine SICHERUNG, kein Schmuck. Beim ersten Bauen
// schrieb dieses Werkzeug nach ChefZ_Wiki/Recipes.md - und das ist eine
// bestehende, von zwoelf Seiten verlinkte Seite ueber die Rezept-Engine, die
// dabei ueberschrieben wurde. Wiederhergestellt aus git, Zielname geaendert,
// und seither weigert sich der Generator, eine Datei anzufassen, die diese
// Markierung nicht traegt.
const MARK = '<!-- generated by tools/chefz-recipecards - do not edit by hand -->';

function writeWiki(file, pages, ext, outDir, missingNote, force) {
  if (fs.existsSync(file)) {
    const head = fs.readFileSync(file, 'utf8').slice(0, 200);
    if (!head.includes('tools/chefz-recipecards') && !force) {
      throw new Error(
        path.relative(REPO, file).replace(/\\/g, '/') + ' stammt nicht von diesem Werkzeug ' +
        '(keine Generator-Markierung in Zeile 1). Ich ueberschreibe sie nicht.\n' +
        '  Anderes Ziel waehlen:  --wiki <datei>\n' +
        '  Oder bewusst ueberschreiben:  --force');
    }
  }
  const relDir = path.relative(path.dirname(file), outDir).replace(/\\/g, '/');
  const L = [];
  L.push(MARK);
  L.push('');
  L.push('# Recipe Cards');
  L.push('');
  L.push('Every recipe of the mod as a card: what goes in on the left, what it takes');
  L.push('in the middle, what comes out on the right.');
  L.push('');
  L.push('> Generated by `tools/chefz-recipecards` from the recipe records of the mod.');
  L.push('> **Do not edit this page by hand** -');
  L.push('> the next run overwrites it. Change the recipes, or the style config, and');
  L.push('> run `node tools/chefz-recipecards/index.mjs` again.');
  L.push('');
  if (missingNote) {
    L.push('> **Item images are missing.** ' + missingNote + ' The cards draw a dashed red');
    L.push('> placeholder with a short token instead, so a gap never looks like a picture.');
    L.push('');
  }
  pages.forEach((_, i) => {
    const n = String(i + 1).padStart(2, '0');
    L.push('## Page ' + (i + 1));
    L.push('');
    L.push('![Recipes Page ' + (i + 1) + '](' + relDir + '/recipes-' + n + '.' + ext + ')');
    L.push('');
  });
  L.push('## Next');
  L.push('');
  L.push('- [Recipes](Recipes) - how a recipe is written and how the engine picks one');
  L.push('- [Recipe-Book](Recipe-Book) - the same recipes in prose');
  L.push('- [Recipe-Reference](Recipe-Reference) - every field of every record');
  L.push('- [Production-Chains](Production-Chains) - how the ingredients are made');
  L.push('');
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, L.join('\n'), 'utf8');
}

// --- Hauptlauf ---------------------------------------------------------------
async function main() {
  const a = parseArgs(process.argv.slice(2));
  if (a.help) { console.log(HELP); return 0; }

  const perPage = a.cfg.perPageOverride || (a.cfg.cols * a.cfg.rows);
  if (!Number.isFinite(perPage) || perPage < 1) throw new Error('Karten je Seite muss >= 1 sein');

  const chefzdata = await import('../chefz-validate/chefzdata.mjs');
  const { recipes, problems } = buildModel(chefzdata, { sort: a.sort });

  console.log('chefz-recipecards');
  console.log('─'.repeat(72));
  console.log('Rezepte gelesen: ' + recipes.length + ' (Quelle: Addons/*/Config/Recipes, Parser der Validatoren)');

  if (problems.length) {
    console.log('');
    console.log('UNGUELTIGE REZEPTDATEN: ' + problems.length);
    for (const p of problems) console.log('  ' + p);
  }

  const images = new ItemImages();
  const pages = paginate(recipes, perPage);
  fs.mkdirSync(a.outDir, { recursive: true });

  const written = [];
  pages.forEach((group, i) => {
    const svg = renderPage(group, i + 1, pages.length, images, a.cfg);
    const n = String(i + 1).padStart(2, '0');
    const f = path.join(a.outDir, 'recipes-' + n + '.svg');
    fs.writeFileSync(f, svg, 'utf8');
    written.push(f);
  });
  console.log('Seiten geschrieben: ' + pages.length + ' SVG in ' + path.relative(REPO, a.outDir).replace(/\\/g, '/'));

  let ext = 'svg';
  if (a.png) {
    const browser = findBrowser();
    if (!browser) {
      console.log('');
      console.log('PNG uebersprungen: weder Chrome noch Edge gefunden.');
      console.log('  Pfad selbst setzen: CHEFZ_CHROME=<pfad zur exe>');
    } else {
      let ok = 0;
      for (const svgFile of written) {
        const pngFile = svgFile.replace(/\.svg$/, '.png');
        try { rasterize(browser, svgFile, pngFile, a.cfg.width, a.cfg.height); ok++; }
        catch (e) { console.log('PNG fehlgeschlagen fuer ' + path.basename(svgFile) + ': ' + e.message); }
      }
      console.log('PNG gerastert: ' + ok + '/' + written.length + ' mit ' + path.basename(browser));
      if (ok === written.length) ext = 'png';
    }
  }

  const note = images.hasGaps
    ? images.missing.size + ' of the classes on these cards have no image entry.'
    : null;
  writeWiki(a.wiki, pages, ext, a.outDir, note, a.force);
  console.log('Wiki aktualisiert: ' + path.relative(REPO, a.wiki).replace(/\\/g, '/') + ' (verweist auf .' + ext + ')');

  console.log('');
  console.log(images.report());
  console.log('─'.repeat(72));

  if (problems.length) { console.log('ERGEBNIS: NICHT BESTANDEN - ungueltige Rezeptdaten.'); return 1; }
  if (a.strict && images.hasGaps) { console.log('ERGEBNIS: NICHT BESTANDEN - --strict und fehlende Itembilder.'); return 1; }
  console.log('ERGEBNIS: BESTANDEN' + (images.hasGaps ? ' - mit gemeldeten Bildluecken.' : '.'));
  return 0;
}

main().then(c => process.exit(c)).catch(e => { console.error('FEHLER: ' + e.message); process.exit(2); });
