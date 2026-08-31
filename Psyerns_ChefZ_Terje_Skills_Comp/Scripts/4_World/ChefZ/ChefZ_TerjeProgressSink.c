// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeProgressSink - die XP-Vergabe fuer Kochen und Verarbeiten
//
// Angemeldet ueber ChefZ_ProgressRegistry.RegisterSink() (ChefZ_Core/
// Scripts/3_Game/ChefZ/ChefZ_ProgressRegistry.c). Das ist die vom Core
// ausdruecklich dafuer vorgesehene Schnittstelle - und zugleich die einzige,
// die Regel "XP nur nach erfolgreichem Abschluss" strukturell garantiert:
// Report() steht im Core ausschliesslich hinter einem vollzogenen Abschluss,
// nach dem Verbrauch der Zutaten und der Erzeugung des Ergebnisses. Es gibt
// keinen Aufruf beim Einlegen und keinen beim Start - eine XP-Schleife durch
// Einlegen und Entnehmen ist deshalb nicht baubar, nicht einmal absichtlich.
//
// ---------------------------------------------------------------------------
// WELCHE ANLAESSE HIER XP GEBEN - UND WELCHE AUSDRUECKLICH NICHT
// ---------------------------------------------------------------------------
//   "cook"     JA. Ein Gericht ist entstanden, die Zutaten sind verbraucht.
//   "process"  JA. Ein Verarbeitungsschritt ist abgeschlossen.
//
//   "preserve" NEIN, und das ist die wichtigste Zeile dieser Datei.
//              Raeuchern, Trocknen und Salzen laufen als TRANSFORM durch den
//              ChefZ_ProcessRunner und melden dort bereits "process". Der
//              Wechsel in einen preserved-Zustand meldet ZUSAETZLICH
//              "preserve" (ChefZ_ItemStateComponent.c:446). Wer beide
//              annimmt, zahlt Trockenfleisch und Raeucherwurst DOPPELT.
//              Hinzu kommt: die preserve-Nutzlast traegt keine identityId
//              (RaiseStateChanged setzt sie nie) - es gaebe gar keinen
//              Spieler, dem man sie gutschreiben koennte.
//
//   "consume"  NEIN. Essen gibt Metabolism-XP automatisch ueber
//              TerjeSkills/Scripts/4_World/Classes/PlayerStomach.c
//              (AddToStomach -> AddSkillExperience("mtblsm", ...)). Eine
//              zweite Vergabe waere genau die Dopplung, die Regel 3 verbietet.
//
//   "discover" NEIN. Ein erstmals gelungenes Rezept hat bereits ueber "cook"
//              gezahlt; "discover" kommt im selben Vorgang obendrauf.
//
// ---------------------------------------------------------------------------
// SYSTEMGRENZEN
// ---------------------------------------------------------------------------
// Alles hier zahlt in "surv" ein, ausnahmslos. Tierzerlegung gehoert Terje
// Hunting (PrepareAnimal.c), Fischfiletieren Terje Fishing (PrepareFish.c) -
// beide werden hier nicht angefasst. Die Wurstherstellung ist
// WEITERVERARBEITUNG und gibt deshalb Survival-XP, niemals Hunting-XP.
//
// ---------------------------------------------------------------------------
// Lebensdauer der Nutzlast
// ---------------------------------------------------------------------------
// args gehoert dem Core und ist nach der Rueckkehr aus OnChefZProgress
// UNGUELTIG (17 §8). Diese Klasse haelt keinen ref darauf; alles, was
// gebraucht wird, wird im Rueckruf ausgelesen.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_TerjeProgressSink extends ChefZ_IProgressSink
{
    override string GetSinkName()
    {
        return "ChefZ_TerjeSkills";
    }

    override void OnChefZProgress(string progressKind, notnull ChefZ_EventArgs args)
    {
        // Nur der Server vergibt Erfahrung. Auf einem Listen-Server laeuft
        // beides im selben Prozess; die Abfrage haelt den reinen Client
        // heraus.
        if (!g_Game || !g_Game.IsServer())
            return;

        if (!ChefZ_TerjeSkillsConfig.IsXpEnabled())
            return;

        // Ohne Spieler gibt es niemanden, dem etwas gutzuschreiben waere.
        // Serveraktionen und Bots laufen laut ChefZ_ActionProcessAtStation.
        // IdentityOf() als identityId 0.
        if (args.identityId == 0)
            return;

        int    xp  = 0;
        string key = "";

        // Der Schluessel der Wiederholungsdaempfung. Beim Kochen ist das die
        // Rezept-ID, beim Verarbeiten seit dem 31.08.2026 die PROZESS-ID und
        // nicht mehr die Transform-ID - die Begruendung steht unten an
        // ProcessDamperKey().
        string damperKey = "";

        // Name() und nicht NameOrMark(): ein leerer Name heisst "der Core
        // konnte den Vorgang nicht benennen". Dann gibt es hier nichts -
        // weder eine sinnvolle Ausnahmeliste noch einen Schluessel fuer die
        // Wiederholungsdaempfung, und eine unbenennbare Aktion soll nicht
        // stillschweigend die volle Vorgabe kassieren.
        if (progressKind == ChefZ_ProgressKind.COOK)
        {
            key = ChefZ_SymbolTable.Name(args.recipeOrTransform);
            if (key == "")
                return;
            xp = CookXp(args, key);
            damperKey = key;
        }
        else if (progressKind == ChefZ_ProgressKind.PROCESS)
        {
            key = ChefZ_SymbolTable.Name(args.recipeOrTransform);
            if (key == "")
                return;

            // EINMAL aufgeloest und zweimal gebraucht: fuer die Einstufung
            // (welcher XP-Wert) und fuer den Daempferschluessel (welcher Topf).
            string processId = ProcessIdOf(args.recipeOrTransform);

            xp = ProcessXp(key, processId);
            damperKey = ProcessDamperKey(key, processId);
        }
        else
        {
            // "preserve", "consume", "discover" - siehe Kopf. Bewusst kein
            // Log: das sind haeufige, voellig normale Ereignisse.
            return;
        }

        if (xp <= 0)
            return;

        // Mengenbonus VOR der Wiederholungsdaempfung: der Bonus gehoert zur
        // Leistung, die Daempfung bewertet die Wiederholung dieser Leistung.
        int produced = 0;
        if (args.producedClasses)
            produced = args.producedClasses.Count();

        xp = xp + ChefZ_TerjeXpDamper.BatchBonus(xp, produced);

        // ZAEHLT MIT - deshalb erst hier, wenn feststeht, dass es XP gibt.
        int percent = ChefZ_TerjeXpDamper.RepeatPercent(args.identityId, damperKey);
        if (percent < 100)
        {
            xp = (xp * percent) / 100;

            // Eine gedaempfte, aber erfolgreiche Aktion soll nie ganz leer
            // ausgehen, solange die Daempfung nicht auf 0 % steht.
            if (xp < 1 && percent > 0)
                xp = 1;
        }

        if (xp <= 0)
            return;

        PlayerBase player = ChefZ_TerjeSkillsBridge.FindPlayerByIdentityId(args.identityId);
        if (!player)
            return;

        if (!ChefZ_TerjeSkillsBridge.AddSurvivalXp(player, xp,
                ChefZ_TerjeSkillsConfig.ShowNotification()))
            return;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
        {
            string line = "TerjeSkills: +" + xp.ToString() + " surv fuer " + progressKind + " \"" + key;
            line = line + "\" (Spieler " + args.identityId.ToString() + ", Ergebnisse " + produced.ToString();
            line = line + ", Daempfung " + percent.ToString() + "% auf \"" + damperKey + "\")";
            ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, line);
        }
    }

    //==========================================================================
    // Einstufung
    //==========================================================================

    /**
     * XP fuer ein fertiges Gericht.
     *
     * Erst die Ausnahmeliste je Rezept-ID, dann die Einstufung nach der Zahl
     * der TATSAECHLICH verbrauchten Zutaten. Warum diese Zahl: sie ist die
     * einzige Angabe zur Aufwendigkeit, die in der Abschlussnutzlast des Core
     * verlaesslich steht (ChefZ_CookingDeviceAdapter.FillCookArgs fuellt
     * consumedClasses aus dem Verbrauchsplan). Ein "premium"-Merkmal am
     * Gericht gibt es in den Rezeptdaten nicht - CHEFZ_PREMIUM sitzt an
     * ZUTATEN, nicht an Ergebnissen.
     *
     * Der Betreiber kann jedes Rezept einzeln festnageln, wenn ihm die
     * Einstufung nicht passt.
     */
    private int CookXp(notnull ChefZ_EventArgs args, string recipeId)
    {
        int inputs = 0;
        if (args.consumedClasses)
            inputs = args.consumedClasses.Count();

        int classified;
        if (inputs <= ChefZ_TerjeSkillsConfig.CookSimpleMax())
            classified = ChefZ_TerjeSkillsConfig.CookSimpleXp();
        else if (inputs <= ChefZ_TerjeSkillsConfig.CookComplexMax())
            classified = ChefZ_TerjeSkillsConfig.CookComplexXp();
        else
            classified = ChefZ_TerjeSkillsConfig.CookPremiumXp();

        return ChefZ_TerjeSkillsConfig.CookRecipeXp(recipeId, classified);
    }

    /**
     * Der PROZESS zu einer Transform, oder "".
     *
     * Der Prozess steht nicht in der Nutzlast: ChefZ_ProcessRunner.c:896
     * setzt recipeOrTransform auf die TRANSFORM. Der Weg zum Prozess fuehrt
     * ueber ChefZ_ProcessingManager.GetTransform(sym).processSym - dieselbe
     * Auskunftsstelle, die der Runner selbst benutzt.
     *
     * "" ist ein normaler Rueckgabewert und kein Fehler: solange der
     * ChefZ_ProcessingManager nicht bereit ist oder die Transform ihm
     * unbekannt ist, gibt es keinen Prozessnamen. Beide Aufrufer haben fuer
     * diesen Fall einen Rueckfallweg auf die Transform-ID.
     */
    private string ProcessIdOf(ChefZ_Sym transformSym)
    {
        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!mgr || !mgr.IsReady())
            return "";

        ChefZ_CompiledTransform tr = mgr.GetTransform(transformSym);
        if (!tr)
            return "";

        return ChefZ_SymbolTable.Name(tr.processSym);
    }

    /**
     * XP fuer einen abgeschlossenen Verarbeitungsschritt.
     *
     * Zuerst der PROZESS (PROCESS_DRY, PROCESS_SMOKE ...), weil die XP-Matrix
     * aus §26 nach Taetigkeiten und nicht nach Einzelrezepturen geordnet ist.
     * Eine einzelne Transform kann das ueberschreiben - dafuer gibt es
     * ChefZ_Transforms in der Config, und dort stehen die beiden Faelle, in
     * denen §26 eine Kette anders bewertet als ihre Schritte.
     */
    private int ProcessXp(string transformId, string processId)
    {
        int fallback = ChefZ_TerjeSkillsConfig.ProcessDefaultXp();

        if (processId != "")
            fallback = ChefZ_TerjeSkillsConfig.ProcessXp(processId, fallback);

        return ChefZ_TerjeSkillsConfig.TransformXp(transformId, fallback);
    }

    /**
     * Der Schluessel, unter dem die Wiederholungsdaempfung mitzaehlt.
     *
     * -------------------------------------------------------------------------
     * WARUM DER PROZESS UND NICHT DIE TRANSFORM (Balance-Befund B-5,
     * 31.08.2026)
     * -------------------------------------------------------------------------
     * Bis dahin zaehlte die Daempfung je TRANSFORM. Das Wildwuchs-System
     * (Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md, 31.08.2026) hat daraus
     * eine Luecke gemacht: die CE stellt Thymian, Rosmarin und Petersilie im
     * selben Ring auf, und ihre Trocknungen sind DREI Transforms
     * (TR_ThymeToDried, TR_RosemaryToDried, TR_ParsleyToDried,
     * ChefZ_Processing/Config/Processing/HerbDrying.json). Wer sie abwechselnd
     * trocknete, hatte damit dreimal repeatFreeCount freie Durchlaeufe je
     * Fenster - 15 statt 5 - fuer eine Taetigkeit, die sich nur im Beutel
     * unterscheidet, den man einlegt.
     *
     * Der Prozess ist die richtige Zaehleinheit: die XP-Matrix aus §26 ist nach
     * TAETIGKEITEN geordnet ("Kraeuter trocknen 3"), nicht nach Zutaten, und
     * die Daempfung bewertet die Wiederholung einer Taetigkeit.
     *
     * -------------------------------------------------------------------------
     * WAS DAS SONST NOCH STRAFFT - alle Faelle, nicht nur die drei Kraeuter
     * -------------------------------------------------------------------------
     * PROCESS_DRY fasst im heutigen Datenbestand ZWOELF Transforms zusammen:
     * sechs Kraeuter- und Gewuerztrocknungen (Thymian, Rosmarin, Petersilie,
     * Baerlauch, Paprika, Pfefferbeeren), zwei Vanilla-Beeren (Canina,
     * Sambucus), Nudeln, gesalzenes Fleisch, gesalzener Fisch und Rohwurst.
     * Sie teilen sich ab jetzt EINEN Topf. Wer erst Fleisch und dann Kraeuter
     * trocknet, kommt also frueher in die Daempfung als bisher.
     *
     * Das ist gewollt und nicht bloss hingenommen: es ist derselbe Handgriff an
     * derselben Station: Ware hinein, warten, Ware heraus. Wer die Daempfung
     * meiden will, soll die TAETIGKEIT wechseln - mahlen, kneten, raeuchern -,
     * nicht die Zutat.
     *
     * Gleichfalls betroffen, aber unauffaellig: PROCESS_STUFF_SAUSAGE (6),
     * PROCESS_GRIND_MEAT (6), PROCESS_GRIND_SPICE (4), PROCESS_CUT_MEAT (4),
     * PROCESS_SMOKE (2), PROCESS_SALT_CURE (2), PROCESS_MILL (2). Alle
     * uebrigen Prozesse fuehren genau eine Transform; fuer sie aendert sich
     * nichts ausser dem Namen im Debuglog.
     *
     * NICHT betroffen ist das KOCHEN. Dort bleibt die Rezept-ID der
     * Schluessel: zwei Rezepte sind zwei Gerichte, und ein gemeinsamer Topf
     * "Kochen" wuerde jeden bestrafen, der abwechslungsreich kocht.
     *
     * -------------------------------------------------------------------------
     * Rueckfall
     * -------------------------------------------------------------------------
     * Ist der Prozess nicht ermittelbar (ProcessingManager noch nicht bereit,
     * Transform unbekannt), bleibt es bei der Transform-ID. Lieber eine
     * Daempfung, die feiner zaehlt als gewollt, als gar keine: ein leerer
     * Schluessel schaltet RepeatPercent() vollstaendig ab
     * (ChefZ_TerjeXpDamper.c, "if (identityId == 0 || key == \"\")").
     */
    private string ProcessDamperKey(string transformId, string processId)
    {
        if (processId != "")
            return processId;

        return transformId;
    }
}
#endif // TERJE_SKILLS_MOD
