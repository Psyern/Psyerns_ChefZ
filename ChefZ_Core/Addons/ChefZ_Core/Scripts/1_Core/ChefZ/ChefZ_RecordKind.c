//==============================================================================
// ChefZ_RecordKind - die Liste der Record-ARTEN
//
// Entwurf: 02 §5.1 ("GetKindName() ist der EINZIGE Ort, an dem der Core eine
// feste Liste fuehrt - die Liste der Arten, nicht der Inhalte") und 02 §6
// ("Ladeordnung ist Vertrag").
//
// Invariante I3 ist damit nicht verletzt: hier steht kein Content-Vokabular.
// "SAUSAGE", "SMOKED" oder "PREMIUM" kommen in dieser Datei nicht vor und
// duerfen es nie. Was hier steht, ist die Aussage "es gibt Kategorien" - nicht
// "es gibt die Kategorie X".
//
// Warum Strings und kein enum: die Art steht als "kind" im JSON-Dokument und
// im Log. Ein enum brauchte an jeder dieser Stellen eine Umwandlung und wuerde
// bei einer unbekannten Art still auf 0 fallen. Ein String faellt sichtbar auf.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_RecordKind
{
    static const string CORE_SETTINGS = "coreSettings";
    static const string CATEGORY      = "category";
    static const string TAG           = "tag";
    static const string STATE         = "state";
    static const string QUALITY_TIER  = "qualityTier";
    static const string TOOL_GROUP    = "toolGroup";
    static const string DEVICE        = "device";
    static const string CONTAINER     = "container";
    static const string INGREDIENT    = "ingredient";
    static const string NUTRITION     = "nutrition";
    static const string PRESERVATION  = "preservation";
    static const string PROCESS       = "process";
    static const string STATION       = "station";
    static const string TRANSFORM     = "transform";
    static const string RECIPE        = "recipe";

    //! Fangtabelle und Koederpraeferenz (20 §4.1, 21 §2/§3).
    //
    // Die Namen lauten "fishingYield" und "bait" und nicht "fishYield": der
    // statische Pruefer chefzcore ahndet das Wort "fish" in Core-Code als
    // Content-Vokabular (Invariante I3). "fishing" ist die Taetigkeit und
    // damit Core-Vokabular wie "HANDCRAFT" - derselbe Begriff traegt bereits
    // den Logkanal aus 20 §3.
    static const string FISHING_YIELD = "fishingYield";
    static const string BAIT          = "bait";

    /**
     * Die Ladeordnung aus 02 §6, woertlich:
     *
     *   CoreSettings -> Categories -> Tags -> States -> QualityTiers
     *                -> ToolGroups -> Devices -> Containers
     *                -> Ingredients -> Nutrition -> Preservation
     *                -> Processes -> Stations -> Transforms
     *                -> Recipes
     *
     * Sie ist eine Abhaengigkeitsordnung, keine Vorliebe: jede Art wird gegen
     * die vor ihr geladenen geprueft. Rezepte stehen zuletzt, weil sie gegen
     * alles pruefen.
     *
     * Neu erzeugtes Array je Aufruf - die Liste wird beim Boot ein einziges Mal
     * durchlaufen, und eine geteilte statische Liste, die ein Aufrufer
     * versehentlich sortiert, waere die teurere Variante.
     */
    static array<string> LoadOrder()
    {
        array<string> order = new array<string>();
        order.Insert(CORE_SETTINGS);
        order.Insert(CATEGORY);
        order.Insert(TAG);
        order.Insert(STATE);
        order.Insert(QUALITY_TIER);
        order.Insert(TOOL_GROUP);
        order.Insert(DEVICE);
        order.Insert(CONTAINER);
        order.Insert(INGREDIENT);
        order.Insert(NUTRITION);
        order.Insert(PRESERVATION);
        order.Insert(PROCESS);
        order.Insert(STATION);
        order.Insert(TRANSFORM);

        // Fangtabelle und Koeder haengen an KEINER anderen Art: ein Ertrag
        // nennt einen Klassennamen, ein Gewicht und zwei Masken, ein Koeder
        // nennt Ertragsnamen. Sie stehen hier und nicht am Ende, damit RECIPE
        // die letzte Art bleibt - Rezepte pruefen gegen alles, und diese
        // Aussage soll ein Leser nicht erst nachzaehlen muessen.
        //
        // Die Reihenfolge untereinander ist dagegen Vertrag: der Koeder wird
        // gegen die Ertraege geprueft (21 §3.4), also kommen die Ertraege
        // zuerst.
        order.Insert(FISHING_YIELD);
        order.Insert(BAIT);

        order.Insert(RECIPE);
        return order;
    }

    static bool IsKnown(string kind)
    {
        array<string> order = LoadOrder();
        for (int i = 0; i < order.Count(); i++)
        {
            if (order.Get(i) == kind)
                return true;
        }
        return false;
    }

    /**
     * Sync-relevante Arten nach 03 §4.
     *
     * Fuer sie gilt die zwingende Auflage: gespeist AUSSCHLIESSLICH aus Rang 1,
     * weil der Sync-Ordinal auf Client und Server unabhaengig aus derselben
     * sortierten Liste abgeleitet wird. Ein Overlay, das hier einen Record
     * hinzufuegt, bricht die Symmetrie - der Sink weist das als ERROR ab.
     */
    static bool IsSyncRelevant(string kind)
    {
        return kind == STATE || kind == QUALITY_TIER;
    }

    /**
     * Arten, die das $profile:-Overlay (Rang 3) NICHT stellen darf (20 §6 D3).
     *
     * Das ist eine ANDERE Auflage als IsSyncRelevant, auch wenn sie aehnlich
     * aussieht. Sync-relevante Arten scheitern an der ORDINALSYMMETRIE; diese
     * beiden scheitern an der GEWICHTSSYMMETRIE:
     *
     *   Client und Server bauen das Wahrscheinlichkeitsfeld der Angelaktion
     *   je fuer sich (CatchingContextBase.SetupProbabilityArray, aufgerufen
     *   aus ActionFishingNew.SetupAction) und ziehen daraus mit EINEM
     *   synchronisierten Zufallsstrom denselben Index. Das Overlay kennt aber
     *   nur der Server. Ein Patch auf diese beiden Arten liesse die Felder
     *   auseinanderlaufen - und dann zeigt der Client den Biss zu einem
     *   anderen Zeitpunkt, als der Server ihn wertet.
     *
     * Anders als bei IsSyncRelevant ist hier auch der FELD-Patch gesperrt, und
     * zwar aus genau diesem Grund: ein geaendertes baseWeight bewegt keine ID,
     * aber jedes einzelne Gewicht.
     */
    static bool IsOverlayBlocked(string kind)
    {
        return kind == FISHING_YIELD || kind == BAIT;
    }

    //! Sync-Obergrenze der Art, 0 = keine. Quelle: 03 §4 / ChefZ_SyncLimits.
    static int SyncLimit(string kind)
    {
        if (kind == STATE)
            return ChefZ_SyncLimits.STATE_ORDINAL_MAX;
        if (kind == QUALITY_TIER)
            return ChefZ_SyncLimits.QUALITY_ORDINAL_MAX;
        return ChefZ_SyncLimits.NO_LIMIT;
    }

    //! Fuer Fehlermeldungen: "unbekannte Art X, gueltig sind: ...".
    static string ValidNames()
    {
        array<string> order = LoadOrder();
        string s = "";
        for (int i = 0; i < order.Count(); i++)
        {
            if (i > 0)
                s = s + ", ";
            s = s + order.Get(i);
        }
        return s;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        array<string> order = LoadOrder();
        if (order.Count() != 17)                        return false;
        if (order.Get(0) != CORE_SETTINGS)              return false;
        if (order.Get(order.Count() - 1) != RECIPE)     return false;

        // Kategorien vor Zutaten, Zutaten vor Rezepten - die beiden
        // Abhaengigkeiten, die 02 §6 ausdruecklich nennt.
        int iCat = order.Find(CATEGORY);
        int iIng = order.Find(INGREDIENT);
        int iRec = order.Find(RECIPE);
        if (iCat < 0 || iIng < 0 || iRec < 0)           return false;
        if (!(iCat < iIng && iIng < iRec))              return false;

        if (!IsKnown(STATE))                            return false;
        if (IsKnown("gibtsNicht"))                      return false;
        if (!IsSyncRelevant(STATE))                     return false;
        if (!IsSyncRelevant(QUALITY_TIER))              return false;
        if (IsSyncRelevant(RECIPE))                     return false;
        if (SyncLimit(STATE) != ChefZ_SyncLimits.STATE_ORDINAL_MAX)     return false;
        if (SyncLimit(RECIPE) != ChefZ_SyncLimits.NO_LIMIT)             return false;

        // Fangtabelle und Koeder: bekannt, nicht sync-relevant, aber fuer das
        // Overlay gesperrt - und Ertraege vor Koedern (21 §3.4).
        if (!IsKnown(FISHING_YIELD))                    return false;
        if (!IsKnown(BAIT))                             return false;
        if (IsSyncRelevant(FISHING_YIELD))              return false;
        if (!IsOverlayBlocked(FISHING_YIELD))           return false;
        if (!IsOverlayBlocked(BAIT))                    return false;
        if (IsOverlayBlocked(RECIPE))                   return false;
        int iYield = order.Find(FISHING_YIELD);
        int iBait  = order.Find(BAIT);
        if (iYield < 0 || iBait < 0)                    return false;
        if (iYield >= iBait)                            return false;

        // Keine Art doppelt.
        for (int a = 0; a < order.Count(); a++)
        {
            for (int b = a + 1; b < order.Count(); b++)
            {
                if (order.Get(a) == order.Get(b))
                    return false;
            }
        }
        return true;
    }
}
