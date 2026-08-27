//==============================================================================
// V-A - Rauchtest: JSON aus einem Mod-PBO lesen
//
// TEMPORAER. Dieser Ordner (Tests/V_A_PboJsonSmoke/) und der zugehoerige
// files[]-Eintrag in config.cpp werden geloescht, sobald das Ergebnis im
// Gate-1-Protokoll steht. Er ist kein Bestandteil des Core.
//
// Frage (Entwurf 01 V8, 02 E7, OF-10):
//   Belegt ist nur, dass FileExist/JsonFileLoader auf Pfade INNERHALB der
//   Spiel-PBOs (dz/...) funktionieren - CfgGameplayHandler.LoadData() tut
//   genau das. Unbelegt ist der Fall MOD-PBO. Faellt der Test negativ aus,
//   wird Rang 2 nicht als Addon-JSON, sondern als config.cpp-Seed gebaut
//   (ChefZ_ConfigCppSource statt ChefZ_AddonJsonSource). Die Abstraktion
//   ChefZ_IRecordSource macht beides architektur-neutral.
//
// Zweiter Vanilla-Beleg, in 01 V8 noch nicht genannt:
//   3_Game/DayZ/Client/Notifications/NotificationSystem.c:74,285
//     protected static const string JSON_FILE_PATH = "scripts/data/notifications.json";
//     JsonFileLoader<map<NotificationType, NotificationData>>.LoadFile(JSON_FILE_PATH, ...)
//   Das ist ein Pfad OHNE "dz/"-Wurzel, aufgeloest gegen das Praefix des
//   scripts-PBO. Zusammen mit CfgGameplayHandler (Wurzel "dz/worlds/...",
//   Praefix der Welt-PBOs) ergibt sich dieselbe Regel: die Wurzel eines
//   Laufzeitpfades ist das PBO-Praefix. Genau das prueft P1 gegen ein MOD-PBO.
//
// I4-BELEG: die folgenden drei Mods werden ZITIERT, nicht benutzt. Der Core ruft
// nichts von ihnen auf, kennt keine ihrer Klassen und haengt an keiner. Sie
// stehen hier als Messbeleg zu 01 V8 - und der Marker sagt chefzcore.mjs, dass
// das geprueft und gewollt ist (I4).
//
// Belege aus fremden, ausgelieferten Mods (Quellrepos, nur gelesen). Alle drei
// loesen einen Laufzeitpfad gegen das PBO-Praefix ihres EIGENEN Mod-PBO auf:
//   DayZExpansion/NamalskAdventure  LoadingScreen.c:37
//     JsonFileLoader<...>.JsonLoadFile("DayZExpansion/NamalskAdventure/Scripts/Data/LoadingImages.json", ...)
//     -> JSON aus einem Mod-PBO. Das ist exakt der Rang-2-Fall von ChefZ.
//   LBmaster_Core  DayZGame.c:106   ($prefix$ = "LBmaster_Core")
//     FileExist("LBmaster_Core/version/scripts/lb_version_check.c")
//   NY_RadiationMod  missiongameplayrad.c:23
//     FileExist("NY_RadiationMod/Video/Rad.mp4")
// Das ist starke Evidenz, aber kein Beweis auf unserem Zielbuild - deshalb
// dieser Test. Es ersetzt keine Messung, es sagt nur, welches Ergebnis
// erwartet wird: P1 PASS, P2 FAIL.
//
// Der Test veraendert nichts. Er liest, zaehlt und schreibt ins RPT.
// Vanilla-Kochen wird nicht beruehrt.
//
// Auswertung: siehe Tests/V_A_PboJsonSmoke/README.md
//==============================================================================

//! Zieltyp fuer JsonFileLoader. Absichtlich winzig und mit einem Marker,
//! damit "LoadFile hat true geliefert" von "es kamen wirklich Daten an"
//! unterscheidbar ist.
class ChefZ_ProbeDoc
{
    int                 schemaVersion;
    string              marker;
    ref array<string>   items;
}

class ChefZ_PboProbe
{
    static const string TAG      = "[ChefZ][V-A]";
    static const int    READ_LEN = 65536;

    static bool s_RanServer;
    static bool s_RanClient;

    //--------------------------------------------------------------------------
    static void RunServer()
    {
        if (s_RanServer)
            return;
        s_RanServer = true;
        RunAll("SERVER");
    }

    static void RunClient()
    {
        if (s_RanClient)
            return;
        s_RanClient = true;
        RunAll("CLIENT");
    }

    //--------------------------------------------------------------------------
    private static void RunAll(string side)
    {
        PrintToRPT(TAG + " ===== Rauchtest PBO-JSON  seite=" + side + " =====");
        PrintToRPT(TAG + " Praefix laut $PREFIX$ = ChefZ_Core");

        // P1  Praefixwurzel == Ordnername. Das ist die Form, die README
        //     "Packing - PBO Prefix" vorschreibt.
        Probe("P1 prefix-root      ", "ChefZ_Core/Config/ChefZ_ProbeData.json", "CHEFZ_V_A_OK");

        // P2  GEGENPROBE zur entschiedenen Pfadwurzel (B4, 02 Paragraph 4.1).
        //     Verbindlich gilt: die Wurzel ist das PBO-Praefix, und das ist der
        //     Ordnername des Addons. P2 setzt stattdessen eine Wurzel voraus,
        //     die es nach dieser Entscheidung nicht gibt.
        //
        //     Diese Sonde MISST die Frage also nicht mehr, sie FALSIFIZIERT die
        //     Antwort: erwartet ist "Pfad unbekannt". Meldet P2 "gelesen", ist
        //     die Entscheidung falsch und gehoert zurueckgenommen - dann traegt
        //     die Engine beide Wurzeln, und der Entwurf muss EINE davon
        //     ausdruecklich verbieten.
        Probe("P2 gegenprobe wurzel", "Psyerns/ChefZ_Core/Config/ChefZ_ProbeData.json", "CHEFZ_V_A_OK");

        // P3  Unterverzeichnis - beantwortet, ob verschachtelte Datenpfade
        //     ("Config/Recipes/Sausage.json") tragen.
        Probe("P3 nested dir       ", "ChefZ_Core/Config/Probe/ChefZ_ProbeNested.json", "CHEFZ_V_A_NESTED");

        // P4  Gross-/Kleinschreibung. Entscheidet, ob der Validator Pfade
        //     case-sensitiv pruefen muss.
        Probe("P4 lowercase        ", "chefz_core/config/chefz_probedata.json", "CHEFZ_V_A_OK");

        // P5  Fuehrender Schraegstrich.
        Probe("P5 leading slash    ", "/ChefZ_Core/Config/ChefZ_ProbeData.json", "CHEFZ_V_A_OK");

        // P7  Rueckwaertsschraegstrich. In freier Wildbahn kommen beide Formen
        //     vor; der Config Manager normalisiert spaeter auf eine davon.
        Probe("P7 backslash        ", "ChefZ_Core\\Config\\ChefZ_ProbeData.json", "CHEFZ_V_A_OK");

        // P6  Kontrolle: der belegte dz/-Fall. Schlaegt P6 fehl, ist die
        //     Messung selbst kaputt und P1-P5 sind wertlos.
        string world;
        GetGame().GetWorldName(world);
        string vanillaPath = string.Format("dz/worlds/%1/ce/cfggameplay.json", world);
        ProbeRawOnly("P6 kontrolle dz/    ", vanillaPath);

        // E1  Gegenprobe zu 01 V8: PBO-Inhalte sind nicht aufzaehlbar.
        //     Erwartet: kein Treffer. Ein Treffer waere eine gute Nachricht
        //     und gehoert genauso ins Protokoll.
        Enumerate("E1 FindFile         ", "ChefZ_Core/Config/*.json");

        PrintToRPT(TAG + " ===== Ende  seite=" + side + " =====");
    }

    //--------------------------------------------------------------------------
    //! Voller Durchlauf: FileExist -> Rohlesen -> typisiertes LoadFile.
    //! Die drei Stufen trennen "Pfad unbekannt" von "nicht lesbar" von
    //! "gelesen, aber nicht deserialisierbar".
    private static void Probe(string label, string path, string expectedMarker)
    {
        bool fileExists = FileExist(path);
        int  rawBytes   = RawRead(path);

        string errorMessage;
        ChefZ_ProbeDoc doc = new ChefZ_ProbeDoc();
        bool loaded = JsonFileLoader<ChefZ_ProbeDoc>.LoadFile(path, doc, errorMessage);

        string markerSeen = "-";
        int    itemCount  = -1;
        if (doc)
        {
            markerSeen = doc.marker;
            if (doc.items)
                itemCount = doc.items.Count();
        }

        bool ok = fileExists && loaded && (markerSeen == expectedMarker) && (itemCount > 0);

        PrintToRPT(string.Format("%1 %2 exists=%3 rawChars=%4 load=%5 marker=%6 items=%7 verdict=%8",
            TAG, label, fileExists, rawBytes, loaded, markerSeen, itemCount, Verdict(ok)));
        PrintToRPT(string.Format("%1 %2   path=%3", TAG, label, path));
        if (!loaded && errorMessage != "")
            PrintToRPT(string.Format("%1 %2   loaderMsg=%3", TAG, label, errorMessage));
    }

    //--------------------------------------------------------------------------
    //! Kontrollpfad: nur Existenz und Rohlesen. Der Inhalt gehoert Vanilla und
    //! wird bewusst nicht in einen ChefZ-Typ deserialisiert.
    private static void ProbeRawOnly(string label, string path)
    {
        bool fileExists = FileExist(path);
        int  rawBytes   = RawRead(path);
        bool ok = fileExists && rawBytes > 0;

        PrintToRPT(string.Format("%1 %2 exists=%3 rawChars=%4 verdict=%5",
            TAG, label, fileExists, rawBytes, Verdict(ok)));
        PrintToRPT(string.Format("%1 %2   path=%3", TAG, label, path));
    }

    //--------------------------------------------------------------------------
    //! Liefert die gelesene Zeichenzahl, -1 wenn die Datei nicht zu oeffnen ist.
    private static int RawRead(string path)
    {
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
            return -1;

        string content;
        ReadFile(handle, content, READ_LEN);
        CloseFile(handle);
        return content.Length();
    }

    //--------------------------------------------------------------------------
    //! FindFile ueber alle drei Flags. 01 V8 sagt: PBOs sind weder
    //! FS-Verzeichnis noch .pak, also darf hier nichts gefunden werden.
    private static void Enumerate(string label, string pattern)
    {
        EnumerateWithFlag(label, pattern, FindFileFlags.DIRECTORIES, "DIRECTORIES");
        EnumerateWithFlag(label, pattern, FindFileFlags.ARCHIVES,    "ARCHIVES");
        EnumerateWithFlag(label, pattern, FindFileFlags.ALL,         "ALL");
    }

    private static void EnumerateWithFlag(string label, string pattern, FindFileFlags flag, string flagName)
    {
        // Aufrufmuster woertlich aus 5_Mission/DayZ/GUI/NewUI/VideoPlayer.c:81:
        // kein Handle-Nullcheck (FindFileHandle ist typedef int[]), erst
        // fileName pruefen, dann FindNextFile, am Ende immer CloseFindFile.
        string   fileName;
        FileAttr fileAttr;
        int      found = 0;
        string   first = "-";

        FindFileHandle handle = FindFile(pattern, fileName, fileAttr, flag);

        if (fileName != "")
        {
            first = fileName;
            found++;
        }

        while (found < 32 && FindNextFile(handle, fileName, fileAttr))
        {
            if (found == 0)
                first = fileName;
            found++;
        }

        CloseFindFile(handle);

        PrintToRPT(string.Format("%1 %2 flag=%3 hits=%4 first=%5 pattern=%6",
            TAG, label, flagName, found, first, pattern));
    }

    //--------------------------------------------------------------------------
    private static string Verdict(bool ok)
    {
        if (ok)
            return "PASS";
        return "FAIL";
    }
}

// KEIN eigener Mission-Hook mehr.
//
// Seit S1 gibt es mit Scripts/5_Mission/ChefZ/ChefZ_CoreEntry.c genau einen
// Einstiegspunkt je Seite. Ein zweites Paar "modded class MissionServer /
// MissionGameplay" im selben Modul waere zulaessig, aber unnoetige
// Kollisionsflaeche gegenueber anderen Mods - und die Regel lautet: modded
// class sparsam und gezielt.
//
// RunServer() und RunClient() werden dort aus einem klar markierten
// TEMPORAER-Block gerufen. Rang 2 wird laut 02 §3 von Client UND Server
// gelesen, deshalb misst der Test weiterhin beide Seiten.
//
// Beim Entfernen dieses Testordners: die beiden Aufrufe in ChefZ_CoreEntry.c
// und den files[]-Eintrag in config.cpp mit loeschen.
