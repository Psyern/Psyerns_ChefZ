// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeSkillsBridge - die EINZIGE Stelle, die Terje anfasst
//
// Jeder Zugriff auf TerjeSkills laeuft durch diese Klasse. Nicht aus
// Ordnungsliebe, sondern weil damit genau eine Datei zu pruefen ist, wenn
// Terje sich aendert - und weil die Vorsichtsmassnahmen dann genau einmal
// dastehen statt an acht Aufrufstellen.
//
// ---------------------------------------------------------------------------
// Belegte Fundstellen (alles hier ist im Fremdcode nachgeschlagen)
// ---------------------------------------------------------------------------
//   TerjeCore/Scripts/4_World/Entities/PlayerBase.c:88
//       TerjePlayerSkillsAccessor GetTerjeSkills()
//       -> liefert null, solange der Spieler nicht lebt oder keine Identitaet
//          hat, und clientseitig nur fuer den lokal gesteuerten Spieler.
//
//   TerjeCore/Scripts/4_World/Classes/Skills/TerjePlayerSkillsAccessor.c
//       int  GetSkillLevel(string skillId)
//       void AddSkillExperience(string skillId, int value,
//                               bool affectModifiers, bool showNotification)
//       int  GetPerkLevel(string skillId, string perkId)
//       bool GetPerkValue(string skillId, string perkId, out float result)
//       bool IsPerkRegistered(string skillId, string perkId)
//
//   TerjeSkills/Scripts/4_World/Classes/Recipes/PrepareAnimal.c:87-93
//   TerjeSkills/Scripts/4_World/Classes/Recipes/PrepareFish.c:22-27
//       Das SICHERE Abfragemuster, das dieses Modul uebernimmt:
//           IsPerkRegistered(...)  - ist der Perk ueberhaupt eingerichtet
//           GetPerkValue(...)      - false heisst "Stufe 0", nicht "Fehler"
//       Terje selbst rechnet mit dem Rueckgabewert und nicht mit dem
//       out-Parameter allein; genau das wird hier nachgebaut.
//
//   TerjeSkills/survival.hpp:2   id="surv"
//
// ---------------------------------------------------------------------------
// Warum hier nirgends auf das Terje-Profil zugegriffen wird
// ---------------------------------------------------------------------------
// GetTerjeProfile() ist oeffentlich und waere schneller. Er ist trotzdem
// tabu: die Umrechnung Erfahrung -> Level -> aktive Perkstufe steht in
// TerjePlayerSkillsAccessor und beruecksichtigt Dinge, die von aussen nicht
// sichtbar sind (Perkstufe hoeher als der Skill erlaubt, abgeschaltete Perks,
// Erfahrungsmodifikatoren aus TerjeScriptableAreas). Ein zweiter Rechenweg
// waere ein zweiter, stiller Fehlerherd.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_TerjeSkillsBridge
{
    //! TerjeSkills/survival.hpp:2. Kochen, Konservieren und Kraeuterkunde
    //! zahlen ALLE hier ein - ein eigener Cooking-Skill ist fuer V1
    //! ausdruecklich nicht vorgesehen (Terje-Analyse §30).
    static const string SKILL_SURVIVAL = "surv";

    //! Terje-Analyse §6: die ID beginnt mit "chefz", damit sie mit keinem
    //! kuenftigen Terje-Perk kollidiert.
    static const string PERK_HERBALIST = "chefzherb";

    //--------------------------------------------------------------------------
    // Zugriff
    //--------------------------------------------------------------------------

    /**
     * Der Accessor eines Spielers, oder null.
     *
     * null ist ein voellig normaler Rueckgabewert - tote Spieler, Spieler
     * ohne Identitaet und (clientseitig) alle fremden Spieler liefern ihn.
     * Jeder Aufrufer muss ihn vertragen.
     */
    static TerjePlayerSkillsAccessor SkillsOf(PlayerBase player)
    {
        if (!player)
            return null;
        if (!player.IsAlive())
            return null;
        return player.GetTerjeSkills();
    }

    /**
     * Der Spieler zu einer ChefZ-Identitaets-ID.
     *
     * ChefZ_EventArgs.identityId ist PlayerIdentity.GetPlayerId() - siehe
     * ChefZ_ActionProcessAtStation.IdentityOf(). Der Core reicht bewusst
     * keine Entity durch (17 E4), deshalb wird hier aufgeloest.
     *
     * 0 heisst "niemand beteiligt" und liefert null, ohne die Spielerliste
     * ueberhaupt anzufassen.
     */
    static PlayerBase FindPlayerByIdentityId(int identityId)
    {
        if (identityId == 0)
            return null;
        if (!GetGame())
            return null;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players.Get(i));
            if (!p)
                continue;

            PlayerIdentity ident = p.GetIdentity();
            if (!ident)
                continue;

            if (ident.GetPlayerId() == identityId)
                return p;
        }

        return null;
    }

    //--------------------------------------------------------------------------
    // Erfahrung
    //--------------------------------------------------------------------------

    /**
     * Survival-Erfahrung gutschreiben.
     *
     * Ausschliesslich serverseitig. Terje prueft das in
     * TerjePlayerSkillsAccessor.AddSkillExperience zwar selbst noch einmal,
     * aber ein Aufruf vom Client waere trotzdem ein Denkfehler und soll hier
     * enden, nicht dort.
     *
     * value <= 0 wird verworfen: eine "Belohnung" von 0 XP erzeugt bei Terje
     * eine Notification ohne Inhalt.
     *
     * @return true, wenn tatsaechlich gutgeschrieben wurde.
     */
    static bool AddSurvivalXp(PlayerBase player, int value, bool showNotification)
    {
        if (value <= 0)
            return false;
        if (!GetGame() || !GetGame().IsServer())
            return false;

        TerjePlayerSkillsAccessor skills = SkillsOf(player);
        if (!skills)
            return false;

        // affectModifiers = true: der Betreiber soll seinen globalen
        // SKILLS_EXPERIENCE_GAIN_MODIFIER auch auf ChefZ-XP anwenden koennen,
        // genau wie Terje es fuer Jagd und Fischerei tut.
        skills.AddSkillExperience(SKILL_SURVIVAL, value, true, showNotification);
        return true;
    }

    //! Skill-Level, 0 wenn nicht ermittelbar.
    static int SurvivalLevel(PlayerBase player)
    {
        TerjePlayerSkillsAccessor skills = SkillsOf(player);
        if (!skills)
            return 0;
        return skills.GetSkillLevel(SKILL_SURVIVAL);
    }

    //--------------------------------------------------------------------------
    // Perks
    //--------------------------------------------------------------------------

    /**
     * Aktive Stufe eines Perks, 0..stagesCount.
     *
     * IsPerkRegistered() zuerst, wie in PrepareFish.c:22: ein Betreiber kann
     * den Perk ueber TerjeSettings abschalten, dann ist er nicht mehr
     * registriert und GetPerkLevel liefert ohnehin 0 - aber die Abfrage macht
     * die Absicht lesbar und spart den zweiten Aufruf.
     */
    static int PerkLevel(PlayerBase player, string skillId, string perkId)
    {
        TerjePlayerSkillsAccessor skills = SkillsOf(player);
        if (!skills)
            return 0;
        if (!skills.IsPerkRegistered(skillId, perkId))
            return 0;
        return skills.GetPerkLevel(skillId, perkId);
    }

    /**
     * Wert der aktiven Perkstufe (values[] aus der Config).
     *
     * Muster woertlich aus PrepareAnimal.c:87-93: liefert GetPerkValue false,
     * ist das KEIN Fehler, sondern "Stufe 0" - und dann ist der Wert 0.
     */
    static float PerkValue(PlayerBase player, string skillId, string perkId)
    {
        TerjePlayerSkillsAccessor skills = SkillsOf(player);
        if (!skills)
            return 0.0;
        if (!skills.IsPerkRegistered(skillId, perkId))
            return 0.0;

        float value = 0.0;
        if (!skills.GetPerkValue(skillId, perkId, value))
            return 0.0;

        return value;
    }

    //--------------------------------------------------------------------------
    // Kraeuterkundiger - die zwei Fragen, die dieses Modul wirklich stellt
    //--------------------------------------------------------------------------

    static int HerbalistLevel(PlayerBase player)
    {
        return PerkLevel(player, SKILL_SURVIVAL, PERK_HERBALIST);
    }

    /**
     * Ausbeutebonus als Anteil, 0.0 .. 0.5 nach der Config.
     *
     * Geklemmt, obwohl die Werte aus der eigenen config.cpp stammen: sie sind
     * ueber GameOverrides.xml vom Betreiber ueberschreibbar, und ein
     * verrutschter Wert soll die Ernte nicht vervielfachen.
     */
    static float HerbalistYield(PlayerBase player)
    {
        float v = PerkValue(player, SKILL_SURVIVAL, PERK_HERBALIST);
        return Math.Clamp(v, 0.0, 2.0);
    }
}
#endif // TERJE_SKILLS_MOD
