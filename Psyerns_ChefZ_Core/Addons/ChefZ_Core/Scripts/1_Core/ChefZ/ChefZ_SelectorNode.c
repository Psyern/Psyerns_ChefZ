//==============================================================================
// ChefZ_SelectorNode - die Basis aller Selektorknoten, ohne Selbstbezug
//
// ---------------------------------------------------------------------------
// WARUM ES DIESE KLASSE UEBERHAUPT GIBT
// ---------------------------------------------------------------------------
// Der JSON-Deserialisierer der Engine (JsonFileLoader/JsonSerializer) bricht an
// einer SELBSTBEZUEGLICHEN Klasse ab. Nicht mit einer Fehlermeldung, nicht mit
// einer Skriptausnahme - der Serverprozess endet mit einer Zugriffsverletzung,
// ohne Aufrufkeller und ohne Dateinamen. Der Grund ist der Typbeschreiber, den
// die Engine beim ersten Lesen rekursiv aufbaut: eine Klasse, die sich selbst
// enthaelt, laesst diesen Aufbau nicht enden.
//
// Nachgewiesen am 28.08.2026 auf dem Testserver, in dieser Reihenfolge:
//   - Der Absturz traf ausschliesslich Dokumentarten, deren Datensaetze einen
//     Selektor tragen (transform, recipe). category, tag, nutrition,
//     preservation, station, ingredient liefen durch.
//   - ChefZ_Range - ebenfalls eine geschachtelte ref-Klasse, aber ohne
//     Selbstbezug - ist unauffaellig; ChefZ_PreservationDef traegt eine und
//     wird fehlerfrei gelesen.
//   - Der Name spielt keine Rolle: "not" in "notSelector" umbenannt, Absturz
//     unveraendert.
//   - Der Gegenbeweis: eine kuenstlich selbstbezuegliche Sonde an
//     ChefZ_PreservationDef gehaengt - und Preservation.json stuerzte ab,
//     obwohl es bis dahin fehlerfrei gelesen wurde.
//   - Unter den rund 35 Mods dieses Servers - Expansion, Community Framework,
//     Terje, Ninjins, LBmaster, DME - gibt es KEINE einzige selbstbezuegliche
//     Klasse. ChefZ war die einzige, und ChefZ war die einzige, die abstuerzte.
//
// ---------------------------------------------------------------------------
// DIE LOESUNG: EINE KETTE STATT EINES ZYKLUS
// ---------------------------------------------------------------------------
// Ein Selektorbaum bleibt ein Baum, aber jede Ebene bekommt ihren eigenen Typ:
//
//   ChefZ_Selector  ->  ChefZ_SelectorL1  ->  ...  ->  ChefZ_SelectorL8
//
// ChefZ_SelectorL8 hat keine Kinder mehr. Damit endet die Typkette, und der
// Deserialisierer kommt zum Schluss. Fuer den Autor einer Rezeptdatei aendert
// sich NICHTS: dieselben Schluessel, dieselbe Schachtelung, dieselben Dateien.
//
// Die gemessene Schachtelungstiefe des gesamten vorhandenen Inhalts ist 2.
// Acht Ebenen lassen also reichlich Luft - und weil coreSettings.maxSelectorDepth
// mit 8 vorbelegt ist, bleibt diese Grenze weiterhin die wirksame: eine Datei
// mit neun Ebenen ist baubar und wird vom Compiler abgewiesen, statt still an
// der Typkette zu scheitern.
//
// ---------------------------------------------------------------------------
// WARUM DIE KINDER UEBER "Collect" LAUFEN UND NICHT UEBER EINEN CAST
// ---------------------------------------------------------------------------
// Enforce-Templates sind invariant: array<ref ChefZ_SelectorL1> ist KEIN
// array<ref ChefZ_SelectorNode>, auch wenn L1 von Node erbt. Ein Cast waere
// entweder abgelehnt oder still falsch. Jede Ebene kopiert ihre Kinder deshalb
// in eine frische Liste des Basistyps. Das kostet einen Zeiger je Kind und
// genau einmal je Kompilierlauf - der Selektor wird beim Laden uebersetzt, nie
// im Spielbetrieb.
//
// Dadurch bleibt ChefZ_SelectorCompiler EINE Funktion und muss die Ebenen
// nicht kennen.
//
// Layer: 1_Core. Reine Daten, kein Engine-Typ.
//==============================================================================

class ChefZ_SelectorNode : Managed
{
    //--- Blatt-Praedikate: genau EINES gesetzt (07 §2.1) ---------------------
    string  cls;                    // exakte Klasse     (JSON "cls")
    string  category;               // Kategorie inklusive aller Unterkategorien
    string  tag;                    // effektiver Tag (Klasse + Zustand + Qualitaet)
    string  state;                  // ChefZ-Zustandssymbol
    string  vanillaStage;           // "Raw"|"Baked"|"Boiled"|"Dried"|"Burned"|"Rotten"

    //--- Wertbereiche, additiv UND-verknuepft mit dem Blatt -------------------
    ref ChefZ_Range health;                 // 0..1
    ref ChefZ_Range freshness;              // 0..1
    ref ChefZ_Range temperature;
    ref ChefZ_Range wetness;
    ref ChefZ_Range cleanness;              // Vorbereitung Hygiene, V2
    ref ChefZ_Range quantity;               // absolut
    ref ChefZ_Range quantityPct;            // 0..1 relativ zu quantityMax
    string  minQuality;                     // Qualitaetssymbol, "" = egal

    //--- Fluessigkeit ---------------------------------------------------------
    //
    // isLiquidContainer hat keinen Sentinel - bool hat zwei Werte, und beide
    // sind Nutzdaten (02 E3). "false" heisst deshalb hier zuverlaessig NUR
    // "nicht gesetzt". Wer "dieses Item ist KEIN Fluessigkeitsbehaelter"
    // ausdruecken will, schreibt   { "not": { "isLiquidContainer": true } }  -
    // dafuer ist der Negationsknoten da.
    bool    isLiquidContainer;
    string  liquidType;

    void ChefZ_SelectorNode()
    {
        // Strings starten leer, ref-Felder null. Beides ist bereits der
        // Sentinel "nicht gesetzt" (02 E3, Mittel 1 und 2) - fuer Strings ist
        // der Leerstring genau ChefZ_Undefined.TEXT. Es ist trotzdem
        // ausgeschrieben, weil ein Selektor auch von Hand gebaut wird (Test,
        // Vorschau) und dort niemand auf Enforce-Feldinitialisierung wetten
        // soll.
        cls               = "";
        category          = "";
        tag               = "";
        state             = "";
        vanillaStage      = "";
        minQuality        = "";
        liquidType        = "";
        isLiquidContainer = false;
        health            = null;
        freshness         = null;
        temperature       = null;
        wetness           = null;
        cleanness         = null;
        quantity          = null;
        quantityPct       = null;
    }

    //==========================================================================
    // Kinder - von jeder Ebene ueberschrieben, hier bewusst leer
    //==========================================================================

    //! Haengt die anyOf-Kinder an outList an. outList wird NICHT geleert.
    void CollectAnyOf(notnull array<ref ChefZ_SelectorNode> outList) { }

    //! Haengt die allOf-Kinder an outList an. outList wird NICHT geleert.
    void CollectAllOf(notnull array<ref ChefZ_SelectorNode> outList) { }

    //! Der Negationsknoten, oder null.
    ChefZ_SelectorNode GetNot() { return null; }

    //! Ob der Kombinator ueberhaupt geschrieben wurde. Wichtig: ein LEERES
    //! anyOf ist gesetzt und damit ein Praedikat - 07 §7 verlangt dafuer einen
    //! Kompilierfehler und keine stille Annahme.
    bool HasAnyOf() { return false; }
    bool HasAllOf() { return false; }

    //! Die letzte Ebene der Kette hat keine Kinder mehr. Wer hier true
    //! zurueckgibt, sagt dem Compiler: eine weitere Schachtelung ist in dieser
    //! Datei nicht darstellbar gewesen.
    bool IsLastLevel() { return true; }

    //==========================================================================
    // Auskuenfte fuer den Compiler. Bewusst hier und nicht im Compiler: es
    // sind Aussagen ueber die Rohform, und wer ein Feld hinzufuegt, findet sie
    // in derselben Datei.
    //==========================================================================

    void Normalize()
    {
        cls.TrimInPlace();
        category.TrimInPlace();
        tag.TrimInPlace();
        state.TrimInPlace();
        vanillaStage.TrimInPlace();
        minQuality.TrimInPlace();
        liquidType.TrimInPlace();

        array<ref ChefZ_SelectorNode> kinder = new array<ref ChefZ_SelectorNode>();
        CollectAnyOf(kinder);
        CollectAllOf(kinder);
        for (int i = 0; i < kinder.Count(); i++)
        {
            ChefZ_SelectorNode kind = kinder.Get(i);
            if (kind)
                kind.Normalize();
        }

        ChefZ_SelectorNode negation = GetNot();
        if (negation)
            negation.Normalize();
    }

    //! Anzahl der gesetzten Blatt- und Kombinatorfelder. Genau EINS ist
    //! zulaessig; mehr als eins ist ein Kompilierfehler (07 §7).
    int CountPredicates()
    {
        int n = 0;
        if (cls != "")              n++;
        if (category != "")         n++;
        if (tag != "")              n++;
        if (state != "")            n++;
        if (vanillaStage != "")     n++;
        if (HasLiquidPredicate())   n++;
        if (HasAnyOf())             n++;
        if (HasAllOf())             n++;
        if (GetNot())               n++;
        return n;
    }

    //! Namen der gesetzten Praedikatfelder - fuer die Fehlermeldung, die dem
    //! Autor sagt, WELCHE zwei Felder er gleichzeitig gesetzt hat.
    string PredicateNames()
    {
        string s = "";
        if (cls != "")              s = Append(s, "cls");
        if (category != "")         s = Append(s, "category");
        if (tag != "")              s = Append(s, "tag");
        if (state != "")            s = Append(s, "state");
        if (vanillaStage != "")     s = Append(s, "vanillaStage");
        if (HasLiquidPredicate())   s = Append(s, "isLiquidContainer/liquidType");
        if (HasAnyOf())             s = Append(s, "anyOf");
        if (HasAllOf())             s = Append(s, "allOf");
        if (GetNot())               s = Append(s, "not");
        if (s == "")
            return "(keines)";
        return s;
    }

    private string Append(string acc, string part)
    {
        if (acc == "")
            return part;
        return acc + ", " + part;
    }

    bool HasLiquidPredicate()
    {
        return isLiquidContainer || liquidType != "";
    }

    //! Ist ausser dem Praedikat noch etwas gesetzt, das einschraenkt?
    bool HasConstraints()
    {
        if (minQuality != "")
            return true;
        return CountRanges() > 0;
    }

    int CountRanges()
    {
        int n = 0;
        if (health)         n++;
        if (freshness)      n++;
        if (temperature)    n++;
        if (wetness)        n++;
        if (cleanness)      n++;
        if (quantity)       n++;
        if (quantityPct)    n++;
        return n;
    }

    //! Vollstaendig leer - weder Praedikat noch Kind noch Einschraenkung.
    //! Ein solcher Selektor traefe auf ALLES; 07 §7 verlangt dafuer einen
    //! Kompilierfehler, keine stille Annahme.
    bool IsEmpty()
    {
        return CountPredicates() == 0 && !HasConstraints();
    }
}
