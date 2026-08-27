//==============================================================================
// ChefZ_ContainerDef - ein Behaelter als DATEN, nie als Klassenname im Code
//
// Entwurf: 16 §3.1 (Deklaration in der Game-Config, woertlich), 16 §3.2
// (Feldliste woertlich), 16 §4 ("AUTO"), 16 §7 (Fehlerverhalten Zeile fuer
// Zeile), 16 E2 (Kategorien statt Klassenlisten), 16 E5 (searchScope als
// Bitfeld), 16 E7 (spoilageModifier), 02 E3 (Bool-Sonde).
//
// ---------------------------------------------------------------------------
// Die eine Zusage dieser Datei
// ---------------------------------------------------------------------------
// Im gesamten Core steht kein Tellername. Ein Rezept fordert eine KATEGORIE
// ("die Portion braucht eine Schuessel"); WELCHE Klassen dieser Kategorie
// angehoeren, steht in CfgChefZContainers und damit im Content oder beim
// Serverbetreiber. Ein spaeter hinzugefuegter Holznapf traegt sich mit
// containerCategories[] ein und funktioniert SOFORT mit jedem bestehenden
// Schuesselgericht - ohne dass ein Rezept oder eine Zeile Core-Code angefasst
// wird (16 E2).
//
// ---------------------------------------------------------------------------
// Warum das in der GAME-CONFIG liegt und nicht in JSON (16 §3.1, 02 §2)
// ---------------------------------------------------------------------------
// ChefZ_ActionTakePortion.ActionCondition() laeuft auch auf dem CLIENT und
// muss dort entscheiden, ob eine passende Schuessel im Zugriff ist - sonst
// erscheint die Aktion, der Server weist sie ab, und der Spieler sieht einen
// Fortschrittsbalken, der nichts bewirkt. Der Client liest Rang 1
// (Game-Config) garantiert; ob er eine JSON-Datei aus einem PBO lesen kann,
// ist eine offene Messfrage (OF-10). Also gehoert die Behaelterzuordnung nach
// CfgChefZContainers.
//
// JSON bleibt trotzdem zulaessig - es ist derselbe Record. Ein Betreiber, der
// per $profile-Overlay einen Behaelter ergaenzt, aendert damit allerdings nur
// die SERVERSEITIGE Sicht; die Aktion erscheint dem Spieler dann eventuell
// nicht, obwohl der Server sie erlauben wuerde. Das steht hier, damit niemand
// es suchen muss.
//
// KEIN CONTENT: kein Behaelter, keine Kategorie, kein Gericht. "PLATE" und
// "BOWL" sind Beispiele aus dem Entwurf und kommen in dieser Datei nicht vor.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Wo nach einem Behaelter gesucht wird - ein Bitfeld je Behaelterklasse
 * (16 E5).
 *
 * Bewusst ein Bitfeld und kein enum: 16 §3.1 schreibt "searchScope = 3" als
 * ZAHL in die Game-Config, und ConfigGetInt liefert genau das. Ein enum
 * brauchte an dieser Stelle eine Umwandlung und fiele bei einem unbekannten
 * Wert still auf 0 - also auf "nirgends suchen", und damit auf einen
 * Behaelter, den niemand je findet.
 *
 * Die REIHENFOLGE der Suche steht NICHT hier, sondern in
 * ChefZ_ContainerService: Haende -> Inventar -> Umgebung, fest und nicht
 * konfigurierbar (16 E5). Das Bitfeld sagt nur, welche Stufen ueberhaupt
 * betreten werden.
 *
 * Layer: 1_Core.
 */
class ChefZ_ContainerScope
{
    static const int NONE         = 0;
    static const int HANDS        = 1;
    static const int INVENTORY    = 2;
    static const int NEARBY_CARGO = 4;

    /**
     * Vorgabe: Haende und Inventar (16 E5, woertlich "Default HANDS |
     * INVENTORY").
     *
     * NEARBY_CARGO ist ausdruecklich NICHT dabei: es wird bei Basen mit vielen
     * Kisten teuer und macht die Auswahl fuer den Spieler undurchsichtig - er
     * sieht nicht, aus welchem Fass sein Teller kam. Wer es will, sagt es.
     */
    static const int DEFAULT      = 3;      // HANDS | INVENTORY

    //! Alle bekannten Bits. Alles darueber ist ein Tippfehler und wird beim
    //! Build gemeldet und ausmaskiert - nicht uebernommen: ein unbekanntes Bit
    //! koennte in einer spaeteren Fassung eine Stufe bezeichnen, die dieser
    //! Core nicht durchsucht, und "unsichtbar nicht gesucht" ist der
    //! Fehlerfall, den niemand findet.
    static const int ALL          = 7;      // HANDS | INVENTORY | NEARBY_CARGO

    static bool Has(int scope, int bit)
    {
        return (scope & bit) != 0;
    }

    //! Bits, die dieser Core nicht kennt. 0 = alles verstanden.
    static int UnknownBits(int scope)
    {
        return scope & ~ALL;
    }

    static int Sanitize(int scope)
    {
        return scope & ALL;
    }

    static string Name(int scope)
    {
        if (scope == NONE)
            return "NONE";

        string s = "";
        if (Has(scope, HANDS))          s = s + "HANDS";
        if (Has(scope, INVENTORY))      { if (s != "") s = s + "|"; s = s + "INVENTORY"; }
        if (Has(scope, NEARBY_CARGO))   { if (s != "") s = s + "|"; s = s + "NEARBY_CARGO"; }

        int unknown = UnknownBits(scope);
        if (unknown != 0)
        {
            if (s != "")
                s = s + "|";
            s = s + "?" + unknown.ToString();
        }
        return s;
    }

    static string ValidNames()
    {
        return "1 = HANDS, 2 = INVENTORY, 4 = NEARBY_CARGO (Summe erlaubt, Vorgabe 3)";
    }
}

//------------------------------------------------------------------------------

class ChefZ_ContainerDef extends ChefZ_Record
{
    /**
     * Der Wert von returnContainer, der "gib genau den zurueck, der benutzt
     * wurde" bedeutet (16 §4).
     *
     * Er steht HIER und nicht als Zeichenkette an drei Stellen im Code, weil
     * genau das der Unterschied zwischen einer Zusage und einem Tippfehler
     * ist. Content-seitig ist es ein Wort in einer Datei; im Core ist es diese
     * eine Konstante.
     *
     * KEIN CONTENT: "AUTO" benennt kein Item, sondern eine Aufloesungsregel.
     */
    static const string AUTO = "AUTO";

    /**
     * Untergrenze des Verderbfaktors (16 §7, "spoilageModifier <= 0 -> auf
     * 0.01 geklemmt, WARN").
     *
     * 0 waere kein Faktor, sondern ein Totalstopp des Verfalls - und zwar
     * einer, den niemand aufgeschrieben hat. Wer Verfall wirklich anhalten
     * will, sagt das ueber einen Preservation-Record mit stopsDecay (14 E7);
     * dort steht es dann auch im Ladebericht.
     */
    static const float MIN_SPOILAGE = 0.01;

    //! Vorgabe: neutral. Ein Behaelter ohne Angabe soll die Haltbarkeit weder
    //! heben noch senken - dieselbe Ueberlegung wie bei
    //! ChefZ_DeviceDef.qualityModifier.
    static const float DEFAULT_SPOILAGE = 1.0;

    //--------------------------------------------------------------------------

    //! Kategorien, denen dieser Behaelter angehoert. 16 §7: mehrere sind
    //! ausdruecklich zulaessig - ein Napf kann BOWL und CONTAINER_DEEP sein.
    ref array<string> containerCategories;

    /**
     * Was beim vollstaendigen Verzehr zurueckkommt.
     *
     * Vorgabe ist die ID, also der Behaelter selbst: "eine Schuessel gibt eine
     * Schuessel zurueck" ist der Normalfall, und ihn hinschreiben zu muessen
     * waere eine Fehlerquelle ohne Gegenwert.
     *
     * Die Rueckgabe erzeugt eine NEUE Klasse; sie gibt nicht das urspruengliche
     * Objekt zurueck (16 §6). Damit ist keine Verknuepfung "dieses Gericht kam
     * aus jener Schuessel" zu speichern, und ein Serverneustart zwischen Kochen
     * und Essen aendert nichts.
     */
    string emptyClass;

    //! Kommt beim vollstaendigen Verzehr etwas zurueck? Vorgabe true.
    //! false ist der Konservenfall (16 §7): es wird nichts zurueckgegeben,
    //! und das ist kein Fehler.
    bool reusable;

    /**
     * Geht der Behaelter beim Servieren im gefuellten Gericht auf? Vorgabe
     * true.
     *
     * false heisst: der Behaelter bleibt beim Spieler. Dann darf auch nichts
     * zurueckgegeben werden - sonst haette der Spieler nach dem Essen zwei.
     * Diese Kopplung steht in ChefZ_ContainerRegistry.ReturnsEmpty() und
     * nicht hier, damit sie genau einmal existiert.
     */
    bool consumedOnServe;

    //! Faktor auf die Haltbarkeit des Inhalts (16 E7, wirkt in 14).
    //! "Eingemachtes im Glas haelt laenger" ist damit eine Zahl in einer Datei
    //! und braucht weder einen Zustand CANNED noch Sonderlogik.
    float spoilageModifier;

    //! Bitfeld, siehe ChefZ_ContainerScope.
    int searchScope;

    //! Stringtable-Schluessel fuer die Anzeige. Leer ist zulaessig; dann
    //! erscheint die ID.
    string displayName;

    //--------------------------------------------------------------------------

    void ChefZ_ContainerDef()
    {
        containerCategories = null;
        emptyClass          = ChefZ_Undefined.TEXT;
        displayName         = ChefZ_Undefined.TEXT;

        spoilageModifier    = ChefZ_Undefined.FLOAT;
        searchScope         = ChefZ_Undefined.INT;

        // bool ohne Sentinel: die Bool-Sonde traegt den Feldnamen in
        // explicitFields[] nach, wenn er in der Datei stand (02 E3). In der
        // Game-Config uebernimmt ChefZ_ConfigCppSource dieselbe Aufgabe -
        // dort IST die Anwesenheit des Eintrags die Aussage.
        reusable            = ChefZ_RecordProbe.Bool();
        consumedOnServe     = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.CONTAINER;
    }

    override void Normalize()
    {
        super.Normalize();
        emptyClass.TrimInPlace();
        displayName.TrimInPlace();
        ChefZ_TextList.TrimAll(containerCategories);
    }

    /**
     * Ein Behaelter, der zu KEINER Kategorie etwas sagt, wird abgewiesen.
     *
     * Er waere ein Eintrag, den kein Rezept je adressieren kann - und das ist
     * genau die Sorte Fehler, die wie fehlender Content aussieht: die
     * Entnahmeaktion erscheint einfach nicht, und niemand weiss warum.
     *
     * Kein Fehler ist dagegen eine LEERE Liste. "containerCategories": [] ist
     * eine ausdrueckliche Ansage ("dieser Behaelter ist derzeit in keiner
     * Kategorie") und im Overlay-Betrieb der einzige Weg, eine geerbte Liste
     * zu raeumen. Sie wird beim Build gemeldet, nicht hier abgewiesen -
     * dieselbe Trennung wie bei ChefZ_ToolGroupDef.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!containerCategories)
        {
            if (ctx)
                ctx.Error(this, "Behaelter nennt keine \"containerCategories\" (16 §3.1) - "
                    + "abgewiesen. Ein Behaelter ohne Kategorie kann von keinem Rezept "
                    + "gefordert werden; die Entnahmeaktion erschiene nie und niemand "
                    + "wuesste warum.");
            return false;
        }

        return true;
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_ContainerDef s = ChefZ_ContainerDef.Cast(src);
        if (!s)
            return;
        containerCategories = PatchStringArray(containerCategories, s.containerCategories);
        emptyClass          = PatchText(emptyClass, s.emptyClass, s, "emptyClass");
        displayName         = PatchText(displayName, s.displayName, s, "displayName");
        spoilageModifier    = PatchFloat(spoilageModifier, s.spoilageModifier, s, "spoilageModifier");
        searchScope         = PatchInt(searchScope, s.searchScope, s, "searchScope");
        reusable            = PatchBool(reusable, s.reusable, s, "reusable");
        consumedOnServe     = PatchBool(consumedOnServe, s.consumedOnServe, s, "consumedOnServe");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_ContainerDef o = ChefZ_ContainerDef.Cast(other);
        if (!o)
            return;
        if (reusable == o.reusable)
            MarkExplicit("reusable");
        if (consumedOnServe == o.consumedOnServe)
            MarkExplicit("consumedOnServe");
    }

    /**
     * Code-Defaults aus 16 §3.1/§3.2.
     *
     * spoilageModifier wird hier NUR entsentinelt, nicht geklemmt: die
     * Klemmung gehoert zu einer Meldung (16 §7, "auf 0.01 geklemmt, WARN"),
     * und diese Methode hat keinen Ladebericht. Geklemmt und gemeldet wird
     * deshalb in ChefZ_ContainerRegistry.Build() - an genau einer Stelle,
     * mit genau einer Zeile im Startlog.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        // "Eine Schuessel gibt eine Schuessel zurueck" - siehe Feldkommentar.
        emptyClass       = ChefZ_Undefined.TextOr(emptyClass, id);
        spoilageModifier = ChefZ_Undefined.FloatOr(spoilageModifier, DEFAULT_SPOILAGE);
        searchScope      = ChefZ_Undefined.IntOr(searchScope, ChefZ_ContainerScope.DEFAULT);

        if (!HasExplicit("reusable"))
            reusable = true;
        if (!HasExplicit("consumedOnServe"))
            consumedOnServe = true;
    }

    //--------------------------------------------------------------------------

    //! Nennt dieser Record ueberhaupt eine Kategorie? Eine ausdrueckliche
    //! leere Liste ist zulaessig und beantwortet die Frage mit false.
    bool DeclaresCategories()
    {
        return ChefZ_TextList.Count(containerCategories) > 0;
    }

    string ToDebugString()
    {
        string s = id;

        if (containerCategories)
            s = s + " [" + ChefZ_TextList.Join(containerCategories, ",") + "]";
        else
            s = s + " [keine Kategorie]";

        s = s + " leer=" + emptyClass
              + " scope=" + ChefZ_ContainerScope.Name(searchScope);

        if (!reusable)
            s = s + " keineRueckgabe";
        if (!consumedOnServe)
            s = s + " bleibtBeimSpieler";
        if (spoilageModifier != DEFAULT_SPOILAGE)
            s = s + " verderb=" + spoilageModifier.ToString();

        return s;
    }

    //==========================================================================
    // Nur fuer den Selbsttest
    //==========================================================================

    static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // 1. Ohne Kategorieangabe: abgewiesen (siehe Validate).
        ChefZ_ContainerDef stumm = new ChefZ_ContainerDef();
        stumm.id = "CHEFZ_CT_STUMM";
        if (stumm.Validate(ctx))                            return false;

        // 2. Ausdruecklich leere Liste: angenommen, aber ohne Kategorie.
        ChefZ_ContainerDef leer = new ChefZ_ContainerDef();
        leer.id                  = "CHEFZ_CT_LEER";
        leer.containerCategories = new array<string>();
        if (!leer.Validate(ctx))                            return false;
        if (leer.DeclaresCategories())                      return false;

        // 3. Die Vorgaben aus 16 §3.1/§3.2.
        ChefZ_ContainerDef def = new ChefZ_ContainerDef();
        def.id                  = "CHEFZ_CT_SCHALE";
        def.containerCategories = new array<string>();
        def.containerCategories.Insert("  CHEFZ_CT_KAT  ");
        def.Normalize();
        if (def.containerCategories.Get(0) != "CHEFZ_CT_KAT") return false;
        if (!def.DeclaresCategories())                       return false;

        def.ResolveDefaults();
        if (def.emptyClass != "CHEFZ_CT_SCHALE")            return false;   // = id
        if (!def.reusable)                                  return false;
        if (!def.consumedOnServe)                           return false;
        if (def.spoilageModifier != ChefZ_ContainerDef.DEFAULT_SPOILAGE) return false;
        if (def.searchScope != ChefZ_ContainerScope.DEFAULT) return false;

        // 4. Das Bitfeld (16 E5).
        if (!ChefZ_ContainerScope.Has(ChefZ_ContainerScope.DEFAULT,
                                      ChefZ_ContainerScope.HANDS))          return false;
        if (!ChefZ_ContainerScope.Has(ChefZ_ContainerScope.DEFAULT,
                                      ChefZ_ContainerScope.INVENTORY))      return false;
        if (ChefZ_ContainerScope.Has(ChefZ_ContainerScope.DEFAULT,
                                     ChefZ_ContainerScope.NEARBY_CARGO))    return false;
        if (ChefZ_ContainerScope.UnknownBits(ChefZ_ContainerScope.ALL) != 0) return false;
        if (ChefZ_ContainerScope.UnknownBits(8) == 0)                        return false;
        if (ChefZ_ContainerScope.Sanitize(8 | 1) != 1)                       return false;

        // 5. Eine ausdrueckliche emptyClass ueberschreibt die Vorgabe.
        ChefZ_ContainerDef eigen = new ChefZ_ContainerDef();
        eigen.id                  = "CHEFZ_CT_GLAS";
        eigen.containerCategories = new array<string>();
        eigen.containerCategories.Insert("CHEFZ_CT_KAT");
        eigen.emptyClass          = "CHEFZ_CT_LEERGLAS";
        eigen.spoilageModifier    = 0.10;
        eigen.searchScope         = ChefZ_ContainerScope.ALL;
        eigen.ResolveDefaults();
        if (eigen.emptyClass != "CHEFZ_CT_LEERGLAS")        return false;
        if (eigen.spoilageModifier != 0.10)                 return false;
        if (eigen.searchScope != ChefZ_ContainerScope.ALL)  return false;

        // 6. "AUTO" ist eine Regel, kein Klassenname.
        if (ChefZ_ContainerDef.AUTO != "AUTO")              return false;

        return true;
    }
}
