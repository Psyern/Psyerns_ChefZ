//==============================================================================
// ChefZ_ICapabilityProvider / ChefZ_CapabilityRegistry - Faehigkeiten von
//                                                        aussen, ohne dass der
//                                                        Core weiss, woher
//
// Entwurf: 17 §3.3 (Schnittstelle woertlich), 17 §2 (Umkehrung der
// Abhaengigkeit), 17 §9 (Fehlerverhalten, Zeile fuer Zeile), 12 E6
// (Capability wirkt als Grade-Regel), 12 E8 ("degrade verschiebt Stufen,
// blockiert nicht"), 11 E9 (kein Fremdsystem und kein Skillbegriff im Core).
//
// ---------------------------------------------------------------------------
// Was eine Faehigkeit fuer den Core ist
// ---------------------------------------------------------------------------
// Ein NAME und eine ZAHL. Mehr nicht. Der Core weiss nicht, ob die Zahl aus
// einem Skillsystem, einem Rang, einem Beruf oder einer Wuerfelrolle kommt -
// er weiss nur, dass ein registrierter Anbieter zu "CHEFZ_CAP_FIELDCOOK" den
// Wert 2 liefert. Das ist der ganze Mechanismus, und er ist der Grund, warum
// im Core kein einziger Fremdsystemname steht (11 E9, 17 §2).
//
// Ein Comp-Modul baut seine Ableitung in seinem EIGENEN PBO:
//
//     class MeinMod_CapabilityProvider extends ChefZ_ICapabilityProvider { ... }
//     ChefZ_CapabilityRegistry.Get().RegisterProvider(new MeinMod_CapabilityProvider());
//
// ---------------------------------------------------------------------------
// Der Server ohne Skillmod ist der NORMALFALL
// ---------------------------------------------------------------------------
// Ohne einen einzigen Anbieter liefert GetCapability() den Default aus
// ChefZ_CoreSettingsDef, und der Server ist voll spielbar (17 §9). Es gibt
// keinen Pfad, auf dem eine fehlende Faehigkeit einen Fehler erzeugt: das
// Schlimmste, was passieren kann, ist ein Gericht eine Stufe schlechter.
//
// Zwei Betreiberschalter dazu (17 §9, capabilityMode):
//   "neverBlock"  -> onFail "block" wird wie "degrade" behandelt
//   "ignore"      -> ALLE requires[] gelten als erfuellt
//
// ---------------------------------------------------------------------------
// Zwei Vertraege, die absichtlich verschieden sind
// ---------------------------------------------------------------------------
//   GetCapability()   antwortet IMMER - notfalls mit dem Config-Default.
//                     Das ist der Vertrag aus 17 §3.3 und gilt fuer
//                     Anforderungen (requires[]).
//   ChefZ_RegistryCapabilityProbe.TryGetValue() antwortet mit FALSE, wenn kein
//                     Anbieter etwas sagen konnte. Das ist der Vertrag aus
//                     12 §8 und gilt fuer Qualitaetsregeln: "Capability-
//                     Provider fehlt -> Regel gilt als nicht erfuellt
//                     (0 Punkte). KEIN Fehler - ohne Skillmod gibt es eben
//                     keinen Skillbonus."
//
// Der Unterschied ist kein Versehen. Eine Anforderung braucht eine Zahl, um
// ueberhaupt entscheiden zu koennen; eine Bonusregel darf schlicht nicht
// zuenden. Waeren beide gleich, gaebe der Config-Default entweder jedem
// Spieler einen Skillbonus oder er sperrte jedes Rezept.
//
// KEIN CONTENT: keine Faehigkeit wird hier benannt. "CHEFZ_CAP_FIELDCOOK" ist
// ein Beispiel aus dem Entwurf und steht in keiner Zeile Code.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Die Auskunftsstelle, die ein Comp-Modul liefert.
 *
 * Enforce kennt kein "interface" - 17 §3.3 schreibt es als Pseudocode. Die
 * Umsetzung ist eine Basisklasse, deren Vorgaben ausdruecklich SICHER sind:
 * eine Ableitung, die eine Methode vergisst, liefert "weiss ich nicht" und
 * nicht einen Absturz.
 */
class ChefZ_ICapabilityProvider
{
    //! Erscheint in jeder Meldung ueber diesen Anbieter. Ohne ihn ist ein
    //! Anbieter, der Unsinn liefert, nicht zuzuordnen (17 §9).
    string GetProviderName()
    {
        return "<unbenannter Anbieter>";
    }

    //! Hoechste Prioritaet gewinnt; bei Gleichstand der zuerst registrierte
    //! (17 §9). Ein Modul, das nur einen Ausfallwert liefern will, meldet sich
    //! mit einer negativen Prioritaet an.
    int GetPriority()
    {
        return 0;
    }

    /**
     * @param identityId  0 heisst "niemand beteiligt". Ein Anbieter, der einen
     *                    Spieler braucht, antwortet dann mit false.
     * @param capability  Symbol des Faehigkeitsnamens.
     * @param value       nur bei true belegt.
     * @return false, wenn dieser Anbieter zu dieser Faehigkeit nichts sagen
     *         kann. Dann wird der naechste Anbieter gefragt.
     */
    bool TryGetCapability(int identityId, ChefZ_Sym capability, out float value)
    {
        value = 0.0;
        return false;
    }
}

//==============================================================================

class ChefZ_CapabilityRegistry
{
    private static ref ChefZ_CapabilityRegistry s_Instance;

    static const string MODE_AS_AUTHORED = "asAuthored";
    static const string MODE_NEVER_BLOCK = "neverBlock";
    static const string MODE_IGNORE      = "ignore";

    static const string ON_FAIL_BLOCK        = "block";
    static const string ON_FAIL_DEGRADE      = "degrade";
    static const string ON_FAIL_REDUCE_YIELD = "reduceYield";

    /**
     * Die Anbieter, absteigend nach Prioritaet.
     *
     * MIT ref, anders als der Besitzer einer Ereignisanmeldung: ein Anbieter
     * wird ueblicherweise als "new MeinProvider()" im Boot des Comp-Moduls
     * uebergeben und hat dort keinen zweiten Halter. Ohne ref waere er sofort
     * wieder weg. Wer ihn loswerden will, ruft UnregisterProvider().
     */
    private ref array<ref ChefZ_ICapabilityProvider> m_Providers;

    //--- Regler aus Core.json (17 §3.3, §9) ----------------------------------
    private string m_Mode;
    private float  m_Default;
    private float  m_Min;
    private float  m_Max;

    private bool m_QuietForTest;

    private int m_CountQueries;
    private int m_CountAnswered;
    private int m_CountClamped;

    void ChefZ_CapabilityRegistry()
    {
        m_Providers    = new array<ref ChefZ_ICapabilityProvider>();
        m_Mode         = MODE_AS_AUTHORED;
        m_Default      = 0.0;
        m_Min          = 0.0;
        m_Max          = 10.0;
        m_QuietForTest = false;
        ResetCounters();
    }

    static ChefZ_CapabilityRegistry Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_CapabilityRegistry();
        return s_Instance;
    }

    /**
     * Die Regler aus Core.json uebernehmen.
     *
     * settings darf null sein - dann gelten die Code-Defaults. Die Registry
     * funktioniert unabhaengig von der Config: ein Comp-Modul, das sich vor
     * dem Laden anmeldet, soll das duerfen.
     */
    void Configure(ChefZ_CoreSettingsDef settings)
    {
        if (!settings)
            return;

        if (ChefZ_CoreSettingsDef.IsKnownCapabilityMode(settings.capabilityMode))
            m_Mode = settings.capabilityMode;

        m_Default = settings.defaultCapabilityValue;
        m_Min     = settings.capabilityMin;
        m_Max     = settings.capabilityMax;

        if (m_Max < m_Min)
            m_Max = m_Min;
        if (m_Default < m_Min)
            m_Default = m_Min;
        if (m_Default > m_Max)
            m_Default = m_Max;

        if (m_Mode != MODE_AS_AUTHORED)
        {
            // An der Stufenpruefung vorbei waere hier zu viel, aber ein
            // Betreiberschalter, der jede Faehigkeitsanforderung aushebelt,
            // gehoert ins Startlog - sonst sucht irgendwann jemand stundenlang,
            // warum sein Rezeptschloss nicht greift.
            ChefZ_Log.Info(ChefZ_LogChannel.EVENT,
                "capabilityMode = \"" + m_Mode + "\": "
                + ModeExplanation(m_Mode));
        }
    }

    private string ModeExplanation(string mode)
    {
        if (mode == MODE_NEVER_BLOCK)
            return "onFail \"block\" wirkt wie \"degrade\" - kein Rezept wird gesperrt.";
        if (mode == MODE_IGNORE)
            return "alle requires[] gelten als erfuellt - Faehigkeiten wirken gar nicht.";
        return "Faehigkeitsanforderungen wirken wie im Content geschrieben.";
    }

    //==========================================================================
    // Anbieter (17 §3.3)
    //==========================================================================

    void RegisterProvider(notnull ChefZ_ICapabilityProvider provider)
    {
        if (m_Providers.Find(provider) >= 0)
        {
            Warn("cap.provider.dup." + provider.GetProviderName(),
                "Anbieter \"" + provider.GetProviderName() + "\" ist bereits registriert. "
                + "Die zweite Anmeldung wird verworfen.");
            return;
        }

        int prio = provider.GetPriority();

        for (int i = 0; i < m_Providers.Count(); i++)
        {
            ChefZ_ICapabilityProvider other = m_Providers.Get(i);
            if (!other)
                continue;

            if (other.GetPriority() == prio)
            {
                // 17 §9: "Zwei Provider antworten -> hoechste GetPriority()
                // gewinnt; bei Gleichstand der zuerst registrierte, WARN."
                Warn("cap.provider.tie." + prio.ToString(),
                    "Anbieter \"" + provider.GetProviderName() + "\" und \""
                    + other.GetProviderName() + "\" haben dieselbe Prioritaet ("
                    + prio.ToString() + "). Es antwortet der zuerst registrierte. "
                    + "Wer das entscheiden will, vergibt unterschiedliche Prioritaeten.");
                break;
            }
        }

        InsertByPriority(provider, prio);

        ChefZ_Log.Info(ChefZ_LogChannel.EVENT,
            "Faehigkeitsanbieter \"" + provider.GetProviderName() + "\" registriert (prio "
            + prio.ToString() + ", jetzt " + m_Providers.Count().ToString() + ").");
    }

    void UnregisterProvider(notnull ChefZ_ICapabilityProvider provider)
    {
        int idx = m_Providers.Find(provider);
        if (idx < 0)
            return;
        m_Providers.RemoveOrdered(idx);         // Reihenfolge IST die Prioritaet
    }

    int GetProviderCount()
    {
        return m_Providers.Count();
    }

    private void InsertByPriority(notnull ChefZ_ICapabilityProvider provider, int prio)
    {
        for (int i = 0; i < m_Providers.Count(); i++)
        {
            ChefZ_ICapabilityProvider other = m_Providers.Get(i);
            if (other && other.GetPriority() < prio)
            {
                m_Providers.InsertAt(provider, i);
                return;
            }
        }
        m_Providers.Insert(provider);
    }

    //==========================================================================
    // Abfrage
    //==========================================================================

    /**
     * Der Wert einer Faehigkeit. ANTWORTET IMMER (17 §3.3).
     *
     * Ohne Anbieter, ohne Antwort oder bei Unsinn: der Default aus
     * ChefZ_CoreSettingsDef. Nie ein Fehler, nie ein Abbruch.
     */
    float GetCapability(int identityId, ChefZ_Sym capability)
    {
        float value;
        if (TryQuery(identityId, capability, value))
            return value;
        return m_Default;
    }

    //! Bequemlichkeit fuer Aufrufer, die den Namen und nicht das Symbol haben -
    //! ChefZ_CapabilityReq traegt eine Zeichenkette.
    float GetCapabilityByName(int identityId, string capability)
    {
        return GetCapability(identityId, SymbolOf(capability));
    }

    /**
     * Hat ueberhaupt jemand geantwortet?
     *
     * Der zweite Vertrag (siehe Kopf): fuer Qualitaetsregeln, die ohne
     * Skillmod schlicht nicht zuenden sollen.
     */
    bool TryQuery(int identityId, ChefZ_Sym capability, out float value)
    {
        value = m_Default;
        m_CountQueries++;

        if (!ChefZ_SymbolTable.IsValid(capability))
            return false;
        if (m_Providers.Count() == 0)
            return false;

        for (int i = 0; i < m_Providers.Count(); i++)
        {
            ChefZ_ICapabilityProvider p = m_Providers.Get(i);
            if (!p)
                continue;

            float raw;
            if (!p.TryGetCapability(identityId, capability, raw))
                continue;

            value = Sanitize(raw, p);
            m_CountAnswered++;
            return true;                        // hoechste Prioritaet gewinnt
        }

        return false;
    }

    /**
     * 17 §9: "Provider liefert NaN, negativ oder unsinnig -> auf den
     * Config-Bereich geklemmt, WARN EINMAL JE PROVIDER."
     *
     * Klemmen statt abweisen, weil ein Anbieter, der einmal danebengreift,
     * kein Grund ist, das Kochsystem eines Servers abzuschalten - und weil ein
     * abgewiesener Wert stillschweigend zum Default wuerde, was schwerer zu
     * finden waere als eine geklemmte Zahl.
     */
    private float Sanitize(float raw, notnull ChefZ_ICapabilityProvider provider)
    {
        if (!ChefZ_EventArgs.IsFinite(raw))
        {
            m_CountClamped++;
            Warn("cap.nan." + provider.GetProviderName(),
                "Anbieter \"" + provider.GetProviderName() + "\" liefert keine Zahl. "
                + "Es gilt der Default " + m_Default.ToString()
                + " (CoreSettings.defaultCapabilityValue).");
            return m_Default;
        }

        if (raw < m_Min)
        {
            m_CountClamped++;
            Warn("cap.low." + provider.GetProviderName(),
                "Anbieter \"" + provider.GetProviderName() + "\" liefert " + raw.ToString()
                + " - geklemmt auf " + m_Min.ToString()
                + " (CoreSettings.capabilityMin). Diese Meldung erscheint einmal je Anbieter.");
            return m_Min;
        }

        if (raw > m_Max)
        {
            m_CountClamped++;
            Warn("cap.high." + provider.GetProviderName(),
                "Anbieter \"" + provider.GetProviderName() + "\" liefert " + raw.ToString()
                + " - geklemmt auf " + m_Max.ToString()
                + " (CoreSettings.capabilityMax). Diese Meldung erscheint einmal je Anbieter.");
            return m_Max;
        }

        return raw;
    }

    //==========================================================================
    // Anforderungen (17 §3.3)
    //==========================================================================

    /**
     * Erfuellt dieser Spieler diese Anforderung?
     *
     * @param failReason   nur bei false belegt, im Klartext.
     * @param degradeSteps nur bei false belegt und nur bei onFail "degrade"
     *                     von Belang.
     *
     * capabilityMode "ignore" laesst JEDE Anforderung als erfuellt gelten -
     * der Betreiberschalter fuer Server ohne Skillmod (17 §9).
     */
    bool MeetsRequirement(int identityId, notnull ChefZ_CapabilityReq req,
                          out string failReason, out int degradeSteps)
    {
        failReason   = "";
        degradeSteps = 0;

        if (m_Mode == MODE_IGNORE)
            return true;
        if (req.capability == "")
            return true;                        // nichts gefordert, nichts zu pruefen

        float value = GetCapabilityByName(identityId, req.capability);
        if (value >= req.min)
            return true;

        degradeSteps = req.degradeSteps;
        failReason   = "Faehigkeit \"" + req.capability + "\" ist " + value.ToString()
                     + ", gefordert sind " + req.min.ToString();
        return false;
    }

    /**
     * Was bei Nichterfuellung wirklich passiert (17 §9, Zeile "neverBlock").
     *
     * Der Betreiberschalter greift GENAU HIER und an keiner zweiten Stelle:
     * "neverBlock" macht aus einem "block" ein "degrade". Damit ist ein
     * Server, dessen Betreiber keine harten Rezeptschloesser will, mit einer
     * Zeile in Core.json entschaerft - ohne den Content anzufassen.
     */
    string EffectiveOnFail(notnull ChefZ_CapabilityReq req)
    {
        string onFail = req.onFail;
        if (onFail == "")
            onFail = ON_FAIL_DEGRADE;

        if (onFail == ON_FAIL_BLOCK && m_Mode == MODE_NEVER_BLOCK)
            return ON_FAIL_DEGRADE;

        return onFail;
    }

    /**
     * Der Filter aus 08 §7 Schritt 2c: sperrt eine dieser Anforderungen das
     * Rezept?
     *
     * NUR "block" sperrt. "degrade" und "reduceYield" sind Abwertungen und
     * wirken spaeter - ein Spieler ohne Faehigkeit bekommt das Gericht, nur
     * schlechter (12 E8).
     */
    bool BlocksAny(array<ref ChefZ_CapabilityReq> reqs, int identityId, out string reason)
    {
        reason = "";

        if (!reqs || reqs.Count() == 0)
            return false;
        if (m_Mode == MODE_IGNORE)
            return false;

        for (int i = 0; i < reqs.Count(); i++)
        {
            ChefZ_CapabilityReq req = reqs.Get(i);
            if (!req)
                continue;
            if (EffectiveOnFail(req) != ON_FAIL_BLOCK)
                continue;

            string why;
            int    steps;
            if (MeetsRequirement(identityId, req, why, steps))
                continue;

            reason = why;
            return true;
        }

        return false;
    }

    /**
     * Wieviele Stufen das Ergebnis abgewertet wird (12 E8).
     *
     * Summe ueber alle nicht erfuellten "degrade"-Anforderungen. Summe und
     * nicht Maximum: zwei fehlende Faehigkeiten sind zwei Gruende, und ein
     * Rezept, das beide fordert, meint auch beide.
     */
    int DegradeStepsFor(array<ref ChefZ_CapabilityReq> reqs, int identityId,
                        out string reason)
    {
        reason = "";

        if (!reqs || reqs.Count() == 0)
            return 0;
        if (m_Mode == MODE_IGNORE)
            return 0;

        int total = 0;

        for (int i = 0; i < reqs.Count(); i++)
        {
            ChefZ_CapabilityReq req = reqs.Get(i);
            if (!req)
                continue;
            if (EffectiveOnFail(req) != ON_FAIL_DEGRADE)
                continue;

            string why;
            int    steps;
            if (MeetsRequirement(identityId, req, why, steps))
                continue;

            if (steps <= 0)
                continue;

            total = total + steps;
            if (reason != "")
                reason = reason + "; ";
            reason = reason + why;
        }

        return total;
    }

    /**
     * Ausbeutefaktor aus nicht erfuellten "reduceYield"-Anforderungen.
     *
     * Produkt und nicht Summe: zwei Abschlaege von je 0.75 ergeben 0.5625 und
     * nicht 0.5. Ein Produkt kann nie negativ werden, eine Summe schon - und
     * eine negative Ausbeute waere ein Rezept, das Zutaten frisst und nichts
     * erzeugt.
     */
    float YieldFactorFor(array<ref ChefZ_CapabilityReq> reqs, int identityId)
    {
        if (!reqs || reqs.Count() == 0)
            return 1.0;
        if (m_Mode == MODE_IGNORE)
            return 1.0;

        float factor = 1.0;

        for (int i = 0; i < reqs.Count(); i++)
        {
            ChefZ_CapabilityReq req = reqs.Get(i);
            if (!req)
                continue;
            if (EffectiveOnFail(req) != ON_FAIL_REDUCE_YIELD)
                continue;

            string why;
            int    steps;
            if (MeetsRequirement(identityId, req, why, steps))
                continue;

            float f = req.yieldFactor;
            if (f < 0.0)
                f = 0.0;
            if (f > 1.0)
                f = 1.0;
            factor = factor * f;
        }

        return factor;
    }

    //==========================================================================
    // Auskunft und Wartung
    //==========================================================================

    string GetMode()          { return m_Mode; }
    float  GetDefaultValue()  { return m_Default; }
    float  GetMin()           { return m_Min; }
    float  GetMax()           { return m_Max; }
    int    GetQueryCount()    { return m_CountQueries; }
    int    GetAnsweredCount() { return m_CountAnswered; }
    int    GetClampedCount()  { return m_CountClamped; }

    void ResetCounters()
    {
        m_CountQueries  = 0;
        m_CountAnswered = 0;
        m_CountClamped  = 0;
    }

    void DumpProviders(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Faehigkeiten: " + m_Providers.Count().ToString() + " Anbieter, "
            + "modus=" + m_Mode
            + "  default=" + m_Default.ToString()
            + "  bereich=[" + m_Min.ToString() + ".." + m_Max.ToString() + "]"
            + "  abfragen=" + m_CountQueries.ToString()
            + "  beantwortet=" + m_CountAnswered.ToString()
            + "  geklemmt=" + m_CountClamped.ToString());

        if (m_Providers.Count() == 0)
        {
            outLines.Insert("  (kein Anbieter - jeder Spieler gilt als "
                + m_Default.ToString() + ", der Server ist voll spielbar)");
            return;
        }

        for (int i = 0; i < m_Providers.Count(); i++)
        {
            ChefZ_ICapabilityProvider p = m_Providers.Get(i);
            if (p)
                outLines.Insert("  " + p.GetProviderName() + "  prio=" + p.GetPriority().ToString());
        }
    }

    //! Vorgesehener Aufrufer ist der SAFE_MODE (02 §8) und der Selbsttest.
    void ClearProviders()
    {
        m_Providers.Clear();
    }

    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //! Nur fuer den Selbsttest: die Regler ohne Config setzen.
    void ConfigureForTest(string mode, float defaultValue, float minValue, float maxValue)
    {
        if (ChefZ_CoreSettingsDef.IsKnownCapabilityMode(mode))
            m_Mode = mode;
        m_Default = defaultValue;
        m_Min     = minValue;
        m_Max     = maxValue;
    }

    /**
     * Symbol zu einem Faehigkeitsnamen.
     *
     * Intern() und nicht Lookup(): Faehigkeitsnamen stammen aus einem fremden
     * PBO und aus Content-Records; im Core werden sie nie interniert. Lookup()
     * lieferte deshalb dauerhaft INVALID, und kein Anbieter kaeme je zu Wort.
     *
     * Der Warnhinweis an ChefZ_SymbolTable.Intern ("nie im heissen Pfad")
     * gilt hier nicht: die Menge der Faehigkeitsnamen ist durch den Content
     * beschraenkt und winzig, und fuer einen bereits bekannten Namen ist
     * Intern() genau ein Map-Zugriff. Der Pfad selbst ist zudem nicht heiss -
     * er laeuft je Kandidatenrezept eines Vollmatches, und Vollmatches sind
     * gedrosselt (10 §6).
     */
    private ChefZ_Sym SymbolOf(string capability)
    {
        if (capability == "")
            return ChefZ_SymbolTable.INVALID;
        return ChefZ_SymbolTable.Intern(capability);
    }

    private void Warn(string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.EVENT, key, message);
    }
}

//==============================================================================

/**
 * Die Registry als Auskunftsstelle fuer QUALITAETSREGELN (12 E6).
 *
 * Sie haelt sich absichtlich an den Vertrag aus 12 §8 und nicht an den aus
 * 17 §3.3: ohne antwortenden Anbieter liefert sie FALSE, damit eine
 * capability-Regel schlicht nicht zuendet. Der Config-Default waere hier
 * falsch - er gaebe jedem Spieler auf jedem Server ohne Skillmod denselben
 * Bonus, und das ist eine Balancingaussage, die niemand getroffen hat.
 *
 * Eingehaengt wird sie beim Boot ueber ChefZ_QualityManager.SetCapabilityProbe.
 */
class ChefZ_RegistryCapabilityProbe extends ChefZ_CapabilityProbe
{
    override bool TryGetValue(string capability, int actorId, out float value)
    {
        value = 0.0;

        ChefZ_CapabilityRegistry reg = ChefZ_CapabilityRegistry.Get();
        if (reg.GetProviderCount() == 0)
            return false;

        return reg.TryQuery(actorId, ChefZ_SymbolTable.Intern(capability), value);
    }
}

//==============================================================================

/**
 * Die Registry als Filter im Rezeptablauf (08 §7 Schritt 2c).
 *
 * Eingehaengt wird sie beim Boot ueber ChefZ_CapabilityGate.SetActive. Ohne
 * diesen Einhaengepunkt verhaelt sich der Rezeptablauf exakt wie bis S12 -
 * es blockiert nichts.
 */
class ChefZ_RegistryCapabilityGate extends ChefZ_CapabilityGate
{
    override bool BlocksRecipe(array<ref ChefZ_CapabilityReq> reqs, int actorId,
                               out string reason)
    {
        reason = "";
        return ChefZ_CapabilityRegistry.Get().BlocksAny(reqs, actorId, reason);
    }
}
