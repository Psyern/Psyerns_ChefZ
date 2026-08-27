//==============================================================================
// ChefZ_TerjeSkillsConfig - jede Zahl dieses Moduls kommt aus der Config
//
// Es gibt in diesem PBO keine fest verdrahtete XP-Zahl und keine fest
// verdrahtete Kraeuterklasse. Alles steht in config.cpp unter
// CfgChefZTerjeSkills und wird hier gelesen.
//
// ---------------------------------------------------------------------------
// Warum GetTerjeGameConfig() und nicht GetGame()
// ---------------------------------------------------------------------------
// TerjeCore/Scripts/3_Game/TerjeGameConfig.c: die Klasse liest zuerst aus
// $profile:TerjeSettings\Core\GameOverrides.xml und faellt erst dann auf
// GetGame().ConfigGet*() zurueck. Damit kann ein Betreiber JEDEN Wert dieses
// Moduls veraendern, ohne ein PBO neu zu bauen - dieselbe Stellschraube, die
// er fuer Terje selbst schon kennt. Der Fallback bedeutet zugleich, dass die
// Zahlen auf dem CLIENT (wo TerjeGameConfig ohne empfangene Serverdaten leer
// ist) trotzdem aus der config.cpp kommen; das ist genau richtig, denn
// clientseitig wird hier nur die Hervorhebungs-Reichweite gebraucht.
//
// ---------------------------------------------------------------------------
// Zwischenspeicher
// ---------------------------------------------------------------------------
// Die Skalare werden einmal beim Missionsstart gelesen. Die Nachschlage-
// tabellen (Prozess-ID -> XP) werden bei Bedarf gefuellt und danach behalten:
// eine Config-Abfrage im Abschlusspfad des Kochens waere Arbeit fuer etwas,
// das sich zur Laufzeit nicht aendern kann (Config-Reload gibt es in V1
// nicht, OF-16).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_TerjeSkillsConfig
{
    private static const string ROOT = "CfgChefZTerjeSkills";

    private static bool s_Loaded;

    //--- Hauptschalter --------------------------------------------------------
    private static bool  s_Enabled;
    private static bool  s_XpEnabled;
    private static string s_Skill;
    private static bool  s_ShowNotification;

    //--- Mengenbonus ----------------------------------------------------------
    private static int   s_BatchBonusPerUnit;
    private static int   s_BatchMaxUnits;
    private static int   s_BatchCapPercent;

    //--- Wiederholungsdaempfung ----------------------------------------------
    private static int   s_RepeatFreeCount;
    private static float s_RepeatWindowSec;
    private static int   s_RepeatStepPercent;
    private static int   s_RepeatMinPercent;

    //--- Kochen ---------------------------------------------------------------
    private static int   s_CookSimpleXp;
    private static int   s_CookComplexXp;
    private static int   s_CookPremiumXp;
    private static int   s_CookSimpleMax;
    private static int   s_CookComplexMax;

    //--- Verarbeitung ---------------------------------------------------------
    private static int   s_ProcessDefaultXp;

    //--- Ernte ----------------------------------------------------------------
    private static int   s_HarvestDefaultXp;
    private static ref array<string> s_HarvestTags;

    //--- Kraeuterkundiger -----------------------------------------------------
    private static string s_HerbTag;
    private static bool   s_HighlightEnabled;
    private static bool   s_HighlightUnripe;
    private static bool   s_YieldEnabled;
    private static ref array<float> s_HighlightRange;

    //--- Faehigkeiten ---------------------------------------------------------
    private static bool  s_CapEnabled;
    private static int   s_CapPriority;

    //--- Nachschlagetabellen --------------------------------------------------
    private static ref map<string, int> s_Lookup;

    //==========================================================================
    // Laden
    //==========================================================================

    /**
     * Einmal je Prozess. Ein zweiter Aufruf ist folgenlos.
     *
     * Bewusst OHNE Fehlerpfad: fehlt ein Feld, liefert die Terje-Config 0, und
     * die Vorgabe daneben faengt das ab. Ein Modul, das wegen eines fehlenden
     * Config-Feldes den Start abbricht, waere schlechter als eines, das mit
     * seinen Vorgaben weiterlaeuft.
     */
    static void Load()
    {
        if (s_Loaded)
            return;
        s_Loaded = true;

        s_Lookup = new map<string, int>();
        s_HarvestTags = new array<string>();
        s_HighlightRange = new array<float>();

        s_Enabled          = GetBool(ROOT + " enabled", true);

        string xp = ROOT + " ChefZ_Xp";
        s_XpEnabled        = GetBool(xp + " enabled", true);
        s_Skill            = GetText(xp + " skill", ChefZ_TerjeSkillsBridge.SKILL_SURVIVAL);
        s_ShowNotification = GetBool(xp + " showNotification", true);

        s_BatchBonusPerUnit = GetInt(xp + " batchBonusPerUnit", 1, 0, 100);
        s_BatchMaxUnits     = GetInt(xp + " batchMaxUnits",     3, 0, 100);
        s_BatchCapPercent   = GetInt(xp + " batchCapPercent",  50, 0, 100);

        s_RepeatFreeCount   = GetInt(xp + " repeatFreeCount",   5, 0, 1000);
        s_RepeatWindowSec   = GetInt(xp + " repeatWindowSec", 900, 0, 86400);
        s_RepeatStepPercent = GetInt(xp + " repeatStepPercent", 25, 0, 100);
        s_RepeatMinPercent  = GetInt(xp + " repeatMinPercent",  25, 0, 100);

        string cook = xp + " ChefZ_Cook";
        s_CookSimpleXp   = GetInt(cook + " simpleXp",   3, 0, 10000);
        s_CookComplexXp  = GetInt(cook + " complexXp",  8, 0, 10000);
        s_CookPremiumXp  = GetInt(cook + " premiumXp", 15, 0, 10000);
        s_CookSimpleMax  = GetInt(cook + " simpleMaxInputs",  2, 0, 64);
        s_CookComplexMax = GetInt(cook + " complexMaxInputs", 5, 0, 64);

        s_ProcessDefaultXp = GetInt(xp + " ChefZ_Process defaultXp", 1, 0, 10000);

        string harv = xp + " ChefZ_Harvest";
        s_HarvestDefaultXp = GetInt(harv + " defaultXp", 2, 0, 10000);

        // Ueber eine LOKALE Liste und dann umkopiert. ConfigGetTextArrayRaw
        // nimmt einen out-Parameter und legt die Liste notfalls selbst an
        // (TerjeGameConfig.c) - ein statisches ref-Feld direkt hineinzureichen
        // waere ein Besitzwechsel, den man sich sparen kann.
        TStringArray rawTags = new TStringArray();
        GetTerjeGameConfig().ConfigGetTextArrayRaw(harv + " harvestTags", rawTags);
        for (int t = 0; t < rawTags.Count(); t++)
        {
            string tag = rawTags.Get(t);
            tag.TrimInPlace();
            if (tag != "")
                s_HarvestTags.Insert(tag);
        }
        if (s_HarvestTags.Count() == 0)
        {
            // Vorgabe, falls die Liste fehlt oder leer ueberschrieben wurde.
            s_HarvestTags.Insert("CHEFZ_HERB");
        }

        string herb = ROOT + " ChefZ_Herb";
        s_HerbTag          = GetText(herb + " tag", "CHEFZ_HERB");
        s_HighlightEnabled = GetBool(herb + " highlightEnabled", true);
        s_HighlightUnripe  = GetBool(herb + " highlightUnripe", false);
        s_YieldEnabled     = GetBool(herb + " yieldEnabled", true);

        TFloatArray rawRange = new TFloatArray();
        GetTerjeGameConfig().ConfigGetFloatArray(herb + " highlightRange", rawRange);
        for (int r = 0; r < rawRange.Count(); r++)
            s_HighlightRange.Insert(rawRange.Get(r));

        string cap = ROOT + " ChefZ_Capabilities";
        s_CapEnabled  = GetBool(cap + " enabled", true);
        s_CapPriority = GetInt(cap + " priority", 100, -10000, 10000);
    }

    //==========================================================================
    // Abfragen - Skalare
    //==========================================================================

    static bool  IsEnabled()          { Load(); return s_Enabled; }
    static bool  IsXpEnabled()        { Load(); return s_Enabled && s_XpEnabled; }
    static string SkillId()           { Load(); return s_Skill; }
    static bool  ShowNotification()   { Load(); return s_ShowNotification; }

    static int   BatchBonusPerUnit()  { Load(); return s_BatchBonusPerUnit; }
    static int   BatchMaxUnits()      { Load(); return s_BatchMaxUnits; }
    static int   BatchCapPercent()    { Load(); return s_BatchCapPercent; }

    static int   RepeatFreeCount()    { Load(); return s_RepeatFreeCount; }
    static float RepeatWindowSec()    { Load(); return s_RepeatWindowSec; }
    static int   RepeatStepPercent()  { Load(); return s_RepeatStepPercent; }
    static int   RepeatMinPercent()   { Load(); return s_RepeatMinPercent; }

    static int   CookSimpleXp()       { Load(); return s_CookSimpleXp; }
    static int   CookComplexXp()      { Load(); return s_CookComplexXp; }
    static int   CookPremiumXp()      { Load(); return s_CookPremiumXp; }
    static int   CookSimpleMax()      { Load(); return s_CookSimpleMax; }
    static int   CookComplexMax()     { Load(); return s_CookComplexMax; }

    static int   ProcessDefaultXp()   { Load(); return s_ProcessDefaultXp; }
    static int   HarvestDefaultXp()   { Load(); return s_HarvestDefaultXp; }

    static string HerbTag()           { Load(); return s_HerbTag; }
    static bool  HighlightEnabled()   { Load(); return s_Enabled && s_HighlightEnabled; }
    static bool  HighlightUnripe()    { Load(); return s_HighlightUnripe; }
    static bool  YieldEnabled()       { Load(); return s_Enabled && s_YieldEnabled; }

    static bool  CapabilitiesEnabled(){ Load(); return s_Enabled && s_CapEnabled; }
    static int   CapabilityPriority() { Load(); return s_CapPriority; }

    /**
     * Tags, die eine Ernte ueberhaupt survival-relevant machen.
     *
     * Die Liste selbst wird herausgegeben, nicht eine Kopie - sie ist nach
     * Load() unveraenderlich, und der einzige Leser ist der Erntepfad.
     */
    static array<string> HarvestTags()
    {
        Load();
        return s_HarvestTags;
    }

    /**
     * Hervorhebungsreichweite fuer eine Perkstufe (1-basiert wie bei Terje).
     *
     * Fehlt die Liste oder ist sie zu kurz, gilt der letzte vorhandene Wert;
     * fehlt sie ganz, 12 m. Eine Reichweite von 0 waere ein unsichtbarer
     * Perk - das waere der schlechtere Ausfallwert.
     */
    static float HighlightRange(int perkLevel)
    {
        Load();

        if (perkLevel <= 0)
            return 0.0;
        if (s_HighlightRange.Count() == 0)
            return 12.0;

        int idx = perkLevel - 1;
        if (idx >= s_HighlightRange.Count())
            idx = s_HighlightRange.Count() - 1;

        float r = s_HighlightRange.Get(idx);
        if (r <= 0.0)
            return 12.0;

        return r;
    }

    //==========================================================================
    // Abfragen - Tabellen
    //==========================================================================

    static int CookRecipeXp(string recipeId, int fallback)
    {
        return LookupInt(ROOT + " ChefZ_Xp ChefZ_Cook ChefZ_Recipes", recipeId, fallback);
    }

    static int ProcessXp(string processId, int fallback)
    {
        return LookupInt(ROOT + " ChefZ_Xp ChefZ_Process ChefZ_Processes", processId, fallback);
    }

    static int TransformXp(string transformId, int fallback)
    {
        return LookupInt(ROOT + " ChefZ_Xp ChefZ_Process ChefZ_Transforms", transformId, fallback);
    }

    static int HarvestClassXp(string className, int fallback)
    {
        return LookupInt(ROOT + " ChefZ_Xp ChefZ_Harvest ChefZ_Classes", className, fallback);
    }

    /**
     * Ein Feld aus einer Tabelle, mit Ausfallwert.
     *
     * ConfigIsExisting() ZUERST, weil ConfigGetInt() fuer einen fehlenden
     * Pfad 0 liefert und 0 hier ein gueltiger, absichtlicher Wert ist
     * (PROCESS_CUT_OUT_SEEDS = 0 schliesst eine XP-Schleife). Ohne die
     * Existenzpruefung waere "gibt es nicht" von "soll null geben" nicht zu
     * unterscheiden.
     *
     * Der Zwischenspeicher merkt sich auch das NICHT-Vorhandensein, sonst
     * fragte jeder Kochvorgang dieselbe fehlende Zeile erneut ab. Der
     * Ausfallwert wird deshalb NICHT mitgespeichert: gespeichert wird nur,
     * was in der Config steht.
     */
    private static int LookupInt(string basePath, string key, int fallback)
    {
        Load();

        if (key == "")
            return fallback;

        string full = basePath + " " + key;

        int cached;
        if (s_Lookup.Find(full, cached))
        {
            if (cached == int.MIN)
                return fallback;
            return cached;
        }

        int value = int.MIN;
        if (GetTerjeGameConfig().ConfigIsExisting(full))
            value = GetTerjeGameConfig().ConfigGetInt(full);

        // Der Zwischenspeicher wird nicht unbegrenzt gross: die Schluessel
        // sind Rezept-, Prozess-, Transform- und Klassennamen, also eine
        // durch die Configs feste Menge.
        s_Lookup.Set(full, value);

        if (value == int.MIN)
            return fallback;

        return value;
    }

    //==========================================================================
    // Kleine Leser
    //==========================================================================

    private static bool GetBool(string path, bool fallback)
    {
        if (!GetTerjeGameConfig().ConfigIsExisting(path))
            return fallback;
        return GetTerjeGameConfig().ConfigGetInt(path) == 1;
    }

    private static int GetInt(string path, int fallback, int minValue, int maxValue)
    {
        int v = fallback;
        if (GetTerjeGameConfig().ConfigIsExisting(path))
            v = GetTerjeGameConfig().ConfigGetInt(path);

        // Geklemmt, weil der Wert aus GameOverrides.xml kommen kann. Ein
        // Tippfehler des Betreibers soll eine schlechte Balance ergeben, keine
        // kaputte Runde.
        if (v < minValue)
            v = minValue;
        if (v > maxValue)
            v = maxValue;

        return v;
    }

    private static string GetText(string path, string fallback)
    {
        string v;
        if (!GetTerjeGameConfig().ConfigGetTextRaw(path, v))
            return fallback;
        v.TrimInPlace();
        if (v == "")
            return fallback;
        return v;
    }

    //! Nur fuer die Startmeldung im RPT.
    static string Summary()
    {
        Load();
        return "aktiv=" + s_Enabled.ToString()
             + " xp=" + s_XpEnabled.ToString()
             + " skill=" + s_Skill
             + " kraut=" + s_HerbTag
             + " hervorheben=" + s_HighlightEnabled.ToString()
             + " ausbeute=" + s_YieldEnabled.ToString()
             + " faehigkeiten=" + s_CapEnabled.ToString();
    }
}
