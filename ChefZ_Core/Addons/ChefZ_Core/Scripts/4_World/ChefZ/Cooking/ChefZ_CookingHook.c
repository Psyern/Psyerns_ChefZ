//==============================================================================
// ChefZ_CookingHook - die Nahtstelle zwischen Vanilla und ChefZ
//
// Entwurf: 10 §3 (der Hook), 10 §4 (Schnittstelle woertlich), 10 §5 (Stufe 0),
// 10 §8 (Fehlerverhalten), 10 E1 (Post-Hook statt Pre-Hook), 10 E3 (Methode
// erfragen statt nachbilden), 01 V1, 01 V2, 01 V11.
//
// ---------------------------------------------------------------------------
// Diese Klasse ist bewusst duenn
// ---------------------------------------------------------------------------
// Sie besteht aus einem Bool-Test, einer Umrechnung und einer Weitergabe. Es
// gibt keinen Zustand, keine Entscheidung und keinen Rueckkanal.
//
// Der fehlende Rueckkanal ist der Kern (10 §3): AfterVanillaCook gibt nichts
// zurueck und nimmt nichts entgegen, womit sich der bereits gelaufene
// Vanilla-Tick beeinflussen liesse. Regel §10.2 - "Vanilla-Kochen bleibt
// intakt" - ist damit Struktur und nicht Vorsatz. Wer sie brechen wollte,
// muesste die Signatur aendern, und das faellt in jedem Diff auf.
//
// ---------------------------------------------------------------------------
// Die Methodennamen sind Vanilla-Vokabular, kein ChefZ-Content
// ---------------------------------------------------------------------------
// BAKING, BOILING, DRYING, TIME und NONE sind die fuenf Werte des
// engine-seitigen enum CookingMethodType (Cooking.c:1). 08 §2 fuehrt genau
// diese fuenf Namen als zulaessige Eintraege in recipe.contexts[].methods.
// Sie hier zu benennen ist dasselbe wie in ChefZ_VanillaStage: eine
// geschlossene, engine-gegebene Werteliste wird benannt, damit die Datenseite
// sie ansprechen kann. Es ist kein Gericht, keine Zutat und keine Station.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CookingHook
{
    //! Die fuenf Namen aus 08 §2, in derselben Schreibweise, in der ein Rezept
    //! sie nennt. Case-sensitiv, weil ChefZ_SymbolTable.Intern es ist.
    static const string METHOD_NONE    = "NONE";
    static const string METHOD_BAKING  = "BAKING";
    static const string METHOD_BOILING = "BOILING";
    static const string METHOD_DRYING  = "DRYING";
    static const string METHOD_TIME    = "TIME";

    //! Einmal interniert, danach ein Array-Zugriff. Die Umrechnung laeuft in
    //! jedem Kochtick jeder Feuerstelle.
    private static ref array<ChefZ_Sym> s_MethodSyms;

    //! Nur fuer QueryMethodForDiagnostics (S18). Wird NIE im Kochtick
    //! angefasst und ist deshalb bewusst lazy: auf einem Server, an dem nie
    //! jemand "chefz match" tippt, entsteht dieses Objekt nicht.
    private static ref Cooking s_QueryCooking;

    //==========================================================================

    /**
     * Die eine Frage, die vor allem anderen steht (19 S7: "Bei leerem
     * Rezeptbestand kostet der Hook einen Bool-Test").
     *
     * Sie deckt drei Faelle auf einmal ab (10 §8):
     *   - Client: nichts Autoritatives auf dem Client, der Hook laeuft nie
     *   - enabled = false oder SAFE_MODE: der Core ist inert, Kochen ist
     *     bitgenau Vanilla
     *   - Config nie geladen: IsActive() ist false, solange nichts geladen ist
     */
    static bool IsEnabled()
    {
        if (!g_Game || !g_Game.IsServer())
            return false;

        return ChefZ_ConfigManager.Get().IsActive();
    }

    /**
     * Der Eintritt aus der modded class Cooking, NACH super.
     *
     * @param device     das Gefaess, unveraendert wie Vanilla es bekam
     * @param timeCoef   cooking_time_coef desselben Aufrufs
     * @param updateTime m_UpdateTime der Cooking-Instanz (01 V11: die einzige
     *                   verlaessliche Zeitquelle fuer TIMED-Rezepte)
     * @param method     Ergebnis von GetCookingMethodWithTimeOverride, also
     *                   Vanillas eigene Auskunft (10 E3). Darf null sein.
     *
     * Kein Rueckgabewert. Siehe Kopf.
     */
    static void AfterVanillaCook(ItemBase device, float timeCoef, float updateTime, Param2<CookingMethodType, float> method)
    {
        int methodType = CookingMethodType.NONE;
        if (method)
            methodType = method.param1;

        ChefZ_CookingDeviceAdapter.Get().Observe(device, timeCoef, updateTime, methodType);
    }

    /**
     * Der billige Vorabtest, damit der Aufrufer Vanilla nicht ohne Not nach
     * der Kochmethode fragen muss.
     *
     * GetCookingMethodWithTimeOverride laeuft im Zweifel durchs ganze Cargo
     * (Cooking.c:445, GetItemTypeFromCargo fuer Lard) und legt ein Param2 an.
     * Das jeden Tick jeder Feuerstelle zu bezahlen, obwohl ChefZ abgeschaltet
     * ist oder das Geraet gar nicht kennt, widerspraeche der Abnahmebedingung
     * aus 19 S7.
     *
     * Die Pruefung ist dieselbe wie Stufe 0 im Adapter und wird dort gleich
     * darauf wiederholt. Das ist Absicht: AfterVanillaCook ist oeffentlich und
     * darf sich nicht darauf verlassen, dass jemand vorher gefragt hat.
     */
    static bool ShouldObserve(ItemBase device)
    {
        // IsEnabled() steht VOR jedem Zugriff auf den Adapter, damit
        // ChefZ_CookingDeviceAdapter.Get() auf einem Client oder bei
        // abgeschaltetem Core gar nicht erst eine Instanz anlegt. Auf einem
        // Server ohne ChefZ-Konfiguration ist der gesamte Hook damit
        // buchstaeblich ein Bool-Test (19 S7).
        if (!IsEnabled())
            return false;

        if (!device || device.IsRuined())
            return false;

        ChefZ_DeviceDescriptor desc;
        int cargoCount;
        return ChefZ_CookingDeviceAdapter.Get().PassesGate(device, desc, cargoCount);
    }

    //==========================================================================

    /**
     * CookingMethodType -> ChefZ-Symbol (10 §4).
     *
     * Ein unbekannter Wert - etwa weil ein DayZ-Update den enum erweitert -
     * ergibt NONE und keine Ausnahme. Ein Rezept, das keine Methode nennt,
     * laeuft dann weiter; eines, das eine nennt, bindet nicht. Das ist die
     * richtige Richtung: im Zweifel weniger ChefZ.
     */
    static ChefZ_Sym MapVanillaMethod(int cookingMethodType)
    {
        EnsureMethodSyms();

        if (cookingMethodType < 0 || cookingMethodType >= s_MethodSyms.Count())
            return s_MethodSyms.Get(CookingMethodType.NONE);

        return s_MethodSyms.Get(cookingMethodType);
    }

    /**
     * if-Kette und kein switch: der Parameter ist ein int, die Marken waeren
     * Werte eines enum. Enforce laesst das je nach Kontext durchgehen oder
     * nicht, und dieselbe Vorsicht steht bereits in
     * ChefZ_RecipeEvaluator.CheckReady. Fuenf Vergleiche, einmal beim ersten
     * Kochtick des Servers - das ist die Lesbarkeit wert.
     */
    static string MethodName(int cookingMethodType)
    {
        if (cookingMethodType == CookingMethodType.BAKING)  return METHOD_BAKING;
        if (cookingMethodType == CookingMethodType.BOILING) return METHOD_BOILING;
        if (cookingMethodType == CookingMethodType.DRYING)  return METHOD_DRYING;
        if (cookingMethodType == CookingMethodType.TIME)    return METHOD_TIME;
        return METHOD_NONE;
    }

    /**
     * Die Tabelle wird ueber den Zahlenwert des enum indiziert, nicht ueber
     * eine Map. Deshalb muessen die Eintraege genau in der Reihenfolge des
     * enum stehen - Cooking.c:1:
     *
     *     NONE = 0, BAKING = 1, BOILING = 2, DRYING = 3, TIME = 4, COUNT
     */
    private static void EnsureMethodSyms()
    {
        if (s_MethodSyms)
            return;

        s_MethodSyms = new array<ChefZ_Sym>();
        for (int i = 0; i < CookingMethodType.COUNT; i++)
            s_MethodSyms.Insert(ChefZ_SymbolTable.Intern(MethodName(i)));
    }

    //! Nur fuer Test und Diagnose.
    static void ClearCaches()
    {
        s_MethodSyms = null;
        s_QueryCooking = null;
    }

    //==========================================================================
    // Diagnose (S18)
    //==========================================================================

    /**
     * Welche Kochmethode saehe Vanilla an diesem Gefaess GERADE JETZT?
     *
     * Nur fuer die Diagnose. Der Kochtick fragt nicht hier, sondern bekommt
     * die Auskunft aus derselben Cooking-INSTANZ, die gerade kocht - die
     * kennt m_UpdateTime, und die zaehlt fuer TIMED-Rezepte (01 V11).
     *
     * Warum eine eigene Instanz: "chefz match" laeuft ausserhalb jedes
     * Kochticks, und es gibt keinen Weg, an die Cooking-Instanz einer
     * beliebigen Feuerstelle zu kommen (FireplaceBase.m_CookingProcess ist
     * protected). Vanilla selbst legt seine Instanz genauso an
     * (FireplaceBase.c:318, "m_CookingProcess = new Cooking()"), und
     * GetCookingMethodWithTimeOverride haengt an keinem Instanzzustand - es
     * liest ausschliesslich das uebergebene Gefaess.
     *
     * Die Instanz wird EINMAL angelegt und behalten: ein Diagnosekommando
     * soll keinen Allokationsspur hinterlassen, und ein Cooking-Objekt ohne
     * Bindung an eine Feuerstelle kostet nichts.
     *
     * @return CookingMethodType als int. NONE, wenn nichts ermittelbar ist -
     *         nie eine Ausnahme.
     */
    static int QueryMethodForDiagnostics(ItemBase device)
    {
        if (!device)
            return CookingMethodType.NONE;

        if (!s_QueryCooking)
            s_QueryCooking = new Cooking();

        return s_QueryCooking.ChefZ_QueryMethod(device);
    }

    //==========================================================================

    //! Nur fuer den Selbsttest. Braucht kein Gefaess und keine Feuerstelle.
    static bool SelfCheck()
    {
        // Die Zuordnung muss auf beiden Wegen dasselbe ergeben - sonst matcht
        // ein Rezept mit "methods": ["BOILING"] am siedenden Topf nicht.
        if (MapVanillaMethod(CookingMethodType.BOILING) != ChefZ_SymbolTable.Intern(METHOD_BOILING))    return false;
        if (MapVanillaMethod(CookingMethodType.BAKING) != ChefZ_SymbolTable.Intern(METHOD_BAKING))     return false;
        if (MapVanillaMethod(CookingMethodType.DRYING) != ChefZ_SymbolTable.Intern(METHOD_DRYING))     return false;
        if (MapVanillaMethod(CookingMethodType.TIME) != ChefZ_SymbolTable.Intern(METHOD_TIME))       return false;
        if (MapVanillaMethod(CookingMethodType.NONE) != ChefZ_SymbolTable.Intern(METHOD_NONE))       return false;

        // Ausserhalb des Wertebereichs -> NONE, keine Ausnahme.
        if (MapVanillaMethod(-1) != ChefZ_SymbolTable.Intern(METHOD_NONE))       return false;
        if (MapVanillaMethod(9999) != ChefZ_SymbolTable.Intern(METHOD_NONE))       return false;

        return true;
    }
}
