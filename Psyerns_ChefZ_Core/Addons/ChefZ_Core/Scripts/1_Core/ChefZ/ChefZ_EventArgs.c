//==============================================================================
// ChefZ_EventArgs - die Nutzlast jedes ChefZ-Ereignisses
//
// Entwurf: 17 §3.1 (Feldliste woertlich), 17 E3 (Kontextklasse statt
// Param-Tupel), 17 E4 (nur Symbole und Netz-IDs, keine EntityAI), 17 §8
// (Lebensdauer: aus einem Pool, NACH Raise ungueltig).
//
// ---------------------------------------------------------------------------
// Warum hier keine einzige EntityAI steht (17 E4)
// ---------------------------------------------------------------------------
// Drei Gruende, und jeder allein wuerde reichen:
//
//   1. Diese Klasse liegt in 1_Core. Die Layer-Regel aus 00 §4 verbietet
//      Engine-Typen dort - dieselbe Regel, die den Matcher und den
//      Preservation Manager ohne laufendes Spiel pruefbar macht.
//   2. Ein Abonnent kann keine Entity veraendern, an der der Core GERADE
//      arbeitet. Waere hier ein ItemBase, koennte ein fremder Mod mitten in
//      der Kochtransaktion eine Zutat loeschen.
//   3. Ein aufbewahrter Kontext kann keinen Nullzugriff auf eine geloeschte
//      Entity ausloesen - er traegt nur Zahlen.
//
// Wer die Entity braucht, loest die Netz-ID in 4_World auf
// (g_Game.GetObjectByNetworkId) - bewusst, mit eigener Gueltigkeitspruefung.
// Nebeneffekt: Ereignisse sind serialisierbar, ein spaeteres Statistikmodul
// kann sie ohne Weiteres in eine Datei schreiben.
//
// ---------------------------------------------------------------------------
// Lebensdauer - die typische Fehlerquelle (17 §8)
// ---------------------------------------------------------------------------
// Die Objekte kommen aus dem Pool des ChefZ_EventBus und sind NACH Raise()
// ungueltig. Ein Abonnent, der sich einen ref darauf merkt, liest beim
// naechsten Ereignis fremde Daten - deshalb steht diese Warnung hier und
// nicht nur in der Dokumentation.
//
//     WER DATEN BRAUCHT, KOPIERT SIE. In den Callback, nicht darueber hinaus.
//
// ---------------------------------------------------------------------------
// Warum eine Klasse und kein Param1..Param9 (17 E3)
// ---------------------------------------------------------------------------
// Vanilla nutzt an vielen Stellen Param-Tupel. Fuer eine oeffentliche API ist
// das falsch: jedes zusaetzliche Feld aendert den TYP und bricht jeden
// Abonnenten. Eine Klasse mit benannten Feldern kann wachsen, ohne dass ein
// bestehendes Comp-Modul neu gebaut werden muss - und genau das braucht ein
// Projekt, dessen Comp-Module in eigenen PBOs liegen.
//
// KEIN CONTENT: kein Gericht, keine Zutat, kein Fremdsystem wird hier benannt.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_EventArgs
{
    //! Der Name, unter dem dieses Ereignis ausgeloest wurde. Ein Abonnent, der
    //! mehrere Ereignisse mit demselben Callback bedient, unterscheidet sie
    //! hieran.
    string    eventId;

    //! Beteiligter Spieler; 0 = niemand. Bewusst die Identitaets-ID und kein
    //! PlayerBase - siehe Kopf, Grund 1 und 2.
    int       identityId;

    float     worldTime;

    //--- Was betroffen ist ---------------------------------------------------
    ChefZ_Sym subjectClass;          // Gericht, Zutat, Station
    ChefZ_Sym recipeOrTransform;
    ChefZ_Sym deviceClass;

    ChefZ_Sym stateBefore;
    ChefZ_Sym stateAfter;

    ChefZ_Sym qualityTier;
    float     qualityScore;

    int       amount;
    int       portionsLeft;

    //--- Listen. Nie null, aber haeufig leer ---------------------------------
    //
    // Sie werden nur gefuellt, wenn ueberhaupt jemand zuhoert - der Aufrufer
    // fragt vorher ChefZ_EventBus.HasSubscribers() (17 E2). Auf einem Server
    // ohne Comp-Module entsteht hier nie ein Eintrag.
    ref array<ChefZ_Sym> consumedClasses;
    ref array<ChefZ_Sym> producedClasses;

    //! Effekt-IDs aus dem Rezept. Der Core wertet sie NIE aus (08 §2) - er
    //! reicht sie hier durch, damit ein Comp-Modul sie auswerten kann.
    ref array<string>    effects;

    ref array<ChefZ_Sym> tags;

    //--- Weltbezug NUR ueber Netz-IDs (17 §3.1, E4) --------------------------
    int subjectNetIdLow;
    int subjectNetIdHigh;
    int deviceNetIdLow;
    int deviceNetIdHigh;

    //--- Rueckkanal ----------------------------------------------------------
    //
    // Ausgewertet NUR bei ausdruecklich stornierbaren bzw. Abfrage-Events
    // (17 §3.1). Bei allen anderen setzt der Bus beides nach dem Aufruf
    // zurueck und warnt einmal je Abonnent (17 §9).
    bool   cancelled;
    string cancelReason;

    //! NUR bei QUALITY_BONUS_QUERY. Abonnenten ADDIEREN darauf (AddBonus),
    //! sie setzen nicht. Der Core klemmt die Summe auf
    //! maxExternalQualityBonus (17 §5).
    float  bonusPoints;

    void ChefZ_EventArgs()
    {
        consumedClasses = new array<ChefZ_Sym>();
        producedClasses = new array<ChefZ_Sym>();
        effects         = new array<string>();
        tags            = new array<ChefZ_Sym>();
        Reset();
    }

    /**
     * Auf "nichts bekannt" zuruecksetzen.
     *
     * Die vier Listen werden GELEERT, nicht neu angelegt: die Objekte kommen
     * aus einem Pool (17 §8) und werden wiederverwendet. Eine Neuallokation
     * je Ereignis waere genau die Sorte Kosten, die 17 E2 vermeiden will.
     */
    void Reset()
    {
        eventId           = "";
        identityId        = 0;
        worldTime         = 0.0;

        subjectClass      = ChefZ_SymbolTable.INVALID;
        recipeOrTransform = ChefZ_SymbolTable.INVALID;
        deviceClass       = ChefZ_SymbolTable.INVALID;
        stateBefore       = ChefZ_SymbolTable.INVALID;
        stateAfter        = ChefZ_SymbolTable.INVALID;
        qualityTier       = ChefZ_SymbolTable.INVALID;

        qualityScore      = 0.0;
        amount            = 0;
        portionsLeft      = 0;

        consumedClasses.Clear();
        producedClasses.Clear();
        effects.Clear();
        tags.Clear();

        subjectNetIdLow   = 0;
        subjectNetIdHigh  = 0;
        deviceNetIdLow    = 0;
        deviceNetIdHigh   = 0;

        cancelled         = false;
        cancelReason      = "";
        bonusPoints       = 0.0;
    }

    //--------------------------------------------------------------------------
    // Fuellen - fuer den AUSLOESER, nicht fuer Abonnenten
    //--------------------------------------------------------------------------

    void SetSubjectNetId(int low, int high)
    {
        subjectNetIdLow  = low;
        subjectNetIdHigh = high;
    }

    void SetDeviceNetId(int low, int high)
    {
        deviceNetIdLow  = low;
        deviceNetIdHigh = high;
    }

    //! Dubletten sind erlaubt: zwei Steaks sind zwei Eintraege. Wer zaehlen
    //! will, zaehlt - wer Mengen will, liest amount.
    void AddConsumed(ChefZ_Sym cls)
    {
        if (ChefZ_SymbolTable.IsValid(cls))
            consumedClasses.Insert(cls);
    }

    void AddProduced(ChefZ_Sym cls)
    {
        if (ChefZ_SymbolTable.IsValid(cls))
            producedClasses.Insert(cls);
    }

    void AddEffect(string effect)
    {
        if (effect != "")
            effects.Insert(effect);
    }

    void AddTag(ChefZ_Sym tag)
    {
        if (!ChefZ_SymbolTable.IsValid(tag))
            return;
        if (tags.Find(tag) >= 0)
            return;
        tags.Insert(tag);
    }

    //--------------------------------------------------------------------------
    // Rueckkanal - fuer ABONNENTEN
    //--------------------------------------------------------------------------

    /**
     * Das Ereignis stornieren.
     *
     * Wirkt NUR bei ChefZ_EventNames.IsCancellable() (17 E5). Bei allen
     * anderen Ereignissen setzt der Bus das Feld nach dem Aufruf zurueck und
     * meldet einmal je Abonnent ein WARN - stillschweigend ignorieren waere
     * schlimmer, weil der fremde Modautor dann glaubt, seine Sperre wirke.
     *
     * Der Grund ist Pflicht in dem Sinn, dass ohne ihn im Trace steht, WER
     * storniert hat, aber nicht WARUM. Leer ist trotzdem erlaubt - eine
     * Stornierung ohne Grund ist besser als keine Stornierung.
     */
    void Cancel(string reason)
    {
        cancelled    = true;
        cancelReason = reason;
    }

    /**
     * Punkte zur Qualitaetsabfrage beitragen (17 §5).
     *
     * ADDIEREND und nicht setzend, und das ist der ganze Unterschied zwischen
     * einer Query und einer Hintertuer: mehrere Abonnenten koennen beitragen,
     * keiner kann die Beitraege der anderen loeschen, und die Summe wird vom
     * Core geklemmt. Es gibt bewusst kein SetQuality().
     *
     * NaN und Unendlich werden hier verworfen, nicht erst beim Klemmen: eine
     * einzige NaN-Addition wuerde die Summe unrettbar machen, und die
     * Zuordnung zum Verursacher waere danach verloren.
     */
    void AddBonus(float points)
    {
        if (!IsFinite(points))
            return;
        bonusPoints = bonusPoints + points;
    }

    /**
     * Ist das ueberhaupt eine Zahl?
     *
     * Wortgleich zu ChefZ_QualityEvaluation.IsFinite und bewusst NICHT von
     * dort geliehen: die Ereignisschicht ist die oeffentliche Aussenkante des
     * Core und soll fuer eine dreizeilige Zahlenpruefung nicht auf ein
     * Qualitaetsdetail zeigen muessen. NaN ist der einzige Wert, der sich
     * selbst ungleich ist; der zweite Teil faengt Unendlichkeiten ab.
     */
    static bool IsFinite(float v)
    {
        if (v != v)
            return false;
        if (v > float.MAX || v < float.LOWEST)
            return false;
        return true;
    }

    //--------------------------------------------------------------------------

    bool HasSubject()
    {
        return subjectNetIdLow != 0 || subjectNetIdHigh != 0;
    }

    bool HasDevice()
    {
        return deviceNetIdLow != 0 || deviceNetIdHigh != 0;
    }

    /**
     * Eine Zeile fuer Log und Trace.
     *
     * Baut Zeichenketten und gehoert deshalb hinter eine ChefZ_Log.Enabled-
     * Wache (18 E2) - ausnahmslos, auch im Ereignispfad.
     */
    string ToDebugString()
    {
        string s = eventId;

        if (ChefZ_SymbolTable.IsValid(recipeOrTransform))
            s = s + " rezept=" + ChefZ_SymbolTable.Name(recipeOrTransform);
        if (ChefZ_SymbolTable.IsValid(subjectClass))
            s = s + " subjekt=" + ChefZ_SymbolTable.Name(subjectClass);
        if (ChefZ_SymbolTable.IsValid(deviceClass))
            s = s + " geraet=" + ChefZ_SymbolTable.Name(deviceClass);

        if (ChefZ_SymbolTable.IsValid(stateBefore) || ChefZ_SymbolTable.IsValid(stateAfter))
        {
            s = s + " zustand=" + ChefZ_SymbolTable.NameOrMark(stateBefore) + "->" + ChefZ_SymbolTable.NameOrMark(stateAfter);
        }
        if (ChefZ_SymbolTable.IsValid(qualityTier))
        {
            s = s + " stufe=" + ChefZ_SymbolTable.Name(qualityTier) + "/" + qualityScore.ToString();
        }

        if (identityId != 0)
            s = s + " spieler=" + identityId.ToString();
        if (amount != 0)
            s = s + " menge=" + amount.ToString();
        if (portionsLeft != 0)
            s = s + " portionen=" + portionsLeft.ToString();

        if (consumedClasses.Count() > 0)
            s = s + " verbraucht[" + ChefZ_TextList.JoinSymbols(consumedClasses, ",") + "]";
        if (producedClasses.Count() > 0)
            s = s + " erzeugt[" + ChefZ_TextList.JoinSymbols(producedClasses, ",") + "]";
        if (effects.Count() > 0)
            s = s + " effekte[" + ChefZ_TextList.Join(effects, ",") + "]";

        if (cancelled)
            s = s + " STORNIERT(" + cancelReason + ")";
        if (bonusPoints != 0.0)
            s = s + " bonus=" + bonusPoints.ToString();

        return s;
    }

    //! Nur fuer den Selbsttest (S13).
    static bool SelfCheck()
    {
        ChefZ_EventArgs a = new ChefZ_EventArgs();
        if (a.eventId != "")                            return false;
        if (a.cancelled)                                return false;
        if (a.bonusPoints != 0.0)                       return false;
        if (a.consumedClasses.Count() != 0)             return false;
        if (a.HasSubject())                             return false;

        ChefZ_Sym s1 = ChefZ_SymbolTable.Intern("CHEFZ_EV_A");
        a.AddConsumed(s1);
        a.AddConsumed(s1);                              // Dubletten erlaubt
        a.AddConsumed(ChefZ_SymbolTable.INVALID);       // INVALID nie
        if (a.consumedClasses.Count() != 2)             return false;

        a.AddTag(s1);
        a.AddTag(s1);                                   // Tags dedupliziert
        if (a.tags.Count() != 1)                        return false;

        a.AddEffect("");
        a.AddEffect("CHEFZ_EV_EFFEKT_A");
        if (a.effects.Count() != 1)                     return false;

        // Additiv, nicht setzend - der Kern von 17 §5.
        a.AddBonus(1.5);
        a.AddBonus(0.5);
        if (a.bonusPoints != 2.0)                       return false;

        a.Cancel("kein Skill");
        if (!a.cancelled)                               return false;
        if (a.cancelReason != "kein Skill")             return false;

        a.SetSubjectNetId(7, 0);
        if (!a.HasSubject())                            return false;

        a.Reset();
        if (a.cancelled)                                return false;
        if (a.bonusPoints != 0.0)                       return false;
        if (a.tags.Count() != 0)                        return false;
        if (a.HasSubject())                             return false;

        if (!IsFinite(1.0))                             return false;
        if (!IsFinite(0.0))                             return false;
        if (!IsFinite(float.MAX))                       return false;
        if (!IsFinite(float.LOWEST))                    return false;

        return true;
    }
}
