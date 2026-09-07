//==============================================================================
// ChefZ_IRecordSource - die Quellenabstraktion
// ChefZ_SliceManifest / ChefZ_ManifestReader - die Registrierungsflaeche
// ChefZ_PathTools - Pfadnormalisierung
//
// Entwurf: 02 §5.2, 02 §4 (CfgChefZ-Manifest), 02 E1 (Manifest statt Scan),
// 02 E7 (Rueckfall auf config.cpp-Seed).
//
// ---------------------------------------------------------------------------
// ABWEICHUNG VON DER ENTWURFSSCHREIBWEISE, bewusst:
//
// 02 §5.2 schreibt "interface ChefZ_IRecordSource". Enforce Script kennt kein
// interface-Schluesselwort - in den gesamten Vanilla-Skripten (1.29) kommt
// keine einzige interface-Deklaration vor, und die Enforce-Referenz kennt es
// ebenfalls nicht. Umgesetzt wird es deshalb als Basisklasse mit virtuellen
// Methoden. Die Schnittstelle - GetName/GetRank/Read - ist Zeichen fuer
// Zeichen die aus 02 §5.2; nur das Schluesselwort ist ein anderes.
//
// Der Zweck der Abstraktion bleibt vollstaendig erhalten, und er ist konkret:
// faellt der V-A-Rauchtest negativ aus (JSON aus einem Mod-PBO nicht lesbar),
// erzeugt ChefZ_ConfigCppSource denselben Recordstrom aus Klassenbaeumen und
// ChefZ_AddonJsonSource entfaellt - ohne dass Sink, Registry oder Manager
// davon etwas merken (02 E7).
// ---------------------------------------------------------------------------
//
// Layer: 3_Game (JsonFileLoader und g_Game.ConfigGet* liegen hier).
//==============================================================================

class ChefZ_IRecordSource : Managed
{
    //! Name fuer Log und Ladebericht. Erscheint als Quellenangabe.
    string GetName()
    {
        return "?";
    }

    //! ChefZ_SourceRank.*
    int GetRank()
    {
        return ChefZ_SourceRank.UNKNOWN;
    }

    /**
     * Liest die Quelle und reicht jeden Record an den Sink weiter.
     *
     * Rueckgabe false heisst "diese Quelle hat nichts geliefert" - NICHT
     * "Abbruch". Der Manager laeuft in jedem Fall weiter (02 §8: nie werfen,
     * nie abbrechen).
     */
    bool Read(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        return false;
    }

    //! Zaehler fuer die Zusammenfassungszeile aus 02 §8.
    int GetFileCount()
    {
        return 0;
    }
}

//==============================================================================
// Manifest
//==============================================================================

/**
 * Ein Slice, wie er sich in seiner EIGENEN config.cpp anmeldet (02 §4):
 *
 *   class CfgChefZ
 *   {
 *       class ChefZ_Meat
 *       {
 *           chefzApiVersion = 1;
 *           loadOrder       = 200;
 *           dataFiles[]     = { "...json", "...json" };
 *       };
 *   };
 *
 * Das ist die einzige Datei ausserhalb seines Modulordners, die ein
 * Content-Autor je anfassen muss - und sie gehoert ihm.
 */
class ChefZ_SliceManifest : Managed
{
    string name;
    int    apiVersion;
    int    loadOrder;
    ref array<string> dataFiles;

    void ChefZ_SliceManifest()
    {
        name       = "";
        apiVersion = 0;
        loadOrder  = 0;
        dataFiles  = new array<string>();
    }

    string SourceRef()
    {
        return "CfgChefZ " + name;
    }
}

class ChefZ_ManifestReader
{
    //! Wurzelknoten der Registrierungsflaeche. Wird von der Engine ueber alle
    //! geladenen Addons gemerged - genau wie CfgMods (02 §4).
    static const string ROOT = "CfgChefZ";

    //! Hoechste Manifestversion, die dieser Core versteht.
    static const int API_VERSION = 1;

    /**
     * Optionales Manifestfeld: wie viele HANDWERKS-Rezeptplaetze dieser Slice
     * in Vanillas Rezeptliste braucht (02 §4).
     *
     *     class CfgChefZ
     *     {
     *         class ChefZ_Beispiel
     *         {
     *             chefzApiVersion       = 1;
     *             loadOrder             = 200;
     *             handcraftRecipeSlots  = 12;   // <- so viele HANDCRAFT-Transforms
     *             dataFiles[]           = { "..." };
     *         };
     *     };
     *
     * Die Zahl ist eine RESERVIERUNG, keine Obergrenze fuer Transforms
     * ueberhaupt: sie zaehlt ausschliesslich Transforms, deren Prozess
     * exec = "HANDCRAFT" hat. Warum das ueberhaupt deklariert werden muss,
     * steht im Kopf von ChefZ_HandcraftBridge.
     */
    static const string SLOT_FIELD = "handcraftRecipeSlots";

    /**
     * Enumeriert CfgChefZ. Muster woertlich aus
     * 3_Game/DayZ/Client/Mods/ModLoader.c:18-25 (ConfigGetChildrenCount +
     * ConfigGetChildName).
     *
     * Anders als ModLoader beginnt die Schleife bei 0: ModLoader ueberspringt
     * mit "i = 2" die beiden Basiseintraege von CfgMods. CfgChefZ hat keine
     * Basiseintraege - ein Startwert von 2 wuerde die ersten beiden Slices
     * still verschlucken.
     *
     * Sortiert aufsteigend nach loadOrder, bei Gleichstand ordinal nach Name.
     * Der Namens-Tiebreak ist nicht Kosmetik: ohne ihn haengt bei gleicher
     * loadOrder die Merge-Reihenfolge an der Addon-Ladereihenfolge, und
     * "erste gewinnt" (02 §8) waere nicht mehr reproduzierbar.
     */
    static array<ref ChefZ_SliceManifest> ReadAll(ChefZ_LoadReport report)
    {
        array<ref ChefZ_SliceManifest> slices = new array<ref ChefZ_SliceManifest>();

        if (!g_Game)
            return slices;

        if (!g_Game.ConfigIsExisting(ROOT))
            return slices;

        int count = g_Game.ConfigGetChildrenCount(ROOT);
        for (int i = 0; i < count; i++)
        {
            string sliceName;
            if (!g_Game.ConfigGetChildName(ROOT, i, sliceName))
                continue;
            if (sliceName == "")
                continue;

            string node = ROOT + " " + sliceName;

            ChefZ_SliceManifest m = new ChefZ_SliceManifest();
            m.name       = sliceName;
            m.apiVersion = g_Game.ConfigGetInt(node + " chefzApiVersion");
            m.loadOrder  = g_Game.ConfigGetInt(node + " loadOrder");

            TStringArray files = new TStringArray();
            if (g_Game.ConfigIsExisting(node + " dataFiles"))
                g_Game.ConfigGetTextArray(node + " dataFiles", files);
            for (int f = 0; f < files.Count(); f++)
            {
                string path = files.Get(f);
                path.TrimInPlace();
                if (path != "")
                    m.dataFiles.Insert(path);
            }

            CheckApiVersion(m, report);
            slices.Insert(m);
        }

        Sort(slices);
        return slices;
    }

    /**
     * Summe der von allen Slices angemeldeten HANDWERKS-REZEPTPLAETZE.
     *
     * ---------------------------------------------------------------------
     * WARUM DAS HIER STEHT UND NICHT IM CONFIG MANAGER
     * ---------------------------------------------------------------------
     * Diese Zahl wird zu einem Zeitpunkt gebraucht, an dem es noch keinen
     * geladenen Bestand gibt: Vanilla baut seine Rezeptliste im
     * MissionBase-KONSTRUKTOR auf, ChefZ laedt erst in MissionServer.OnInit
     * (siehe Kopf von ChefZ_HandcraftBridge, Abschnitt POSITIONSANKER).
     *
     * Sie darf deshalb NICHTS voraussetzen ausser der Engine-Config - und
     * genau die steht dort bereits vollstaendig: Vanilla selbst laeuft im
     * selben Konstruktor mit g_Game.ConfigGetChildrenCount() ueber
     * CFG_VEHICLESPATH (PluginRecipesManager.GenerateRecipeCache).
     *
     * Kein Report, kein Sink, keine Allokation ausser zwei Zeichenketten je
     * Slice. Fehlt CfgChefZ ganz oder nennt kein Slice das Feld, ist das
     * Ergebnis 0 - und dann reserviert ChefZ keinen einzigen Platz und
     * veraendert Vanillas Rezeptliste um kein Bit.
     *
     * Rang 3 geht hier ABSICHTLICH nicht ein. Ein $profile-Overlay kennt nur
     * der Server; eine daraus abgeleitete Platzzahl waere auf den beiden
     * Seiten verschieden, und die Zahl muss auf beiden Seiten gleich sein -
     * sie ist der Anker (02 §6, 02 E2).
     */
    static int ReadHandcraftSlotTotal()
    {
        if (!g_Game)
            return 0;

        if (!g_Game.ConfigIsExisting(ROOT))
            return 0;

        int total = 0;
        int count = g_Game.ConfigGetChildrenCount(ROOT);

        for (int i = 0; i < count; i++)
        {
            string sliceName;
            if (!g_Game.ConfigGetChildName(ROOT, i, sliceName))
                continue;
            if (sliceName == "")
                continue;

            string node = ROOT + " " + sliceName + " " + SLOT_FIELD;
            if (!g_Game.ConfigIsExisting(node))
                continue;

            int n = g_Game.ConfigGetInt(node);
            if (n <= 0)
                continue;

            total = total + n;
        }

        return total;
    }

    private static void CheckApiVersion(ChefZ_SliceManifest m, ChefZ_LoadReport report)
    {
        if (!report)
            return;

        if (m.apiVersion <= 0)
        {
            report.AddWarn(m.SourceRef(), "", "chefzApiVersion fehlt oder ist 0 - angenommen wird " + API_VERSION.ToString() + ". Der Slice wird geladen; der Eintrag gehoert trotzdem gesetzt.");
            m.apiVersion = API_VERSION;
            return;
        }

        if (m.apiVersion > API_VERSION)
        {
            // Vorwaertskompatibel wie schemaVersion (02 §8): laden, warnen,
            // Unbekanntes ignorieren. Ein blockierender Fehler hiesse, dass
            // ein Content-Update den Core-Betrieb anhaelt.
            report.AddWarn(m.SourceRef(), "", "chefzApiVersion " + m.apiVersion.ToString() + " ist neuer als dieser Core (" + API_VERSION.ToString() + "). Der Slice wird geladen, unbekannte Angaben " + "werden ignoriert.");
        }
    }

    //! Einfuegesortierung, stabil, deterministisch. Slices sind eine
    //! einstellige bis zweistellige Zahl - Laufzeit ist hier kein Thema.
    private static void Sort(array<ref ChefZ_SliceManifest> slices)
    {
        for (int i = 1; i < slices.Count(); i++)
        {
            ChefZ_SliceManifest key = slices.Get(i);
            int j = i - 1;
            while (j >= 0 && IsAfter(slices.Get(j), key))
            {
                slices.Set(j + 1, slices.Get(j));
                j--;
            }
            slices.Set(j + 1, key);
        }
    }

    private static bool IsAfter(ChefZ_SliceManifest a, ChefZ_SliceManifest b)
    {
        if (a.loadOrder != b.loadOrder)
            return a.loadOrder > b.loadOrder;
        return ChefZ_StringOrder.Compare(a.name, b.name) > 0;
    }
}

//==============================================================================
// Pfade
//==============================================================================

/**
 * Pfadnormalisierung fuer Laufzeitpfade in PBOs.
 *
 * Warum ueberhaupt: in ausgelieferten Mods kommen beide Trennzeichen vor
 * ("Fremdmod/.../datei.json" gegen "ChefZ_Core\\Config\\x.json"),
 * und der V-A-Rauchtest misst genau diese Formen (Sonden P4, P5, P7). Solange
 * das Messergebnis nicht vorliegt, waere es falsch, EINE Form zu erzwingen -
 * und genauso falsch, wahllos zu raten.
 *
 * Deshalb: eine kanonische Form (Schraegstrich, kein fuehrender Trenner) und
 * eine kleine, feste Kandidatenliste. Die tatsaechlich tragende Form landet im
 * DEBUG-Log; damit beantwortet der erste Serverstart die Frage, die V-A
 * offengelassen hat, im Betrieb selbst.
 */
class ChefZ_PathTools
{
    //! Schraegstrich als kanonische Form - das ist die Schreibweise der
    //! Vanilla-Aufrufe (CfgGameplayHandler: "dz/worlds/%1/ce/cfggameplay.json").
    static string Normalize(string path)
    {
        string p = path;
        p.TrimInPlace();
        p.Replace("\\", "/");
        while (p.IndexOf("//") >= 0)
            p.Replace("//", "/");
        if (p.Length() > 0 && p.Get(0) == "/")
            p = p.Substring(1, p.Length() - 1);
        return p;
    }

    static string ToBackslash(string path)
    {
        string p = Normalize(path);
        p.Replace("/", "\\");
        return p;
    }

    /**
     * Erste Pfadform, unter der die Datei tatsaechlich existiert.
     * Leerstring, wenn keine passt.
     */
    static string Resolve(string path)
    {
        string canonical = Normalize(path);
        if (canonical == "")
            return "";

        if (FileExist(canonical))
            return canonical;

        string backslash = ToBackslash(canonical);
        if (backslash != canonical && FileExist(backslash))
            return backslash;

        return "";
    }

    //! Alle geprueften Formen als Text - fuer die Fehlermeldung, damit ein
    //! Betreiber sieht, wonach wirklich gesucht wurde.
    static string TriedForms(string path)
    {
        string canonical = Normalize(path);
        string backslash = ToBackslash(canonical);
        if (backslash == canonical)
            return "\"" + canonical + "\"";
        return "\"" + canonical + "\" und \"" + backslash + "\"";
    }

    //! Dateiname ohne Verzeichnis. Fuer kurze Logzeilen.
    static string FileName(string path)
    {
        string p = Normalize(path);
        int slash = p.LastIndexOf("/");
        if (slash < 0)
            return p;
        return p.Substring(slash + 1, p.Length() - slash - 1);
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        if (Normalize("A\\B\\c.json") != "A/B/c.json")      return false;
        if (Normalize("/A/B/c.json") != "A/B/c.json")       return false;
        if (Normalize("A//B/c.json") != "A/B/c.json")       return false;
        if (Normalize("  A/B/c.json  ") != "A/B/c.json")    return false;
        if (ToBackslash("A/B/c.json") != "A\\B\\c.json")    return false;
        if (FileName("A/B/c.json") != "c.json")             return false;
        if (FileName("c.json") != "c.json")                 return false;
        if (Resolve("") != "")                              return false;
        return true;
    }
}
