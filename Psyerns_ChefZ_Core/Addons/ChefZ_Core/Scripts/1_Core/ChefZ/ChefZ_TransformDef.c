//==============================================================================
// ChefZ_TransformDef - WORAUS wird WAS
//
// Entwurf: 11 §2 (Feldliste woertlich), 11 §2 ("Zustandswechsel ohne
// Klassenwechsel ist ein vollwertiger Output"), 11 §7 (Fehlerverhalten),
// 11 E4 (derselbe Selektor und dasselbe OutputDef wie Rezepte), 12 §6 / 14 §6
// (qualityRule, qualityDelta, freshnessCarry), 17 §3.3 (requires).
//
// ---------------------------------------------------------------------------
// E4 ist der ganze Sinn dieser Datei
// ---------------------------------------------------------------------------
// inputs[] sind ChefZ_SlotDef - dieselbe Klasse, die ein Rezept benutzt.
// outputs[] sind ChefZ_OutputDef - dieselbe Klasse, die ein Rezept benutzt.
// Es gibt kein zweites Bedingungsformat und kein zweites Ergebnisformat:
//
//     {"allOf":[{"category":"MEAT"},{"state":"SALTED"}]}
//
// bedeutet in einem Transform genau dasselbe wie in einem Rezept. Ein
// Content-Autor lernt die Sprache einmal, der Validator prueft sie einmal, der
// Compiler implementiert sie einmal - und der Applicator hat genau EINEN Pfad.
//
// ---------------------------------------------------------------------------
// Der Sonderfall "Output ohne Klasse" (11 §2)
// ---------------------------------------------------------------------------
// Ein ChefZ_OutputDef mit leerem "cls" und gesetztem "setState" laesst das
// Eingangsitem bestehen und wechselt nur seinen ChefZ-Zustand. Das ist beim
// REZEPT ein Fehler (ein Gericht ohne Klasse ist kein Gericht) und beim
// TRANSFORM erlaubt - und genau deshalb hat der Verarbeitungspfad einen
// eigenen Compiler statt den Rezeptcompiler mitzubenutzen.
//
// Die Rohform kann diesen Fall nicht abschliessend pruefen: sie sieht nicht,
// ob die Eingangsslots "consume": "none" fuehren. Diese Bedingung prueft der
// ChefZ_ProcessCompiler, weil sie erst nach ResolveDefaults der Slots
// entscheidbar ist.
//
// KEIN CONTENT: kein Prozessname, keine Station, keine Klasse, kein Zustand.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_TransformDef extends ChefZ_Record
{
    //--- WELCHER Prozess -----------------------------------------------------
    string                         process;

    //--- WAS geht hinein (07, dasselbe Slotmodell wie Rezepte) ---------------
    ref array<ref ChefZ_SlotDef>   inputs;

    //--- WAS kommt heraus (08 §2, dasselbe Outputmodell wie Rezepte) --------
    ref array<ref ChefZ_OutputDef> outputs;
    ref array<ref ChefZ_OutputDef> byproducts;

    //--- WO (11 E5: leer = jede Station, die den Prozess kann) ---------------
    ref array<string>              stationsAllowed;

    //--- WIE LANGE ------------------------------------------------------------
    //! Sentinel = "keine Angabe", dann gilt ChefZ_ProcessDef.baseDurationSec.
    //! 0.0 waere hier KEIN brauchbarer Default: es hiesse "sofort fertig" und
    //! saehe wie ein vergessenes Feld aus.
    float                          durationOverrideSec;

    //--- WIE GUT --------------------------------------------------------------
    string                         qualityRule;      // MIN | MEAN | WEIGHTED_MEAN | MAX
    float                          freshnessCarry;   // Default 1.0
    float                          qualityDelta;     // Rangaenderung, z.B. +0

    //--- Feinjustierung --------------------------------------------------------
    int                            priority;

    //--- WER darf es -----------------------------------------------------------
    ref array<ref ChefZ_CapabilityReq> requires;

    void ChefZ_TransformDef()
    {
        process             = ChefZ_Undefined.TEXT;
        qualityRule         = ChefZ_Undefined.TEXT;

        inputs              = null;
        outputs             = null;
        byproducts          = null;
        stationsAllowed     = null;
        requires            = null;

        durationOverrideSec = ChefZ_Undefined.FLOAT;
        freshnessCarry      = ChefZ_Undefined.FLOAT;
        qualityDelta        = ChefZ_Undefined.FLOAT;
        priority            = ChefZ_Undefined.INT;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.TRANSFORM;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE
    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();

        process.TrimInPlace();
        qualityRule.TrimInPlace();
        qualityRule.ToUpper();
        ChefZ_TextList.TrimAll(stationsAllowed);

        int i;
        if (inputs)
        {
            for (i = 0; i < inputs.Count(); i++)
            {
                ChefZ_SlotDef s = inputs.Get(i);
                if (s)
                    s.Normalize();
            }
        }

        NormalizeOutputs(outputs);
        NormalizeOutputs(byproducts);

        if (requires)
        {
            for (i = 0; i < requires.Count(); i++)
            {
                ChefZ_CapabilityReq r = requires.Get(i);
                if (r)
                    r.Normalize();
            }
        }
    }

    private void NormalizeOutputs(array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (o)
                o.Normalize();
        }
    }

    //--------------------------------------------------------------------------
    // VALIDATE (11 §7)
    //--------------------------------------------------------------------------

    /**
     * Die drei Abweisungsgruende, die ohne jeden Nachschlager entscheidbar
     * sind:
     *
     *   ohne process  -> nicht zuordenbar, keine Station koennte ihn anbieten
     *   ohne inputs   -> er wuerde auf JEDE Station passen, auch auf eine leere
     *   ohne outputs  -> "es waere eine Zutatenvernichtungsmaschine" (11 §7)
     *
     * Der Rest - unbekannter Prozess, unbekannte Station, fehlende
     * Ergebnisklasse, HANDCRAFT mit drei Eingaengen - braucht Registries und
     * CfgVehicles und sitzt im ChefZ_ProcessCompiler (3_Game).
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (ChefZ_Undefined.IsTextUndefined(process))
        {
            if (ctx)
                ctx.Error(this, "Transform ohne \"process\" - abgewiesen. Ohne Prozess gibt es " + "keine Station, die ihn anbieten koennte, und keine Aktion, die ihn " + "ausloesen wuerde.");
            return false;
        }

        if (!inputs || inputs.Count() == 0)
        {
            if (ctx)
                ctx.Error(this, "Transform ohne \"inputs\" - abgewiesen. Er haette keine " + "Bedingung und wuerde damit auf jeden Stationsinhalt passen, auch auf " + "einen leeren.");
            return false;
        }

        if (!outputs || outputs.Count() == 0)
        {
            if (ctx)
                ctx.Error(this, "Transform ohne \"outputs\" - abgewiesen. Er wuerde die " + "Eingaenge verbrauchen und nichts erzeugen (11 §7: eine " + "Zutatenvernichtungsmaschine). Ein reiner Zustandswechsel ist ebenfalls " + "ein Output: ein Eintrag mit leerem \"cls\" und gesetztem \"setState\".");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //
    // Unterobjektlisten als GANZES, nie elementweise - wie beim Rezept
    // (ChefZ_RecipeDef.PatchFrom). Eine halb gepatchte Eingangsliste waere ein
    // Transform, den niemand geschrieben hat.
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_TransformDef s = ChefZ_TransformDef.Cast(src);
        if (!s)
            return;

        if (s.inputs)       inputs     = s.inputs;
        if (s.outputs)      outputs    = s.outputs;
        if (s.byproducts)   byproducts = s.byproducts;
        if (s.requires)     requires   = s.requires;

        stationsAllowed     = PatchStringArray(stationsAllowed, s.stationsAllowed);

        process             = PatchText(process, s.process, s, "process");
        qualityRule         = PatchText(qualityRule, s.qualityRule, s, "qualityRule");

        durationOverrideSec = PatchFloat(durationOverrideSec, s.durationOverrideSec, s, "durationOverrideSec");
        freshnessCarry      = PatchFloat(freshnessCarry, s.freshnessCarry, s, "freshnessCarry");
        qualityDelta        = PatchFloat(qualityDelta, s.qualityDelta, s, "qualityDelta");
        priority            = PatchInt(priority, s.priority, s, "priority");
    }

    //! Der Transform selbst hat ausser "disabled" kein bool. Seine
    //! UNTEROBJEKTE haben welche, und die Sonde erreicht sie nur ueber diesen
    //! Durchgriff - dieselbe Paarung ueber den INDEX wie beim Rezept
    //! (ChefZ_RecipeDef.CaptureExplicitBools).
    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_TransformDef o = ChefZ_TransformDef.Cast(other);
        if (!o)
            return;

        if (inputs && o.inputs)
        {
            for (int i = 0; i < inputs.Count() && i < o.inputs.Count(); i++)
            {
                ChefZ_SlotDef mine = inputs.Get(i);
                if (mine)
                    mine.CaptureExplicitBools(o.inputs.Get(i));
            }
        }

        CaptureOutputBools(outputs, o.outputs);
        CaptureOutputBools(byproducts, o.byproducts);
    }

    private void CaptureOutputBools(array<ref ChefZ_OutputDef> mine, array<ref ChefZ_OutputDef> probe)
    {
        if (!mine || !probe)
            return;
        for (int i = 0; i < mine.Count() && i < probe.Count(); i++)
        {
            ChefZ_OutputDef o = mine.Get(i);
            if (o)
                o.CaptureExplicitBools(probe.Get(i));
        }
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    /**
     * Code-Defaults aus 11 §2.
     *
     * durationOverrideSec bleibt ABSICHTLICH Sentinel: "nicht gesetzt" heisst
     * "nimm die Prozessdauer", und das ist etwas anderes als "0 Sekunden".
     *
     * qualityRule bleibt leer und wird NICHT auf "MIN" gesetzt: der
     * ChefZ_QualityManager behandelt den Leerstring bereits als MIN
     * (CombineRanks) und meldet nur bei einem UNBEKANNTEN Namen. Haetten wir
     * hier "MIN" eingesetzt, waere im Log nicht mehr zu sehen, ob der Autor
     * sich entschieden oder nichts geschrieben hat.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        process        = ChefZ_Undefined.TextOr(process, "");
        qualityRule    = ChefZ_Undefined.TextOr(qualityRule, "");
        freshnessCarry = ChefZ_Undefined.FloatOr(freshnessCarry, 1.0);
        qualityDelta   = ChefZ_Undefined.FloatOr(qualityDelta, 0.0);
        priority       = ChefZ_Undefined.IntOr(priority, 0);

        int i;
        if (inputs)
        {
            for (i = 0; i < inputs.Count(); i++)
            {
                ChefZ_SlotDef s = inputs.Get(i);
                if (s)
                    s.ResolveDefaults();
            }
        }

        // ZWINGEND vor ResolveOutputDefaults: der Transformwert gilt dort, wo
        // der Output selbst nichts sagt.
        //
        // Danach waere es nicht mehr moeglich - ChefZ_OutputDef.ResolveDefaults
        // ersetzt den Sentinel durch 1.0, und "nicht gesetzt" ist ab diesem
        // Moment nicht mehr von "ausdruecklich 1.0" zu unterscheiden. Genau
        // das ist der Kern von 02 E3, hier eine Ebene tiefer.
        //
        // Idempotent: beim zweiten Aufruf sind die Outputs nicht mehr
        // Sentinel, und die Durchreiche tut nichts.
        PushFreshnessCarry(outputs);
        PushFreshnessCarry(byproducts);

        ResolveOutputDefaults(outputs);
        ResolveOutputDefaults(byproducts);

        if (requires)
        {
            for (i = 0; i < requires.Count(); i++)
            {
                ChefZ_CapabilityReq r = requires.Get(i);
                if (r)
                    r.ResolveDefaults();
            }
        }
    }

    /**
     * Den Transformwert in jedes Ergebnis durchreichen, das selbst nichts
     * sagt (11 §2: freshnessCarry steht am Transform, 08 §2: auch am Output).
     *
     * Die Vorrangregel ist die uebliche und die einzige, die niemanden
     * ueberrascht: das SPEZIELLERE gewinnt. Wer an einem einzelnen Ergebnis
     * einen Wert schreibt, meint dieses Ergebnis; wer ihn am Transform
     * schreibt, meint alle.
     */
    private void PushFreshnessCarry(array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (!o)
                continue;
            if (!ChefZ_Undefined.IsFloatUndefined(o.freshnessCarry))
                continue;               // der Output hat sich geaeussert
            o.freshnessCarry = freshnessCarry;
        }
    }

    private void ResolveOutputDefaults(array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (o)
                o.ResolveDefaults();
        }
    }

    //--------------------------------------------------------------------------

    bool HasDurationOverride()
    {
        return !ChefZ_Undefined.IsFloatUndefined(durationOverrideSec);
    }

    int InputCount()
    {
        if (!inputs)
            return 0;
        return inputs.Count();
    }

    /**
     * Ist das ein reiner Zustandswechsel (11 §2)?
     *
     * "Rein" heisst: KEIN einziger Output nennt eine Klasse. Ein gemischter
     * Transform - ein Output mit Klasse, einer ohne - ist kein Sonderfall,
     * sondern ein Widerspruch: er wuerde die Eingaben zugleich verbrauchen und
     * bestehen lassen. Der Compiler weist ihn ab.
     */
    bool IsPureStateChange()
    {
        if (!outputs || outputs.Count() == 0)
            return false;

        for (int i = 0; i < outputs.Count(); i++)
        {
            ChefZ_OutputDef o = outputs.Get(i);
            if (!o)
                continue;
            if (o.cls != "")
                return false;
        }
        return true;
    }

    //! true, wenn MINDESTENS ein Output eine Klasse nennt.
    bool HasClassOutput()
    {
        if (!outputs)
            return false;
        for (int i = 0; i < outputs.Count(); i++)
        {
            ChefZ_OutputDef o = outputs.Get(i);
            if (o && o.cls != "")
                return true;
        }
        return false;
    }

    //! Nur fuer den Selbsttest.
    override static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        ChefZ_TransformDef t = new ChefZ_TransformDef();
        t.id = "CHEFZ_TD_A";
        if (t.Validate(ctx))                            return false;   // ohne process

        t.process = "CHEFZ_TD_PROC";
        if (t.Validate(ctx))                            return false;   // ohne inputs

        t.inputs = new array<ref ChefZ_SlotDef>();
        t.inputs.Insert(new ChefZ_SlotDef());
        if (t.Validate(ctx))                            return false;   // ohne outputs

        t.outputs = new array<ref ChefZ_OutputDef>();
        ChefZ_OutputDef outDef = new ChefZ_OutputDef();
        t.outputs.Insert(outDef);
        if (!t.Validate(ctx))                           return false;

        // Reiner Zustandswechsel: kein Output nennt eine Klasse.
        if (!t.IsPureStateChange())                     return false;
        if (t.HasClassOutput())                         return false;

        outDef.cls = "CHEFZ_TD_KLASSE";
        if (t.IsPureStateChange())                      return false;
        if (!t.HasClassOutput())                        return false;

        // Defaults.
        if (t.HasDurationOverride())                    return false;
        t.ResolveDefaults();
        if (t.HasDurationOverride())                    return false;   // bleibt Sentinel
        if (t.freshnessCarry != 1.0)                    return false;
        if (t.qualityDelta != 0.0)                      return false;
        if (t.priority != 0)                            return false;
        if (t.qualityRule != "")                        return false;
        if (t.InputCount() != 1)                        return false;

        // Durchreiche von freshnessCarry: der Transformwert gilt dort, wo der
        // Output selbst nichts sagt - und NUR dort.
        ChefZ_TransformDef carry = new ChefZ_TransformDef();
        carry.id             = "CHEFZ_TD_CARRY";
        carry.process        = "CHEFZ_TD_PROC";
        carry.freshnessCarry = 0.9;
        carry.inputs         = new array<ref ChefZ_SlotDef>();
        carry.inputs.Insert(new ChefZ_SlotDef());
        carry.outputs        = new array<ref ChefZ_OutputDef>();

        ChefZ_OutputDef stumm = new ChefZ_OutputDef();      // sagt nichts
        ChefZ_OutputDef laut  = new ChefZ_OutputDef();
        laut.freshnessCarry   = 0.5;                        // sagt etwas
        carry.outputs.Insert(stumm);
        carry.outputs.Insert(laut);

        carry.ResolveDefaults();
        if (stumm.freshnessCarry != 0.9)                return false;
        if (laut.freshnessCarry != 0.5)                 return false;

        // Zweiter Aufruf aendert nichts mehr.
        carry.ResolveDefaults();
        if (stumm.freshnessCarry != 0.9)                return false;

        return true;
    }
}
