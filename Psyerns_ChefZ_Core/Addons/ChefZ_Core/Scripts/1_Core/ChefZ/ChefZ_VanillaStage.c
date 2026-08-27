//==============================================================================
// ChefZ_VanillaStage - Name <-> Zahl der Vanilla-Garstufe, OHNE den Engine-Enum
//
// Entwurf: 07 §2.1 ("vanillaStage: Raw|Baked|Boiled|Dried|Burned|Rotten"),
// 07 §5 ("VANILLA_STAGE: vanillaFoodStage == value"), 06 §3 (dieselbe
// Schreibweise projiziert der Zustandsdef auf die Vanilla-Stufe), 01 V4.
//
// Warum diese Datei ueberhaupt existiert:
//
//   ChefZ_ItemFacts.vanillaFoodStage ist bewusst ein int und kein
//   FoodStageType - der Enum lebt in 4_World, und 1_Core darf ihn nicht
//   kennen (00 §4). Ein Selektor nennt aber einen NAMEN. Die Umsetzung
//   Name -> Zahl muss also in 1_Core stattfinden, und zwar gegen eine
//   nachgeschriebene Werteliste.
//
// Die Zahlen sind woertlich aus
//   scripts - 1.29/4_World/DayZ/Classes/FoodStage/FoodStage.c:1
//   enum FoodStageType { NONE=0, RAW=1, BAKED=2, BOILED=3, DRIED=4,
//                        BURNED=5, ROTTEN=6, COUNT }
// uebernommen. Sie sind stabil, weil sie netzsynchronisiert sind
// (01 V4: RegisterNetSyncVariableInt ueber m_FoodStageType) - eine Aenderung
// waere ein Bruch im Vanilla-Netzprotokoll und findet nicht statt.
//
// Der ChefZ_FactCollector (4_World) schreibt GetFoodStageType() unveraendert
// in die Fakten; hier wird nur derselbe Zahlenraum benannt. Es gibt keinen
// zweiten Wahrheitsort: wer die Liste aendert, aendert sie falsch.
//
// KEIN CONTENT: die sechs Namen sind Vanilla-Vokabular, kein ChefZ-Zustand.
// ChefZ-Zustaende ("SMOKED", "SALTED", ...) sind Daten und stehen nirgends im
// Code.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_VanillaStage
{
    static const int NONE   = 0;
    static const int RAW    = 1;
    static const int BAKED  = 2;
    static const int BOILED = 3;
    static const int DRIED  = 4;
    static const int BURNED = 5;
    static const int ROTTEN = 6;

    //! Erster Wert oberhalb des gueltigen Bereichs (FoodStageType.COUNT).
    static const int COUNT  = 7;

    /**
     * Name -> Zahl. -1, wenn der Name keiner Vanilla-Stufe entspricht.
     *
     * Gross-/Kleinschreibung wird eingeebnet. Das ist KEIN Widerspruch zu
     * 03 E5 (dort geht es um IDs eines Namensraums, den ChefZ selbst fuehrt):
     * hier ist die Wertemenge geschlossen, engine-gegeben und nicht
     * erweiterbar - "Boiled", "BOILED" und "boiled" koennen unmoeglich drei
     * verschiedene Dinge meinen. Nachsichtig zu sein kostet hier nichts und
     * erspart einen Rezeptfehler, den niemand sieht.
     */
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == "")        return -1;
        if (n == "NONE")    return NONE;
        if (n == "RAW")     return RAW;
        if (n == "BAKED")   return BAKED;
        if (n == "BOILED")  return BOILED;
        if (n == "DRIED")   return DRIED;
        if (n == "BURNED")  return BURNED;
        if (n == "BURNT")   return BURNED;   // haeufiger Schreibfehler, gleiche Absicht
        if (n == "ROTTEN")  return ROTTEN;
        return -1;
    }

    //! Zahl -> Name in der Schreibweise des Entwurfs. Leerstring bei Unsinn.
    static string Name(int stage)
    {
        switch (stage)
        {
            case NONE:   return "None";
            case RAW:    return "Raw";
            case BAKED:  return "Baked";
            case BOILED: return "Boiled";
            case DRIED:  return "Dried";
            case BURNED: return "Burned";
            case ROTTEN: return "Rotten";
        }
        return "";
    }

    static bool IsValid(int stage)
    {
        return stage >= NONE && stage < COUNT;
    }

    /**
     * Die ChefZ-Schreibweise derselben Stufe - Schritt 3 der Projektionsregel
     * (06 §3), woertlich:
     *
     *     RAW->RAW, BAKED->BAKED, BOILED->BOILED, DRIED->DRIED,
     *     BURNED->BURNT, ROTTEN->ROTTEN
     *
     * Nur "Burned"/"BURNT" weicht ab; das ist keine Nachlaessigkeit des
     * Entwurfs, sondern seine Schreibweise (siehe auch CoreSettings
     * defaultExcludedStates: "BURNT", "ROTTEN").
     *
     * KEIN CONTENT (Invariante I3), aus demselben Grund, aus dem der Core den
     * Seed defaultExcludedStates nennen darf: hier wird kein Zustand ANGELEGT.
     * Es steht nur da, unter welcher ID der ChefZ_StateManager nachschlaegt,
     * WENN ein Content-Modul die Vanilla-Aequivalente ueberhaupt deklariert
     * hat. Tut es das nicht, laeuft die Zeile ins Leere und Schritt 3 der
     * Projektionsregel faellt auf die Rueckwaertssuche zurueck.
     *
     * Leerstring fuer NONE und alles ausserhalb.
     */
    static string ChefZStateId(int stage)
    {
        switch (stage)
        {
            case RAW:    return "RAW";
            case BAKED:  return "BAKED";
            case BOILED: return "BOILED";
            case DRIED:  return "DRIED";
            case BURNED: return "BURNT";
            case ROTTEN: return "ROTTEN";
        }
        return "";
    }

    static string ValidNames()
    {
        return "None, Raw, Baked, Boiled, Dried, Burned, Rotten";
    }

    //! Nur fuer den Selbsttest (S5).
    static bool SelfCheck()
    {
        if (FromName("Raw") != RAW)             return false;
        if (FromName("  boiled ") != BOILED)    return false;
        if (FromName("ROTTEN") != ROTTEN)       return false;
        if (FromName("Burnt") != BURNED)        return false;
        if (FromName("") != -1)                 return false;
        if (FromName("Gebraten") != -1)         return false;
        if (Name(BAKED) != "Baked")             return false;
        if (Name(99) != "")                     return false;
        if (!IsValid(NONE))                     return false;
        if (IsValid(COUNT))                     return false;
        if (IsValid(-1))                        return false;

        // 06 §3, Schritt 3: die Zuordnung Vanilla-Stufe -> ChefZ-ID.
        if (ChefZStateId(RAW)    != "RAW")      return false;
        if (ChefZStateId(BURNED) != "BURNT")    return false;
        if (ChefZStateId(ROTTEN) != "ROTTEN")   return false;
        if (ChefZStateId(NONE)   != "")         return false;
        if (ChefZStateId(99)     != "")         return false;

        // Beide Richtungen muessen sich treffen: was ChefZStateId liefert,
        // muss FromName zurueck auf dieselbe Stufe bringen. Sonst faende
        // Schritt 3 einen Zustand, der auf etwas anderes projiziert.
        for (int st = RAW; st <= ROTTEN; st++)
        {
            if (FromName(ChefZStateId(st)) != st)
                return false;
        }
        return true;
    }
}
