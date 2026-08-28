//==============================================================================
// ChefZ_ItemFacts / ChefZ_FactSnapshot - die Waehrung des Core
//
// Entwurf: 05 §3.3 (Feldliste woertlich), 05 §4 (Datenfluss), 05 E1 (KEIN
// EntityAI), 05 E4 (effektive Tags werden im Collector zusammengefuehrt),
// 07 §5 (die Tests, die auf diesen Feldern laufen).
//
// ---------------------------------------------------------------------------
// Der wichtigste Architekturstrich des Entwurfs (05 §4)
// ---------------------------------------------------------------------------
// Links vom Snapshot liegt die Spielwelt, rechts davon reine Datenverarbeitung.
// ChefZ_ItemFacts traegt deshalb ausdruecklich KEINE EntityAI-Referenz, sondern
// nur einen Handle-Index in die parallele Entity-Liste des Collectors
// (05 E1).
//
// Das kostet eine Indirektion beim Anwenden und gewinnt drei Dinge:
//   - der Matcher bleibt in 1_Core und ist ohne laufendes Spiel pruefbar,
//   - er KANN am Item nichts veraendern, weil er keinen Zeiger dorthin hat,
//   - im mehrstufigen Backtracking ist ein Zugriff auf ein bereits geloeschtes
//     ItemBase strukturell ausgeschlossen.
//
// ---------------------------------------------------------------------------
// Warum die Felder vorberechnet sind
// ---------------------------------------------------------------------------
// Der Snapshot wird EINMAL je Auswertung gebaut und danach nur gelesen. Bei
// 30 Kandidatenrezepten x 8 Slots x 8 Items waeren es sonst bis zu 5.760
// Managerzugriffe je Tick statt 8 Zusammenfuehrungen (05 E4).
//
// KEIN CONTENT. KEIN ENGINE-TYP: "vanillaFoodStage" ist bewusst ein int und
// kein FoodStageType - der Enum lebt in 4_World, und 1_Core darf ihn nicht
// kennen (00 §4).
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_ItemFacts
{
    //! Index in die parallele Entity-Liste des Collectors. -1 = ohne Entity
    //! (Selbsttest, Vorschau). Die Zuordnung Handle -> ItemBase existiert
    //! ausschliesslich im ChefZ_FactCollector (05 §3.4).
    int       handle;

    ChefZ_Sym classSym;

    //! Nie null. Bei unbekannter Klasse leer - dann matcht kein
    //! Kategorie-Selektor, und genau das ist gewollt (05 §7).
    ref ChefZ_CategoryClosure closure;

    //! EFFEKTIVE Tags: Klasse + Zustand + Qualitaet, bereits zusammengefuehrt
    //! (05 E4). Der Matcher testet mit einem Find() auf einer kurzen Liste.
    ref array<ChefZ_Sym> tags;

    ChefZ_Sym chefzState;           // INVALID, wenn kein Zustand ermittelbar
    ChefZ_Sym chefzQuality;         // INVALID bei Nicht-ChefZ-Items
    float     freshness01;          // 1.0 bei Nicht-ChefZ-Items
    int       vanillaFoodStage;     // FoodStageType als int, KEIN Enum in 1_Core
    int       portions;             // 0 = kein Portionsgericht
    float     quantity;
    float     quantityMax;
    float     units;                // quantity in Rezepteinheiten (05 §6)
    ChefZ_Sym quantityUnit;
    float     health01;
    float     temperature;
    float     wetness;
    float     cleanness01;          // Vorbereitung fuer Hygiene, V2
    bool      isFrozen;
    bool      isLiquidContainer;
    ChefZ_Sym liquidTypeSym;
    bool      isEdible;
    bool      isChefZManaged;

    //! Arbeitsfeld des Matchers, -1 = frei. Es steht hier und nicht in einer
    //! Nebenstruktur, weil das Backtracking es je Knoten setzt und
    //! zuruecknimmt - eine zweite Tabelle waere eine zweite Wahrheit.
    int       slotBoundTo;

    void ChefZ_ItemFacts()
    {
        closure = new ChefZ_CategoryClosure();
        tags    = new array<ChefZ_Sym>();
        Reset();
    }

    /**
     * Setzt den Datensatz auf "leeres, unbekanntes Item" zurueck.
     *
     * Closure und Tag-Liste werden GELEERT, nicht neu angelegt: der Snapshot
     * ist ein wiederverwendeter Puffer (05 §5), und eine Neuallokation je Item
     * je Auswertung waere genau die Sorte Kosten, die dieser Entwurf vermeiden
     * will.
     */
    void Reset()
    {
        handle            = -1;
        classSym          = ChefZ_SymbolTable.INVALID;
        closure.Clear();
        tags.Clear();
        chefzState        = ChefZ_SymbolTable.INVALID;
        chefzQuality      = ChefZ_SymbolTable.INVALID;
        freshness01       = 1.0;
        vanillaFoodStage  = 0;          // FoodStageType.NONE
        portions          = 0;
        quantity          = 0.0;
        quantityMax       = 0.0;
        units             = 0.0;
        quantityUnit      = ChefZ_SymbolTable.INVALID;
        health01          = 1.0;
        temperature       = 0.0;
        wetness           = 0.0;
        cleanness01       = 1.0;
        isFrozen          = false;
        isLiquidContainer = false;
        liquidTypeSym     = ChefZ_SymbolTable.INVALID;
        isEdible          = false;
        isChefZManaged    = false;
        slotBoundTo       = -1;
    }

    bool HasTag(ChefZ_Sym tag)
    {
        return tags.Find(tag) >= 0;
    }

    //! Tag aufnehmen, ohne Duplikate. Duplikate waeren harmlos fuer das
    //! Ergebnis, aber sie verlaengern die Liste, die im innersten Loop
    //! durchsucht wird.
    void AddTag(ChefZ_Sym tag)
    {
        if (!ChefZ_SymbolTable.IsValid(tag))
            return;
        if (tags.Find(tag) >= 0)
            return;
        tags.Insert(tag);
    }

    string ToLine()
    {
        string s = "#" + handle.ToString() + " " + ChefZ_SymbolTable.NameOrMark(classSym);

        if (ChefZ_SymbolTable.IsValid(chefzState))
            s = s + " zustand=" + ChefZ_SymbolTable.Name(chefzState);
        if (ChefZ_SymbolTable.IsValid(chefzQuality))
            s = s + " qualitaet=" + ChefZ_SymbolTable.Name(chefzQuality);

        s = s + " menge=" + quantity.ToString() + "/" + quantityMax.ToString() + " einheiten=" + units.ToString();
        if (ChefZ_SymbolTable.IsValid(quantityUnit))
            s = s + " " + ChefZ_SymbolTable.Name(quantityUnit);

        s = s + " stage=" + vanillaFoodStage.ToString() + " zustandGesund=" + health01.ToString() + " temp=" + temperature.ToString();

        if (isFrozen)
            s = s + " gefroren";
        if (portions > 0)
            s = s + " portionen=" + portions.ToString();
        if (isLiquidContainer)
            s = s + " fluessig=" + ChefZ_SymbolTable.NameOrMark(liquidTypeSym);
        if (!isChefZManaged)
            s = s + " (nicht deklariert)";

        s = s + " closure=" + closure.ToDebugString();

        s = s + " tags=[";
        for (int i = 0; i < tags.Count(); i++)
        {
            if (i > 0)
                s = s + ",";
            s = s + ChefZ_SymbolTable.Name(tags.Get(i));
        }
        s = s + "]";

        return s;
    }
}

//==============================================================================

/**
 * Die Faktenliste einer Auswertung.
 *
 * Zwei Dinge, die man leicht falsch macht und die hier bewusst geloest sind:
 *
 * 1. WIEDERVERWENDUNG. Clear() gibt die Datensaetze nicht frei, sondern legt
 *    sie in einen Vorrat zurueck. Ein Kessel mit acht Zutaten, der im Sekunden-
 *    takt ausgewertet wird, allokiert nach dem ersten Durchlauf nichts mehr.
 *
 * 2. SORTIERUNG UND HANDLE. SortStable() ordnet ausschliesslich die Sicht
 *    "items" um. Der Handle bleibt im Datensatz und zeigt weiter auf dieselbe
 *    Entity - deshalb darf sortiert werden, ohne die parallele Entity-Liste
 *    anzufassen. Das ist der ganze Zweck des Handles.
 */
class ChefZ_FactSnapshot
{
    //! Die aktive Sicht. Reihenfolge nach SortStable() (05 §3.3).
    ref array<ref ChefZ_ItemFacts> items;

    //! Vorrat aller je erzeugten Datensaetze. Haelt sie am Leben, waehrend
    //! "items" geleert wird.
    private ref array<ref ChefZ_ItemFacts> m_Pool;
    private int m_Used;

    void ChefZ_FactSnapshot()
    {
        items  = new array<ref ChefZ_ItemFacts>();
        m_Pool = new array<ref ChefZ_ItemFacts>();
        m_Used = 0;
    }

    /**
     * Naechsten freien Datensatz holen - zurueckgesetzt und bereits in "items"
     * eingehaengt.
     *
     * Der Rueckgabewert gehoert weiterhin dem Snapshot. Wer ihn ueber den
     * naechsten Clear() hinaus festhaelt, liest danach fremde Daten.
     */
    ChefZ_ItemFacts Acquire()
    {
        ChefZ_ItemFacts facts;

        if (m_Used < m_Pool.Count())
        {
            facts = m_Pool.Get(m_Used);
            facts.Reset();
        }
        else
        {
            facts = new ChefZ_ItemFacts();
            m_Pool.Insert(facts);
        }

        m_Used++;
        items.Insert(facts);
        return facts;
    }

    /**
     * Nimmt den zuletzt geholten Datensatz zurueck.
     *
     * Fuer den einen Fall, in dem der Sammler erst NACH dem Holen merkt, dass
     * ein Item doch nicht in die Liste gehoert. Ohne diesen Weg bliebe ein
     * halb gefuellter Datensatz in der Sicht stehen - und die parallele
     * Entity-Liste waere um eins verschoben, was jeden Handle danach auf das
     * falsche Item zeigen liesse.
     *
     * Der Vorratszeiger m_Used wird bewusst NICHT zurueckgedreht. Waere er es,
     * und haette jemand zwischendurch sortiert, gaebe der naechste Acquire()
     * einen Datensatz heraus, der noch in "items" haengt - zwei Namen fuer
     * dasselbe Objekt, und beide werden beschrieben. Der Preis ist ein
     * ungenutzter Platz im Vorrat bis zum naechsten Clear(); der ist
     * belanglos, der Fehler waere es nicht.
     */
    void DiscardLast()
    {
        if (items.Count() == 0)
            return;
        items.Remove(items.Count() - 1);
    }

    void Clear()
    {
        items.Clear();
        m_Used = 0;
    }

    int Count()
    {
        return items.Count();
    }

    ChefZ_ItemFacts Get(int index)
    {
        if (index < 0 || index >= items.Count())
            return null;
        return items.Get(index);
    }

    //! Datensatz zu einem Handle. Linear, weil die Liste kurz ist und nach
    //! SortStable() nicht mehr nach Handle geordnet sein muss.
    ChefZ_ItemFacts FindByHandle(int handle)
    {
        for (int i = 0; i < items.Count(); i++)
        {
            ChefZ_ItemFacts f = items.Get(i);
            if (f.handle == handle)
                return f;
        }
        return null;
    }

    //! Matcher-Arbeitsfelder zuruecksetzen, ohne die Fakten neu zu erheben.
    void ClearBindings()
    {
        for (int i = 0; i < items.Count(); i++)
            items.Get(i).slotBoundTo = -1;
    }

    /**
     * Stabile Ordnung: classSym, chefzState, Menge ABSTEIGEND, Handle
     * (05 §3.3).
     *
     * Zweck ist Bestimmtheit, nicht Geschwindigkeit: der Verbrauchsplan zieht
     * anteilig in genau dieser Reihenfolge ab (07 E3). Zwei Server mit
     * gleichem Topfinhalt sollen dieselben Instanzen verbrauchen.
     *
     * Verfahren ist Einfuegesortierung: sie ist stabil, kommt ohne Allokation
     * aus und ist bei den hier ueblichen 1..12 Eintraegen schneller als jedes
     * Teile-und-herrsche-Verfahren.
     *
     * WICHTIG, und der Grund, warum der Vorrat m_Pool nicht wegoptimiert
     * werden darf: waehrend des Umschiebens ist "key" eine gewoehnliche
     * lokale Variable, also ein schwacher Zeiger. Der Moment, in dem
     * items.Set() den bisherigen Platz von key ueberschreibt, waere ohne eine
     * zweite starke Referenz der Moment, in dem das Objekt freigegeben wird -
     * und key zeigte ins Leere. m_Pool haelt jeden Datensatz und macht den
     * Umbau dadurch sicher.
     *
     * Anmerkung zur Bestimmtheit: classSym ist ein Laufzeitsymbol und damit
     * NICHT ueber Serverstarts stabil (03 E2). Innerhalb einer Auswertung und
     * innerhalb eines Laufs ist die Ordnung eindeutig; ueber Laufgrenzen
     * hinweg koennen zwei Items DERSELBEN Klasse die Plaetze tauschen - und
     * die sind ununterscheidbar, sonst waeren sie nicht dieselbe Klasse.
     */
    void SortStable()
    {
        for (int i = 1; i < items.Count(); i++)
        {
            ChefZ_ItemFacts key = items.Get(i);
            int j = i - 1;
            while (j >= 0 && Precedes(key, items.Get(j)))
            {
                items.Set(j + 1, items.Get(j));
                j--;
            }
            items.Set(j + 1, key);
        }
    }

    //! true, wenn a vor b gehoert.
    private static bool Precedes(notnull ChefZ_ItemFacts a, notnull ChefZ_ItemFacts b)
    {
        if (a.classSym != b.classSym)
            return a.classSym < b.classSym;
        if (a.chefzState != b.chefzState)
            return a.chefzState < b.chefzState;
        if (a.quantity != b.quantity)
            return a.quantity > b.quantity;         // absteigend
        return a.handle < b.handle;
    }

    void DebugDump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Faktenliste: " + items.Count().ToString() + " Eintraege");
        for (int i = 0; i < items.Count(); i++)
            outLines.Insert("  " + items.Get(i).ToLine());
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S4).
    static bool SelfCheck()
    {
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        if (snap.Count() != 0)                          return false;
        if (snap.Get(0))                                return false;

        // Reihenfolge der Internierung ist hier bedeutungstragend: das zuerst
        // internierte Symbol bekommt die kleinere Zahl, und genau danach
        // sortiert SortStable(). Wer die beiden Zeilen tauscht, dreht die
        // erwartete Reihenfolge um.
        ChefZ_Sym symA = ChefZ_SymbolTable.Intern("CHEFZ_ST_F_A");
        ChefZ_Sym symB = ChefZ_SymbolTable.Intern("CHEFZ_ST_F_B");

        // Absichtlich unsortiert eingefuegt.
        ChefZ_ItemFacts f0 = snap.Acquire();
        f0.handle = 0; f0.classSym = symB; f0.quantity = 5.0;
        ChefZ_ItemFacts f1 = snap.Acquire();
        f1.handle = 1; f1.classSym = symA; f1.quantity = 1.0;
        ChefZ_ItemFacts f2 = snap.Acquire();
        f2.handle = 2; f2.classSym = symA; f2.quantity = 9.0;
        ChefZ_ItemFacts f3 = snap.Acquire();
        f3.handle = 3; f3.classSym = symA; f3.quantity = 9.0;

        if (snap.Count() != 4)                          return false;
        if (snap.FindByHandle(2) != f2)                 return false;
        if (snap.FindByHandle(99))                      return false;

        snap.SortStable();

        // symA vor symB (kleineres Symbol), innerhalb symA Menge absteigend,
        // bei gleicher Menge der kleinere Handle zuerst.
        if (snap.Get(0) != f2)                          return false;
        if (snap.Get(1) != f3)                          return false;
        if (snap.Get(2) != f1)                          return false;
        if (snap.Get(3) != f0)                          return false;

        // Zweimal sortieren aendert nichts.
        snap.SortStable();
        if (snap.Get(0) != f2)                          return false;
        if (snap.Get(3) != f0)                          return false;

        // Bindungen zuruecksetzen
        f0.slotBoundTo = 3;
        snap.ClearBindings();
        if (f0.slotBoundTo != -1)                       return false;

        // Wiederverwendung: nach Clear liefert Acquire dieselben Objekte,
        // aber zurueckgesetzt.
        f0.classSym = symA;
        f0.AddTag(symA);
        snap.Clear();
        if (snap.Count() != 0)                          return false;

        ChefZ_ItemFacts again = snap.Acquire();
        if (again.classSym != ChefZ_SymbolTable.INVALID) return false;
        if (again.tags.Count() != 0)                     return false;
        if (again.slotBoundTo != -1)                     return false;
        if (again.freshness01 != 1.0)                    return false;
        if (!again.closure.IsEmpty())                    return false;

        // Tags sind duplikatfrei, INVALID wird nicht aufgenommen.
        again.AddTag(symA);
        again.AddTag(symA);
        again.AddTag(ChefZ_SymbolTable.INVALID);
        if (again.tags.Count() != 1)                     return false;
        if (!again.HasTag(symA))                         return false;
        if (again.HasTag(symB))                          return false;

        // Zuruecknehmen entfernt genau den letzten Eintrag - und gibt seinen
        // Platz im Vorrat NICHT wieder frei. Sonst zeigten zwei Namen auf
        // denselben Datensatz.
        snap.Clear();
        ChefZ_ItemFacts keep = snap.Acquire();
        ChefZ_ItemFacts drop = snap.Acquire();
        snap.DiscardLast();
        if (snap.Count() != 1)                           return false;
        if (snap.Get(0) != keep)                         return false;

        ChefZ_ItemFacts next = snap.Acquire();
        if (next == keep)                                return false;
        if (next == drop)                                return false;
        if (snap.Count() != 2)                           return false;

        // Auf einer leeren Sicht ist Zuruecknehmen folgenlos.
        snap.Clear();
        snap.DiscardLast();
        if (snap.Count() != 0)                           return false;

        return true;
    }
}
