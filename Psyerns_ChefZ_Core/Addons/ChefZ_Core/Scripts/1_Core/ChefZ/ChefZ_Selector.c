//==============================================================================
// ChefZ_Selector / ChefZ_SlotDef - die ROHFORM aus JSON
//
// Entwurf: 07 §2.1 (Selektor, Feldliste woertlich), 07 §2.3 (Slot, Feldliste
// woertlich), 07 §3 (die sechs Matching-Arten aus Architekturplan §15 als
// Sonderfaelle EINES Mechanismus), 07 E1, E2, E3, E5.
//
// ---------------------------------------------------------------------------
// Was hier steht und was nicht
// ---------------------------------------------------------------------------
// Das hier ist die unverarbeitete Sicht: Strings, wie ein Autor sie schreibt,
// und Sentinel fuer "nicht gesetzt". Ausgewertet wird NIE auf dieser Form -
// dafuer gibt es ChefZ_CompiledSelector und ChefZ_CompiledSlot. Die Trennung
// ist dieselbe wie ChefZ_IngredientDef gegen ChefZ_IngredientInfo (05):
// Rohdaten duerfen unvollstaendig sein, das Ergebnis nicht.
//
// KEIN CONTENT. Diese Datei nennt keine Kategorie, keinen Tag, keinen Zustand
// und keine Klasse. Sie beschreibt nur, WELCHE FELDER ein Selektor hat.
//
// ---------------------------------------------------------------------------
// Eine bewusste Abweichung von der Schreibweise im Entwurf
// ---------------------------------------------------------------------------
// 07 §2.1 schreibt   string cls;   // JSON-Feld "class"
//
// Ein JSON-Schluessel "class" ist mit JsonFileLoader NICHT erreichbar: der
// Enforce-Serializer bildet Schluessel auf Feldnamen ab, und "class" ist ein
// Schluesselwort - ein Feld dieses Namens laesst sich nicht deklarieren. Der
// Entwurf selbst schreibt an anderer Stelle (08 §2, ChefZ_OutputDef) bereits
// "cls" im JSON. Der JSON-Schluessel ist deshalb ebenfalls "cls":
//
//     { "cls": "ChefZ_HunterSausage" }
//
// Das ist eine Schreibweise, keine Semantikaenderung. Wer "class" schreibt,
// bekommt einen leeren Selektor und damit einen klaren Kompilierfehler
// (07 §7, erste Zeile) - nicht etwa stilles Matchen auf alles.
//
// ---------------------------------------------------------------------------
// Zum Feldnamen "not"
// ---------------------------------------------------------------------------
// "not" steht so im Entwurf (07 §2.1) und ist deshalb auch der JSON-Schluessel.
// Es ist in der Enforce-Schluesselwortliste nicht enthalten (anders als
// "class"), und Vanilla belegt den Namen nirgends. Sollte ein kuenftiger
// Compiler ihn dennoch zurueckweisen, ist die Anpassung klein und an genau
// vier Stellen noetig: dieses Feld, Normalize(), CountPredicates()/
// PredicateNames() und der Zweig in ChefZ_SelectorCompiler.CompilePredicate().
// Der JSON-Schluessel hiesse dann "notOf".
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_Selector : Managed
{
    //--- Blatt-Praedikate: genau EINES gesetzt (07 §2.1) ---------------------
    string  cls;                    // exakte Klasse     (JSON "cls", siehe Kopf)
    string  category;               // Kategorie inklusive aller Unterkategorien
    string  tag;                    // effektiver Tag (Klasse + Zustand + Qualitaet)
    string  state;                  // ChefZ-Zustandssymbol
    string  vanillaStage;           // "Raw"|"Baked"|"Boiled"|"Dried"|"Burned"|"Rotten"

    //--- Kombinatoren, rekursiv ----------------------------------------------
    ref array<ref ChefZ_Selector> anyOf;    // ODER
    ref array<ref ChefZ_Selector> allOf;    // UND
    ref ChefZ_Selector            not;      // NICHT

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

    void ChefZ_Selector()
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
        anyOf             = null;
        allOf             = null;
        not               = null;
        health            = null;
        freshness         = null;
        temperature       = null;
        wetness           = null;
        cleanness         = null;
        quantity          = null;
        quantityPct       = null;
    }

    //--------------------------------------------------------------------------
    // Auskuenfte fuer den Compiler. Bewusst hier und nicht im Compiler: es
    // sind Aussagen ueber die Rohform, und wer ein Feld hinzufuegt, findet sie
    // in derselben Datei.
    //--------------------------------------------------------------------------

    void Normalize()
    {
        cls.TrimInPlace();
        category.TrimInPlace();
        tag.TrimInPlace();
        state.TrimInPlace();
        vanillaStage.TrimInPlace();
        minQuality.TrimInPlace();
        liquidType.TrimInPlace();

        NormalizeChildren(anyOf);
        NormalizeChildren(allOf);
        if (not)
            not.Normalize();
    }

    private void NormalizeChildren(array<ref ChefZ_Selector> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_Selector child = list.Get(i);
            if (child)
                child.Normalize();
        }
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
        if (anyOf)                  n++;
        if (allOf)                  n++;
        if (not)                    n++;
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
        if (anyOf)                  s = Append(s, "anyOf");
        if (allOf)                  s = Append(s, "allOf");
        if (not)                    s = Append(s, "not");
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

//==============================================================================

/**
 * Eine Slotdefinition in Rohform (07 §2.3).
 *
 * Der Slot sagt, WIE VIEL und WAS DANACH GESCHIEHT; der Selektor sagt nur, was
 * passt. Diese Trennung ist 07 E2: "optional" ist eine Aussage ueber das
 * Rezept, nicht ueber das Item - derselbe Selektor kann in einem Rezept
 * Pflicht und in einem anderen Kuer sein.
 *
 * Zu den Defaults (07 §2.3, 07 §7):
 *   minCount      1
 *   maxCount      = minCount
 *   optional      false
 *   allowPartial  true
 *   consume       "whole"
 *   gradePoints   0
 *   excludeStates aus CoreSettings.defaultExcludedStates (07 E5)
 *
 * Ein weggelassenes excludeStates und ein leeres excludeStates sind
 * VERSCHIEDENE Aussagen, und das ist der Kern von 07 E5:
 *   fehlt        -> globaler Default greift (verdorbenes Fleisch faellt raus)
 *   []           -> Autor laesst ausdruecklich alles zu (Fermentation, V2)
 * Fuer ref-Arrays ist der Unterschied ohne Kunstgriff sichtbar: abwesend
 * bleibt null (02 E3, Mittel 1).
 *
 * Zu bool: allowPartial hat den Default TRUE, und ein abwesendes bool ist im
 * ueberlebenden Parse-Durchgang false (ChefZ_RecordProbe). Deshalb dieselbe
 * Bool-Sonde wie bei den Records - ohne sie waere "allowPartial nicht
 * geschrieben" nicht von "allowPartial: false" zu unterscheiden, und der
 * dokumentierte Default waere unerreichbar.
 */
class ChefZ_SlotDef : Managed
{
    string             slotId;         // stabil, erscheint in Trace und Grade-Regeln
    ref ChefZ_Selector match;
    int                minCount;
    int                maxCount;
    ref ChefZ_Range    amount;         // Menge in Rezepteinheiten (05 §6)
    string             unit;
    bool               optional;
    bool               allowPartial;
    string             consume;        // "whole" | "amount" | "none"
    float              consumeAmount;
    string             setStateAfter;  // bei consume "none": Zustandswechsel
    int                gradePoints;    // Beitrag zur Qualitaet (12)
    ref array<string>  excludeStates;

    //! Wie ChefZ_Record.explicitFields (02 E3, Mittel 3), nur fuer die beiden
    //! bool-Felder dieses Unterobjekts. Der Rezeptleser (S6) traegt sie aus
    //! dem Sondendurchgang nach.
    ref array<string>  explicitFields;

    void ChefZ_SlotDef()
    {
        slotId         = "";
        unit           = "";
        consume        = "";
        setStateAfter  = "";
        minCount       = ChefZ_Undefined.INT;
        maxCount       = ChefZ_Undefined.INT;
        gradePoints    = ChefZ_Undefined.INT;
        consumeAmount  = ChefZ_Undefined.FLOAT;
        match          = null;
        amount         = null;
        excludeStates  = null;
        explicitFields = null;

        optional     = ChefZ_RecordProbe.Bool();
        allowPartial = ChefZ_RecordProbe.Bool();
    }

    void Normalize()
    {
        slotId.TrimInPlace();
        unit.TrimInPlace();
        consume.TrimInPlace();
        setStateAfter.TrimInPlace();
        if (match)
            match.Normalize();
        if (excludeStates)
        {
            for (int i = 0; i < excludeStates.Count(); i++)
            {
                string s = excludeStates.Get(i);
                s.TrimInPlace();
                excludeStates.Set(i, s);
            }
        }
    }

    void MarkExplicit(string field)
    {
        if (!explicitFields)
            explicitFields = new array<string>();
        if (explicitFields.Find(field) < 0)
            explicitFields.Insert(field);
    }

    bool HasExplicit(string field)
    {
        if (!explicitFields)
            return false;
        return explicitFields.Find(field) >= 0;
    }

    /**
     * Vergleich mit demselben Slot aus dem Sondendurchgang: was in BEIDEN
     * Durchgaengen gleich ist, stand ausdruecklich in der Datei.
     *
     * Wird der Aufruf vergessen, gilt jedes bool als "nicht gesetzt" und
     * bekommt seinen Code-Default. Das ist die harmlose Richtung: ein
     * vergessener Aufruf schaltet keinen Filter ab.
     */
    void CaptureExplicitBools(ChefZ_SlotDef other)
    {
        if (!other)
            return;
        if (optional     == other.optional)     MarkExplicit("optional");
        if (allowPartial == other.allowPartial) MarkExplicit("allowPartial");
    }

    //! Setzt die Code-Defaults aus 07 §2.3 ein. Klammerungen und Meldungen
    //! macht der Compiler, nicht dieser Datensatz - er hat keinen Bericht.
    void ResolveDefaults()
    {
        minCount    = ChefZ_Undefined.IntOr(minCount, 1);
        maxCount    = ChefZ_Undefined.IntOr(maxCount, minCount);
        gradePoints = ChefZ_Undefined.IntOr(gradePoints, 0);
        consume     = ChefZ_Undefined.TextOr(consume, ChefZ_ConsumeMode.WHOLE_NAME);

        if (!HasExplicit("optional"))
            optional = false;
        if (!HasExplicit("allowPartial"))
            allowPartial = true;
    }
}
