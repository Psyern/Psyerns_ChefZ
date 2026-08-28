// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeXpDamper - Anti-XP-Farming, Terje-Analyse §27
//
// ---------------------------------------------------------------------------
// Was §27 verlangt und wo es tatsaechlich durchgesetzt wird
// ---------------------------------------------------------------------------
// §27 nennt drei Dinge. Zwei davon stehen NICHT in dieser Datei, weil sie
// woanders besser aufgehoben sind:
//
//   1. "XP nur nach abgeschlossener Produktion."
//      Durchgesetzt im ChefZ-Core, strukturell: ChefZ_ProgressRegistry.Report()
//      wird ausschliesslich hinter einem vollzogenen Abschluss gerufen
//      (ChefZ_ProcessRunner.c:949, ChefZ_CookingDeviceAdapter.c:1038). Es gibt
//      keinen Aufruf beim Einlegen, beim Start oder beim Match - ein Modul,
//      das nur diese Schnittstelle benutzt, KANN die Regel nicht verletzen.
//      Der Kopf von ChefZ_ProgressRegistry.c sagt das woertlich.
//
//   2. "Kein Ereignis fuer den blossen Zustandswechsel."
//      Ebenfalls Core: ChefZ_ItemStateComponent.RaiseStateChanged meldet
//      Fortschritt NUR bei preserved-Zustaenden, und der Kommentar dort nennt
//      den Grund ("ein Item, das von RAW nach RAW_CHOPPED und zurueck geht,
//      waere sonst eine XP-Schleife").
//
//   3. "Bei Batch-Produktion nicht automatisch volle XP je Stueck."
//      Das ist ChefZ_TerjeXpDamper.BatchBonus() - hier.
//
// Dazu kommt eine vierte, die §27 nicht ausdruecklich fordert, die aber der
// einzige verbleibende Hebel ist: die WIEDERHOLUNGSDAEMPFUNG. Wer denselben
// Schritt in kurzer Folge zwanzigmal ausfuehrt, bekommt ab dem n-ten Mal
// weniger. Das trifft das Stapeln von Zutaten am Fliessband und laesst
// normales Kochen unberuehrt.
//
// ---------------------------------------------------------------------------
// Zum Speicherverhalten
// ---------------------------------------------------------------------------
// Der Zaehler ist bewusst FLUECHTIG. Er wird nicht gespeichert, nicht
// synchronisiert und ueberlebt keinen Serverneustart. Eine Persistenz waere
// eine zweite Datenhaltung fuer eine Daempfung, die nach repeatWindowSec
// ohnehin verfaellt - Aufwand ohne Gegenwert, und ein weiterer Block in
// OnStoreSave, den ChefZ ausdruecklich nicht will.
//
// Die Menge waechst nicht unbegrenzt: Schluessel sind Rezept- und
// Transform-IDs (eine feste Menge), Zeilen sind angemeldete Spieler, und
// abgelaufene Eintraege werden beim naechsten Zugriff desselben Spielers
// entfernt.
//
// Layer: 4_World.
//==============================================================================

//! Ein Zaehler je Spieler und Aktion.
class ChefZ_TerjeXpRepeat
{
    int   count;
    float lastTime;

    void ChefZ_TerjeXpRepeat()
    {
        count = 0;
        lastTime = 0.0;
    }
}

//==============================================================================

class ChefZ_TerjeXpDamper
{
    //! identityId -> (Aktionsschluessel -> Zaehler).
    private static ref map<int, ref map<string, ref ChefZ_TerjeXpRepeat>> s_ByPlayer;

    //! Obergrenze je Spieler. Erreicht sie ein Spieler, wird seine Zeile
    //! geleert statt zu wachsen. Der einzige Effekt ist, dass die Daempfung
    //! einmal von vorn beginnt - das ist harmlos und begrenzt den Speicher
    //! hart.
    private static const int MAX_KEYS_PER_PLAYER = 128;

    private static void EnsureInit()
    {
        if (!s_ByPlayer)
            s_ByPlayer = new map<int, ref map<string, ref ChefZ_TerjeXpRepeat>>();
    }

    //==========================================================================
    // Mengenbonus (§27)
    //==========================================================================

    /**
     * Der Zuschlag fuer eine Mehrfachproduktion.
     *
     * NICHT baseXp * Stueckzahl. Der Zuschlag ist doppelt gedeckelt:
     *
     *   - auf batchMaxUnits zusaetzliche Einheiten (Vorgabe 3), und
     *   - auf batchCapPercent der Basis-XP (Vorgabe 50 %).
     *
     * Bei 10 Wuersten auf einmal (Basis 5) ergibt das 5 + 2 = 7 statt 50.
     * Bei einer einzelnen Zwiebel (Basis 1) ergibt es 1 + 0 = 1.
     *
     * @param producedCount wie viele Ergebnis-Items entstanden sind.
     */
    static int BatchBonus(int baseXp, int producedCount)
    {
        if (baseXp <= 0)
            return 0;

        int extraUnits = producedCount - 1;
        if (extraUnits <= 0)
            return 0;

        int maxUnits = ChefZ_TerjeSkillsConfig.BatchMaxUnits();
        if (extraUnits > maxUnits)
            extraUnits = maxUnits;

        int bonus = extraUnits * ChefZ_TerjeSkillsConfig.BatchBonusPerUnit();

        int cap = (baseXp * ChefZ_TerjeSkillsConfig.BatchCapPercent()) / 100;
        if (bonus > cap)
            bonus = cap;

        if (bonus < 0)
            bonus = 0;

        return bonus;
    }

    //==========================================================================
    // Wiederholungsdaempfung
    //==========================================================================

    /**
     * Faktor in Prozent fuer die naechste Vergabe, und der Zaehler wird dabei
     * fortgeschrieben.
     *
     * Aufrufreihenfolge ist wichtig: diese Funktion ZAEHLT MIT. Sie darf
     * deshalb erst gerufen werden, wenn feststeht, dass tatsaechlich XP
     * vergeben wird - sonst zaehlte ein Vorgang mit 0 XP die Daempfung fuer
     * einen spaeteren echten Vorgang hoch.
     *
     * @param key  Rezept- oder Transform-ID. Verschiedene Aktionen daempfen
     *             sich gegenseitig NICHT: wer abwechselnd Brot backt und
     *             Wurst stopft, wird nicht bestraft. Gedaempft wird das
     *             stumpfe Wiederholen DESSELBEN Schritts.
     */
    static int RepeatPercent(int identityId, string key)
    {
        int freeCount = ChefZ_TerjeSkillsConfig.RepeatFreeCount();
        if (freeCount <= 0)
            return 100;                     // Daempfung abgeschaltet
        if (identityId == 0 || key == "")
            return 100;

        EnsureInit();

        float now = 0.0;
        if (GetGame())
            now = GetGame().GetTime() * 0.001;

        map<string, ref ChefZ_TerjeXpRepeat> row;
        if (!s_ByPlayer.Find(identityId, row) || !row)
        {
            row = new map<string, ref ChefZ_TerjeXpRepeat>();
            s_ByPlayer.Set(identityId, row);
        }

        float window = ChefZ_TerjeSkillsConfig.RepeatWindowSec();
        PruneRow(row, now, window);

        if (row.Count() >= MAX_KEYS_PER_PLAYER)
            row.Clear();

        ChefZ_TerjeXpRepeat entry;
        if (!row.Find(key, entry) || !entry)
        {
            entry = new ChefZ_TerjeXpRepeat();
            row.Set(key, entry);
        }

        // Der Zaehler beginnt neu, sobald der Spieler diese eine Aktion lange
        // genug nicht mehr ausgefuehrt hat.
        if (window > 0.0 && entry.count > 0 && (now - entry.lastTime) > window)
            entry.count = 0;

        entry.count = entry.count + 1;
        entry.lastTime = now;

        if (entry.count <= freeCount)
            return 100;

        int over = entry.count - freeCount;
        int percent = 100 - (over * ChefZ_TerjeSkillsConfig.RepeatStepPercent());

        int minPercent = ChefZ_TerjeSkillsConfig.RepeatMinPercent();
        if (percent < minPercent)
            percent = minPercent;
        if (percent < 0)
            percent = 0;

        return percent;
    }

    /**
     * Abgelaufene Eintraege eines Spielers entfernen.
     *
     * Ueber eine Namensliste und nicht im foreach: eine map waehrend der
     * Iteration zu veraendern ist der klassische Weg zu einem Absturz, der
     * erst nach Wochen auf einem vollen Server auffaellt.
     */
    private static void PruneRow(map<string, ref ChefZ_TerjeXpRepeat> row, float now, float window)
    {
        if (window <= 0.0)
            return;

        array<string> stale = new array<string>();
        for (int i = 0; i < row.Count(); i++)
        {
            ChefZ_TerjeXpRepeat e = row.GetElement(i);
            if (!e)
            {
                stale.Insert(row.GetKey(i));
                continue;
            }
            if ((now - e.lastTime) > window)
                stale.Insert(row.GetKey(i));
        }

        for (int s = 0; s < stale.Count(); s++)
            row.Remove(stale.Get(s));
    }

    //! Wenn ein Spieler den Server verlaesst. Nicht zwingend - die Eintraege
    //! verfallen ohnehin -, aber ein voller Server soll keine Zeilen von
    //! Spielern mit sich herumtragen, die laengst weg sind.
    static void Forget(int identityId)
    {
        if (!s_ByPlayer)
            return;
        s_ByPlayer.Remove(identityId);
    }

    static void ClearAll()
    {
        if (s_ByPlayer)
            s_ByPlayer.Clear();
    }

    static int TrackedPlayers()
    {
        if (!s_ByPlayer)
            return 0;
        return s_ByPlayer.Count();
    }
}
#endif // TERJE_SKILLS_MOD
