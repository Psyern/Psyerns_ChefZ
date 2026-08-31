//==============================================================================
// style.mjs - die EINE Stelle, an der das Aussehen steht.
//
// Der Brief verlangt "zentrale Style-Konfiguration" und ein spaeter leicht
// aenderbares Card-Layout. Deshalb steht hier jede Farbe, jede Kante und jedes
// Mass - im Renderer selbst gibt es keine Zahl, die nicht von hier kommt.
//
// Die Farben sind bewusst entsaettigt: DayZ-Inventar ist schwarz, das Gruen ist
// Militaergruen und kein Web-Gruen. Wer die Optik aendern will, aendert HIER.
//==============================================================================

export const PAGE = {
  // 16:9 Querformat. 1920x1080 rendert als PNG 1:1 in Wiki-Breite.
  width: 1920,
  height: 1080,
  cols: 4,
  rows: 3,
  get perPage() { return this.cols * this.rows; },
  padX: 48,
  padTop: 96,      // Platz fuer den Titel
  padBottom: 40,
  gapX: 14,        // "ohne grosse Abstaende nebeneinander" - eng halten
  gapY: 14,
};

export const COLOR = {
  bg:         '#000000',
  cardBg:     '#0a0c0a',
  headerBg:   '#1c1f1c',
  border:     '#4a5a3a',   // duenner olivgruener Rahmen
  borderSoft: '#243024',   // Gridlinien, dunkler
  text:       '#ffffff',
  textDim:    '#8a9480',
  accent:     '#6b7f4a',   // entsaettigtes Militaergruen
  slotBg:     '#050705',
  arrow:      '#6b7f4a',
  missing:    '#7a2f2f',   // fehlendes Bild: sichtbar, nie still
};

// Kleine farbige Labels ueber Zutaten. Der Brief nennt hellblau/beige/gruen.
export const LABEL = {
  ANY:      { fill: '#2e4a2e', stroke: '#6b8f4a', text: '#cfe0b0' },
  OR:       { fill: '#4a3f1e', stroke: '#8f7a3a', text: '#e6d9a8' },
  OPTIONAL: { fill: '#22303c', stroke: '#4a7090', text: '#bcd6e8' },
  ALL:      { fill: '#3a2a3a', stroke: '#7a5a8a', text: '#dcc6e6' },
  EXCEPT:   { fill: '#3c2626', stroke: '#8a5050', text: '#e8c6c6' },
  DEFAULT:  { fill: '#222622', stroke: '#4a5a3a', text: '#c8d2bc' },
};

export const CARD = {
  headerH: 26,
  headerFont: 15,
  slot: 46,        // Kantenlaenge einer Inventarzelle
  slotGap: 3,
  gridCols: 3,     // maximal, wird je Rezept dynamisch kleiner
  gridRows: 3,
  itemScale: 1.32, // "Itembilder deutlich groesser als die Gridlinien"
  resultSize: 88,  // Ergebnis deutlich groesser als eine Zutat
  arrowW: 40,
  labelH: 13,
  labelFont: 9,
  footFont: 10,
};

// Schmale, technische Schrift. Websichere Kette - der Renderer bettet keine
// Fonts ein, damit die SVG in jedem Betrachter gleich bleibt.
export const FONT = {
  stack: "'DIN Condensed','Oswald','Roboto Condensed','Arial Narrow',Impact,sans-serif",
  mono:  "'Consolas','DejaVu Sans Mono',monospace",
};
