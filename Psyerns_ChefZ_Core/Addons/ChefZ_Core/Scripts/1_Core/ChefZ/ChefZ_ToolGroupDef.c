//==============================================================================
// ChefZ_ToolGroupDef - Werkzeuge als DATEN, nie als Klassenliste im Code
//
// Entwurf: 11 E8 (woertlich: "Ein Prozess verlangt CUTTING_TOOL. Welche
// Klassen das sind, steht in CfgChefZTools"), 02 §4 und 02 §5.1 (die beiden
// Schreibweisen), 02 E3 (Bool-Sonde), Architekturplan §13.
//
// ---------------------------------------------------------------------------
// Zwei Schreibweisen, ein Record - und beide sind gemeint
// ---------------------------------------------------------------------------
//   gruppenweise (02 §5.1)   id = Gruppe,     classes[]        = Mitglieder
//   klassenweise (02 §4)     id = Klasse,     toolCategories[] = Gruppen
//
// Die ChefZ_ToolRegistry (3_Game) liest BEIDE und baut daraus einen einzigen
// Index Klasse -> Gruppen. Welche Schreibweise ein Content-Autor waehlt, ist
// damit seine Sache und keine Systemfrage:
//
//   - Ein Modul, das ein eigenes Messer mitbringt, schreibt klassenweise -
//     der Eintrag steht dann neben der Klasse, zu der er gehoert.
//   - Ein Serverbetreiber, der ein Messer aus einem FREMDEN Mod aufnehmen
//     will, schreibt gruppenweise - er kann die fremde config.cpp nicht
//     anfassen, wohl aber eine eigene Gruppe mit classes[] deklarieren.
//
// Genau der zweite Fall ist die Zusage aus 11 E8: der Core erfuellt
// Architekturplan §13, ohne je einen Messernamen zu tragen.
//
// ---------------------------------------------------------------------------
// Warum das in der GAME-CONFIG liegt und nicht in JSON (11 E8)
// ---------------------------------------------------------------------------
// ChefZ_ActionProcessAtStation.ActionCondition() laeuft auch auf dem CLIENT
// und muss dort entscheiden, ob ein passendes Werkzeug in der Hand liegt.
// Der Client liest Rang 1 (Game-Config) garantiert; ob er eine JSON-Datei aus
// einem PBO lesen kann, ist eine offene Messfrage (OF-10). Also gehoert die
// Werkzeugzuordnung nach CfgChefZTools.
//
// JSON bleibt trotzdem zulaessig - es ist derselbe Record. Ein Betreiber, der
// eine Gruppe per $profile-Overlay ergaenzt, aendert damit allerdings nur die
// SERVERSEITIGE Sicht; die Aktion erscheint dem Spieler dann eventuell nicht,
// obwohl der Server sie erlauben wuerde. Das steht hier, damit niemand es
// suchen muss.
//
// KEIN CONTENT: kein Werkzeugname, kein Gruppenname. "CUTTING_TOOL" ist ein
// Beispiel aus dem Entwurf und kommt im Code nicht vor.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_ToolGroupDef extends ChefZ_Record
{
    //! Gruppenweise Schreibweise: id ist die GRUPPE, hier stehen die Klassen.
    ref array<string> classes;

    //! Klassenweise Schreibweise: id ist die KLASSE, hier stehen die Gruppen.
    ref array<string> toolCategories;

    /**
     * Duerfen Unterklassen der genannten Klassen mitzaehlen?
     *
     * Default false, und das ist die vorsichtige Richtung: eine Gruppe, die
     * ungefragt jede Ableitung einsammelt, nimmt irgendwann ein Werkzeug auf,
     * von dem der Autor nie gehoert hat. Wer die Vererbung will, sagt es.
     */
    bool allowSubclasses;

    void ChefZ_ToolGroupDef()
    {
        classes         = null;
        toolCategories  = null;

        // bool ohne Sentinel: die Bool-Sonde traegt "allowSubclasses" in
        // explicitFields[] nach, wenn es in der Datei stand (02 E3). In der
        // Game-Config uebernimmt ChefZ_ConfigCppSource dieselbe Aufgabe -
        // dort IST die Anwesenheit des Eintrags die Aussage.
        allowSubclasses = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.TOOL_GROUP;
    }

    override void Normalize()
    {
        super.Normalize();
        ChefZ_TextList.TrimAll(classes);
        ChefZ_TextList.TrimAll(toolCategories);
    }

    /**
     * Ein Record, der WEDER classes[] NOCH toolCategories[] nennt, sagt gar
     * nichts - und wird abgewiesen.
     *
     * Er waere sonst eine Gruppe, der nie ein Werkzeug angehoert, und ein
     * Prozess, der sie verlangt, waere unausloesbar. Das ist genau die Sorte
     * Fehler, die wie "fehlender Content" aussieht: die Aktion erscheint
     * einfach nicht, und niemand weiss warum.
     *
     * Kein Fehler ist dagegen eine LEERE Liste - "classes": [] ist eine
     * ausdrueckliche Ansage ("diese Gruppe ist derzeit leer") und im
     * Overlay-Betrieb der einzige Weg, eine geerbte Liste zu raeumen.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!classes && !toolCategories)
        {
            if (ctx)
                ctx.Error(this, "Werkzeugeintrag nennt weder \"classes\" (gruppenweise, "
                    + "02 §5.1) noch \"toolCategories\" (klassenweise, 02 §4) - abgewiesen. "
                    + "Er waere eine Gruppe ohne Mitglieder, und jeder Prozess, der sie "
                    + "verlangt, waere unausloesbar.");
            return false;
        }

        return true;
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_ToolGroupDef s = ChefZ_ToolGroupDef.Cast(src);
        if (!s)
            return;
        classes         = PatchStringArray(classes, s.classes);
        toolCategories  = PatchStringArray(toolCategories, s.toolCategories);
        allowSubclasses = PatchBool(allowSubclasses, s.allowSubclasses, s, "allowSubclasses");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_ToolGroupDef o = ChefZ_ToolGroupDef.Cast(other);
        if (!o)
            return;
        if (allowSubclasses == o.allowSubclasses)
            MarkExplicit("allowSubclasses");
    }

    override void ResolveDefaults()
    {
        super.ResolveDefaults();
        if (!HasExplicit("allowSubclasses"))
            allowSubclasses = false;
    }

    //--------------------------------------------------------------------------

    //! Gruppenweise gelesen: id ist die Gruppe, classes[] sind die Mitglieder.
    bool DeclaresMembers()
    {
        return ChefZ_TextList.Count(classes) > 0;
    }

    //! Klassenweise gelesen: id ist die Klasse, toolCategories[] die Gruppen.
    bool DeclaresGroups()
    {
        return ChefZ_TextList.Count(toolCategories) > 0;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        ChefZ_ToolGroupDef stumm = new ChefZ_ToolGroupDef();
        stumm.id = "CHEFZ_TG_STUMM";
        if (stumm.Validate(ctx))                        return false;   // sagt nichts

        ChefZ_ToolGroupDef leer = new ChefZ_ToolGroupDef();
        leer.id      = "CHEFZ_TG_LEER";
        leer.classes = new array<string>();             // ausdruecklich leer
        if (!leer.Validate(ctx))                        return false;
        if (leer.DeclaresMembers())                     return false;
        if (leer.DeclaresGroups())                      return false;

        ChefZ_ToolGroupDef gruppe = new ChefZ_ToolGroupDef();
        gruppe.id      = "CHEFZ_TG_GRUPPE";
        gruppe.classes = new array<string>();
        gruppe.classes.Insert("  CHEFZ_TG_KLASSE  ");
        gruppe.Normalize();
        if (gruppe.classes.Get(0) != "CHEFZ_TG_KLASSE") return false;
        if (!gruppe.DeclaresMembers())                  return false;

        ChefZ_ToolGroupDef klasse = new ChefZ_ToolGroupDef();
        klasse.id             = "CHEFZ_TG_KLASSE";
        klasse.toolCategories = new array<string>();
        klasse.toolCategories.Insert("CHEFZ_TG_GRUPPE");
        if (!klasse.Validate(ctx))                      return false;
        if (!klasse.DeclaresGroups())                   return false;

        klasse.ResolveDefaults();
        if (klasse.allowSubclasses)                     return false;

        return true;
    }
}
