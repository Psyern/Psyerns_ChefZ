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
        }
        else if (progressKind == ChefZ_ProgressKind.PROCESS)
        {
            key = ChefZ_SymbolTable.Name(args.recipeOrTransform);
            if (key == "")
                return;
            xp = ProcessXp(args, key);
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
        int percent = ChefZ_TerjeXpDamper.RepeatPercent(args.identityId, key);
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
            ChefZ_Log.Debug(ChefZ_LogChannel.EVENT,
                "TerjeSkills: +" + xp.ToString() + " surv fuer " + progressKind + " \"" + key + "\" (Spieler " + args.identityId.ToString() + ", Ergebnisse " + produced.ToString() + ", Daempfung " + percent.ToString() + "%)");
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
     * XP fuer einen abgeschlossenen Verarbeitungsschritt.
     *
     * Zuerst der PROZESS (PROCESS_DRY, PROCESS_SMOKE ...), weil die XP-Matrix
     * aus §26 nach Taetigkeiten und nicht nach Einzelrezepturen geordnet ist.
     * Eine einzelne Transform kann das ueberschreiben - dafuer gibt es
     * ChefZ_Transforms in der Config, und dort stehen die beiden Faelle, in
     * denen §26 eine Kette anders bewertet als ihre Schritte.
     *
     * Der Prozess steht nicht in der Nutzlast: ChefZ_ProcessRunner.c:896
     * setzt recipeOrTransform auf die TRANSFORM. Der Weg zum Prozess fuehrt
     * ueber ChefZ_ProcessingManager.GetTransform(sym).processSym - dieselbe
     * Auskunftsstelle, die der Runner selbst benutzt.
     */
    private int ProcessXp(notnull ChefZ_EventArgs args, string transformId)
    {
        int fallback = ChefZ_TerjeSkillsConfig.ProcessDefaultXp();

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (mgr && mgr.IsReady())
        {
            ChefZ_CompiledTransform tr = mgr.GetTransform(args.recipeOrTransform);
            if (tr)
            {
                string processId = ChefZ_SymbolTable.Name(tr.processSym);
                if (processId != "")
                    fallback = ChefZ_TerjeSkillsConfig.ProcessXp(processId, fallback);
            }
        }

        return ChefZ_TerjeSkillsConfig.TransformXp(transformId, fallback);
    }
}
#endif // TERJE_SKILLS_MOD
