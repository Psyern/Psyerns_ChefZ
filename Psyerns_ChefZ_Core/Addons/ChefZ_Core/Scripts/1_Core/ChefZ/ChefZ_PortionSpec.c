//==============================================================================
// ChefZ_PortionSpec - was ein Portionsgericht ueber sich weiss
//
// Entwurf: 15 §3 (Feldliste woertlich), 15 §4 (wo die Felder wirken), 15 §5
// (die beiden Deckel), 15 §6 (Zustandstabelle), 15 §7 (Fehlerverhalten),
// 15 E1 (Zaehler statt N Einzelitems), 15 E2 (NICHT die Vanilla-quantity),
// 15 E7 (Einzelgerichte sind Portionsgerichte mit portions = 1).
//
// ---------------------------------------------------------------------------
// Was diese Klasse ist
// ---------------------------------------------------------------------------
// Die ausgewertete, entsentinelte Form der Portionsfelder EINES
// ChefZ_OutputDef. Sie entsteht beim Boot im ChefZ_PortionManager und wird
// danach nie wieder veraendert - dieselbe Trennung Rohform/kompiliert wie bei
// Selektoren (07 §2.2) und Rezepten (08 §7).
//
// Der Grund fuer die eigene Klasse statt "der Manager liest das OutputDef
// direkt": das OutputDef ist die Datei, diese Klasse ist die Regel. In der
// Datei kann jedes Feld fehlen; hier hat jedes Feld einen Wert. Der Unterschied
// ist genau eine Auswertung wert - und er sorgt dafuer, dass die
// Entnahmeaktion (4_World) nie einen Sentinel sieht.
//
// ---------------------------------------------------------------------------
// Ergaenzungen gegenueber der Feldliste aus 15 §3, und warum jede noetig ist
// ---------------------------------------------------------------------------
//   bulkClass        Der Registryschluessel. 15 §6 fuehrt eine "Portions-
//                    Registry, aus den Rezept-Outputs abgeleitet"; ohne die
//                    Klasse, zu der eine Spec gehoert, gaebe es nichts zu
//                    schluesseln.
//   returnContainer  Steht bereits im OutputDef (16 §4, "AUTO"). Sie hier
//                    mitzufuehren kostet ein Feld und erspart S17 den zweiten
//                    Weg zurueck zum Rezept - das Gericht kennt sein Rezept
//                    beim Verzehr nicht mehr (16 E3).
//   displayName      15 E5: "GetText() aus der PortionSpec, nicht aus Code".
//                    Die Feldliste in 15 §3 nennt kein Textfeld; ohne eines
//                    waere die Zusage nicht erfuellbar, und der Aktionstext
//                    muesste im Core stehen - also Content im Core.
//   sourceRef        Welches Rezept diese Spec beigesteuert hat. Nur fuer
//                    Meldungen; ohne sie nennt eine Kollisionswarnung zwei
//                    Zahlen und keinen Autor.
//
// KEIN CONTENT: kein Gericht, keine Schuessel, keine Kategorie steht hier.
// Jeder Wert kommt aus einer Datei.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Die harten Grenzen des Portionssystems.
 *
 * Bewusst Konstanten und nicht JSON - genauso wie ChefZ_SyncLimits, und aus
 * demselben Grund: MAX ist die Bitbreite der Sync-Variablen auf dem Item
 * (03 §4). Wer sie in einer Datei aendern koennte, koennte Registrierung und
 * Daten auseinanderlaufen lassen, und das faellt erst beim Spieler auf.
 */
class ChefZ_PortionLimits
{
    //! 15 §4: "n = clamp(n, 1, 31)". Ein Portionsgericht hat mindestens eine
    //! Portion - sonst waere es ein Gericht, das man nie essen kann.
    static const int MIN = 1;

    /**
     * MUSS ChefZ_SyncLimits.PORTIONS_MAX sein: zwei Zahlen, von denen eine 31
     * und die andere 15 sagt, ergaeben einen Client, der einen anderen Zaehler
     * sieht als der Server meint.
     *
     * Als LITERAL geschrieben und nicht als Verweis auf die andere Konstante:
     * die Auswertungsreihenfolge statischer Felder ueber Klassengrenzen hinweg
     * ist in Enforce nicht zugesichert (dieselbe Ueberlegung, die
     * ChefZ_RecordProbe.Bool() nicht als Feldinitialisierer zulaesst). Dass die
     * beiden Zahlen gleich sind, prueft ChefZ_PortionSpec.SelfCheck() - und
     * zwar bei jedem Serverstart.
     */
    static const int MAX = 31;

    /**
     * Kuerzeste Entnahmedauer.
     *
     * 15 E7 wuenscht takeDurationSec = 0 ("fast unsichtbar"). Vanillas
     * CAContinuousTime rechnet die Zeit als Nenner fort; eine Dauer von exakt
     * 0 waere dort eine Division durch null. Diese Untergrenze ist der Preis
     * dafuer, und sie ist so klein gewaehlt, dass sie den in E7 gemeinten
     * Effekt nicht spuerbar veraendert.
     */
    static const float MIN_TAKE_SEC = 0.1;

    //! Rueckfalldauer, wenn weder Rezept noch Einstellungen etwas sagen.
    static const float DEFAULT_TAKE_SEC = 2.0;

    static int Clamp(int n)
    {
        return Math.Clamp(n, MIN, MAX);
    }
}

//------------------------------------------------------------------------------

class ChefZ_PortionSpec
{
    //--- Identitaet (Ergaenzung, siehe Kopf) ----------------------------------
    string    bulkClass;
    ChefZ_Sym bulkClassSym;
    string    sourceRef;

    //--- 15 §3, woertlich -----------------------------------------------------
    int    portions;               // 0 oder 1 = kein Portionsgericht im engeren Sinn
    string portionClass;           // was bei einer Entnahme entsteht
    float  portionQuantity;        // Quantity je Portion; < 0 = Klassendefault
    float  amountPerPortion;       // Mengendeckel; <= 0 = kein Mengendeckel
    string containerCategory;      // Behaelter, den die Entnahme braucht; "" = keiner

    /**
     * Dieselbe Kategorie als Symbol (S17, 16 §3.2).
     *
     * ERGAENZUNG gegenueber 15 §3, und eine notwendige: die
     * ChefZ_ContainerRegistry rechnet ausschliesslich auf Symbolen, und
     * gefragt wird sie in ChefZ_ActionTakePortion.ActionCondition() - also bei
     * JEDEM Zielwechsel des Fadenkreuzes. Dort jedes Mal zu internieren hiesse,
     * die Symboltabelle an einem Anzeigepfad wachsen zu lassen.
     *
     * Gesetzt genau einmal, beim Build in FillFrom(). INVALID heisst "diese
     * Spec verlangt keinen Behaelter".
     */
    ChefZ_Sym containerCategorySym;

    bool   consumesContainer;
    string emptyOnLastPortion;     // was aus dem Bulk-Rest wird; "" = loeschen
    bool   scaleWithDevice;
    bool   inheritQuality;
    bool   inheritState;
    bool   inheritFreshness;
    float  takeDurationSec;

    //--- Ergaenzungen (siehe Kopf) -------------------------------------------
    string returnContainer;        // "" | "AUTO" | Klassenname (16 §4)
    string displayName;            // Stringtable-Schluessel des Aktionstexts

    //--------------------------------------------------------------------------

    void ChefZ_PortionSpec()
    {
        bulkClass          = "";
        bulkClassSym       = ChefZ_SymbolTable.INVALID;
        sourceRef          = "";

        portions           = 0;
        portionClass       = "";
        portionQuantity    = -1.0;
        amountPerPortion   = 0.0;
        containerCategory  = "";
        containerCategorySym = ChefZ_SymbolTable.INVALID;
        consumesContainer  = true;
        emptyOnLastPortion = "";
        scaleWithDevice    = true;
        inheritQuality     = true;
        inheritState       = true;
        inheritFreshness   = true;
        takeDurationSec    = ChefZ_PortionLimits.DEFAULT_TAKE_SEC;

        returnContainer    = "";
        displayName        = "";
    }

    /**
     * Uebernimmt die Portionsfelder eines Ergebnisses (15 §3, Content-Beispiel).
     *
     * Der Aufrufer hat ChefZ_OutputDef.ResolveDefaults() bereits laufen
     * lassen - das tut der Record-Sink fuer jedes geladene Rezept. Die vier
     * bool-Schalter tragen deshalb hier bereits ihren Vorgabewert TRUE, und
     * diese Methode kopiert sie nur noch.
     *
     * @param defaultTakeSec  die Vorgabe aus den CoreSettings. Sie kommt
     *        HEREIN und wird nicht hier nachgeschlagen: 1_Core kennt den
     *        Config Manager nicht, und ein Layer-Verstoss waere ein hoher
     *        Preis fuer eine Zahl.
     */
    void FillFrom(notnull ChefZ_OutputDef def, string cls, string owner, float defaultTakeSec)
    {
        bulkClass    = cls;
        bulkClassSym = ChefZ_SymbolTable.Intern(cls);
        sourceRef    = owner;

        portions           = def.portions;
        portionClass       = def.portionClass;
        containerCategory  = def.containerCategory;
        emptyOnLastPortion = def.emptyOnLastPortion;

        // Einmal internieren, nie wieder (siehe Feldkommentar). Eine leere
        // Kategorie bleibt INVALID - ChefZ_SymbolTable.Intern("") liefert
        // ohnehin kein gueltiges Symbol, der Zweig steht hier trotzdem, damit
        // die Absicht lesbar ist.
        containerCategorySym = ChefZ_SymbolTable.INVALID;
        if (containerCategory != "")
            containerCategorySym = ChefZ_SymbolTable.Intern(containerCategory);
        returnContainer    = def.returnContainer;
        displayName        = def.takeDisplayName;

        consumesContainer  = def.consumesContainer;
        scaleWithDevice    = def.scaleWithDevice;
        inheritQuality     = def.inheritQuality;
        inheritState       = def.inheritState;
        inheritFreshness   = def.inheritFreshness;

        // Sentinel aufloesen. "Nichts gesagt" heisst bei der Menge
        // "Klassendefault der Portionsklasse" und beim Mengendeckel "kein
        // Deckel" - beides sind ausdruecklich zulaessige Angaben und keine
        // Fehler (15 §7, Zeile 1).
        portionQuantity  = -1.0;
        if (def.HasPortionQuantity())
            portionQuantity = def.portionQuantity;

        amountPerPortion = 0.0;
        if (def.HasAmountPerPortion())
            amountPerPortion = def.amountPerPortion;

        takeDurationSec = defaultTakeSec;
        if (def.HasTakeDuration())
            takeDurationSec = def.takeDurationSec;
        if (takeDurationSec < 0.0)
            takeDurationSec = 0.0;
    }

    //--------------------------------------------------------------------------

    //! 15 E7: ein Einzelgericht IST ein Portionsgericht mit portions = 1. Die
    //! Frage lautet deshalb nicht "mehr als eine Portion", sondern "gibt es
    //! ueberhaupt etwas zu entnehmen".
    bool IsPortioned()
    {
        return portions >= 1 && portionClass != "";
    }

    bool RequiresContainer()
    {
        return containerCategory != "";
    }

    //! 15 §5.2: der Mengendeckel wirkt nur, wenn er gesetzt ist. Ohne ihn
    //! entscheidet allein das Geraet - und das ist ausbeutbar, deshalb steht
    //! die Warnung dazu im ChefZ_PortionManager und nicht hier.
    bool HasAmountCap()
    {
        return amountPerPortion > 0.0;
    }

    //! Die Dauer, die der Fortschrittsbalken abbilden soll. Nie 0 - siehe
    //! ChefZ_PortionLimits.MIN_TAKE_SEC.
    float EffectiveTakeSeconds()
    {
        if (takeDurationSec < ChefZ_PortionLimits.MIN_TAKE_SEC)
            return ChefZ_PortionLimits.MIN_TAKE_SEC;
        return takeDurationSec;
    }

    string ToDebugString()
    {
        string s = bulkClass + " -> " + portionClass
                 + " x" + portions.ToString();

        if (HasAmountCap())
            s = s + " je=" + amountPerPortion.ToString() + "E";
        if (!scaleWithDevice)
            s = s + " ohneGeraetedeckel";
        if (portionQuantity >= 0.0)
            s = s + " menge=" + portionQuantity.ToString();
        if (RequiresContainer())
            s = s + " behaelter=" + containerCategory;
        if (emptyOnLastPortion != "")
            s = s + " rest=" + emptyOnLastPortion;
        if (sourceRef != "")
            s = s + "  aus " + sourceRef;

        return s;
    }

    //==========================================================================
    // Nur fuer den Selbsttest
    //==========================================================================

    static bool SelfCheck()
    {
        ChefZ_PortionSpec spec = new ChefZ_PortionSpec();

        // Die vier Schalter aus 15 §3 haben Vorgabe TRUE, auch ohne Datei.
        if (!spec.consumesContainer)                      return false;
        if (!spec.scaleWithDevice)                        return false;
        if (!spec.inheritQuality)                         return false;
        if (!spec.inheritState)                           return false;
        if (!spec.inheritFreshness)                       return false;

        // Eine leere Spec ist ausdruecklich KEIN Portionsgericht.
        if (spec.IsPortioned())                           return false;
        if (spec.RequiresContainer())                     return false;
        if (spec.HasAmountCap())                          return false;

        // Ein Ergebnis ohne jede Portionsangabe darf keine Spec ergeben
        // (15 §7, Zeile 1).
        ChefZ_OutputDef bare = new ChefZ_OutputDef();
        bare.cls = "CHEFZ_PO_TESTKLASSE";
        bare.ResolveDefaults();
        if (bare.IsPortioned())                           return false;

        ChefZ_OutputDef def = new ChefZ_OutputDef();
        def.cls              = "CHEFZ_PO_BULK";
        def.portions         = 8;
        def.portionClass     = "CHEFZ_PO_SCHALE";
        def.portionQuantity  = 200.0;
        def.amountPerPortion = 1.0;
        def.ResolveDefaults();

        if (!def.IsPortioned())                           return false;
        if (!def.scaleWithDevice)                         return false;
        if (!def.consumesContainer)                       return false;

        spec.FillFrom(def, def.cls, "CHEFZ_PO_REZEPT", 2.0);
        if (!spec.IsPortioned())                          return false;
        if (spec.portions != 8)                           return false;
        // Ohne containerCategory bleibt das Symbol INVALID (S17, 16 §3.2).
        if (ChefZ_SymbolTable.IsValid(spec.containerCategorySym)) return false;
        if (spec.portionQuantity != 200.0)                return false;
        if (!spec.HasAmountCap())                         return false;
        if (spec.takeDurationSec != 2.0)                  return false;
        if (spec.bulkClass != "CHEFZ_PO_BULK")            return false;

        // takeDurationSec = 0 ist zulaessig (15 E7) und wird trotzdem nie zu
        // einer Dauer von 0 - sonst teilte der Fortschrittsbalken durch null.
        def.takeDurationSec = 0.0;
        spec.FillFrom(def, def.cls, "CHEFZ_PO_REZEPT", 2.0);
        if (spec.takeDurationSec != 0.0)                  return false;
        if (spec.EffectiveTakeSeconds() < ChefZ_PortionLimits.MIN_TAKE_SEC) return false;

        // Die Klammer aus 15 §4.
        if (ChefZ_PortionLimits.Clamp(0)   != ChefZ_PortionLimits.MIN) return false;
        if (ChefZ_PortionLimits.Clamp(-5)  != ChefZ_PortionLimits.MIN) return false;
        if (ChefZ_PortionLimits.Clamp(500) != ChefZ_PortionLimits.MAX) return false;
        if (ChefZ_PortionLimits.Clamp(7)   != 7)                       return false;

        // Die Grenze MUSS die Sync-Grenze sein (15 §7, "portions > 31").
        if (ChefZ_PortionLimits.MAX != ChefZ_SyncLimits.PORTIONS_MAX)   return false;

        return true;
    }
}
