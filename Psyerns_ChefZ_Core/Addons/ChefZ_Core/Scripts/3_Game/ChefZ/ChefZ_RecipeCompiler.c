//==============================================================================
// ChefZ_RecipeCompiler - Rohform -> kompiliertes Rezept
//
// Entwurf: 08 §2 (Felder), 08 §5.1 (was der Index braucht), 08 §8
// (Fehlerverhalten, Zeile fuer Zeile), 09 §4.1 (Spezifitaet), 07 §5
// (Selektor- und Slotcompiler), V-B §3 Auflage 2 (defaultExtraItems),
// V-B §4 Auflage 4 (essbare Ergebnisklassen brauchen Nutrition/Food).
//
// ---------------------------------------------------------------------------
// Die Leitlinie: ein Rezept wird ganz genommen oder gar nicht
// ---------------------------------------------------------------------------
// 08 §8 weist Rezepte in sechs Faellen ab, und jeder davon hat denselben Kern:
// ein halb gueltiges Rezept ist gefaehrlicher als keines. Ein Rezept, dem ein
// Pflichtslot wegen eines Tippfehlers fehlt, ist zu leicht ausloesbar. Ein
// Rezept, dessen Ergebnisklasse nicht existiert, verbraucht Zutaten und
// erzeugt nichts.
//
// Deshalb: jeder Fehler verwirft das GANZE Rezept, mit einer Meldung, die
// Datei, ID und Ursache nennt. Alle anderen Rezepte bleiben unberuehrt
// (02 §8).
//
// Geklammert statt abgewiesen wird nur, wo 08 §8 es ausdruecklich sagt:
// TIMED bei abgeschaltetem allowTimedRecipes wird zu ON_STAGE, ON_STAGE ohne
// doneStages bekommt den Default. Beides mit WARN.
//
// ---------------------------------------------------------------------------
// Warum das hier liegt und nicht in 1_Core
// ---------------------------------------------------------------------------
// Zwei der Pruefungen aus 08 §8 lesen CfgVehicles - die Ergebnisklasse muss
// existieren, und wenn sie essbar ist, braucht sie einen Nutrition- oder
// Food-Block. g_Game gibt es erst ab 3_Game (00 §4). Der Rest des Compilers
// koennte in 1_Core liegen, waere dann aber ueber zwei Layer verteilt, ohne
// dass irgendjemand davon etwas haette.
//
// KEIN CONTENT: keine Klasse, keine Kategorie, kein Gericht wird hier benannt.
// "Edible_Base" ist ein VANILLA-Typ und kein ChefZ-Inhalt - er steht hier als
// das, was er ist: die Wurzel alles Essbaren in DayZ.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RecipeCompiler
{
    //! Die Vanillawurzel alles Essbaren und der Configpfad stehen seit S12 in
    //! ChefZ_VanillaNutrition - dort, wo auch der Startaudit sie liest. Zwei
    //! Nachbildungen derselben Engine-Bedingung waeren zwei Gelegenheiten, sie
    //! unterschiedlich falsch zu lesen (13 §3).

    //! Default fuer completion ON_STAGE ohne doneStages (08 §8). Es sind
    //! Vanilla-FoodStages, kein ChefZ-Vokabular.
    static const string DEFAULT_STAGE_A = "Baked";
    static const string DEFAULT_STAGE_B = "Boiled";
    static const string DEFAULT_STAGE_C = "Dried";

    private ChefZ_LoadReport   m_Report;     // ohne ref: gehoert dem Manager
    private ChefZ_CompileContext m_Ctx;      // ohne ref: gehoert dem Manager
    private ChefZ_CoreSettingsDef m_Settings;

    //! Pruefung gegen CfgVehicles. Aus, wenn kein Spiel laeuft (Selbsttest):
    //! ohne g_Game gaebe es keine Config, und dann waere JEDE Ergebnisklasse
    //! "nicht vorhanden" - der Selbsttest wuerde jedes Testrezept verwerfen
    //! und dabei nichts pruefen.
    private bool m_VerifyClasses;

    void ChefZ_RecipeCompiler()
    {
        m_VerifyClasses = true;
    }

    void Init(ChefZ_CompileContext ctx, ChefZ_LoadReport report, ChefZ_CoreSettingsDef settings)
    {
        m_Ctx      = ctx;
        m_Report   = report;
        m_Settings = settings;
    }

    void SetVerifyClasses(bool on)
    {
        m_VerifyClasses = on;
    }

    //==========================================================================
    // Der eine oeffentliche Einstieg
    //==========================================================================

    /**
     * Ein Rezept kompilieren. null heisst "abgewiesen", und der Grund steht
     * dann im Ladebericht - immer mit Rezept-ID und Herkunft.
     */
    ChefZ_CompiledRecipe Compile(ChefZ_RecipeDef def)
    {
        if (!def)
            return null;

        if (!m_Ctx)
        {
            // Ohne Kontext gibt es keinen Nachschlager, und ohne Nachschlager
            // waere jede Kategorie unbekannt. Programmierfehler, kein
            // Datenfehler.
            Fail(def, "kein Selektorkontext vorhanden - der Config Manager hat die "
                + "Registries noch nicht gebaut. Das Rezept wird uebersprungen.");
            return null;
        }

        m_Ctx.SetSubject(def.sourceRef, def.id);

        ChefZ_CompiledRecipe rec = new ChefZ_CompiledRecipe();
        rec.recipeSym = ChefZ_SymbolTable.Intern(def.id);
        rec.id        = def.id;
        rec.sourceRef = def.sourceRef;
        rec.priority  = ClampPriority(def);

        if (!CompileSlots(def, rec))        return null;
        if (!CompileContexts(def, rec))     return null;
        if (!CompilePolicy(def, rec))       return null;
        if (!CompileCompletion(def, rec))   return null;
        if (!CompileOutputs(def, rec))      return null;

        CompileQuality(def, rec);
        CompileRequirements(def, rec);
        CompileTools(def, rec);
        CompilePassThrough(def, rec);

        ComputeIndexFacts(rec);
        return rec;
    }

    //==========================================================================
    // Slots (07 §5)
    //==========================================================================

    private bool CompileSlots(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        array<ref ChefZ_CompiledSlot> slots;
        string error;

        if (!ChefZ_SelectorCompiler.CompileSlotList(def.slots, m_Ctx, slots, error))
        {
            // Die Einzelmeldung steht bereits im Bericht (der Slotcompiler hat
            // sie gesetzt). Diese Zeile sagt, was daraus FOLGT - und das ist
            // die Information, die ein Betreiber braucht.
            Fail(def, "Rezept abgewiesen, weil ein Slot nicht uebersetzt werden konnte: "
                + error + ". Ein Rezept mit weggefallenem Pflichtslot waere viel zu leicht "
                + "ausloesbar (07 §7).");
            return false;
        }

        rec.slots.Clear();
        for (int i = 0; i < slots.Count(); i++)
            rec.slots.Insert(slots.Get(i));

        int required = 0;
        for (int s = 0; s < rec.slots.Count(); s++)
        {
            if (ChefZ_CompiledRecipe.IsRequiredSlot(rec.slots.Get(s)))
                required++;
        }

        if (required == 0)
        {
            Fail(def, "Rezept hat keinen einzigen Pflichtslot - alle " + rec.slots.Count().ToString()
                + " Slots sind optional oder verlangen 0 Items. Es wuerde bei jedem "
                + "Gefaessinhalt zuenden, auch bei einem leeren.");
            return false;
        }

        rec.requiredSlots = required;
        return true;
    }

    //==========================================================================
    // Kontexte (08 §2, 08 E4)
    //==========================================================================

    private bool CompileContexts(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        rec.contexts.Clear();

        // Nullschutz, obwohl Validate() bereits abweist: der Compiler wird
        // auch aus Selbsttests und (ab S18) aus Adminkommandos gerufen, und
        // dort ist die Validierungsstufe nicht garantiert durchlaufen.
        if (!def.contexts)
        {
            Fail(def, "Rezept ohne \"contexts\" - abgewiesen. Es zuendete sonst in jedem "
                + "Topf, jeder Pfanne, jedem Kessel.");
            return false;
        }

        for (int i = 0; i < def.contexts.Count(); i++)
        {
            ChefZ_ContextRule raw = def.contexts.Get(i);
            if (!raw)
                continue;

            ChefZ_CompiledContext ctx = new ChefZ_CompiledContext();
            ChefZ_TextList.SymbolsOf(raw.deviceClasses, ctx.deviceClasses);
            ChefZ_TextList.SymbolsOf(raw.deviceCategories, ctx.deviceCategories);
            ChefZ_TextList.SymbolsOf(raw.methods, ctx.methods);
            ChefZ_TextList.SymbolsOf(raw.liquidTypes, ctx.liquidTypes);
            ctx.deviceTemperature = SaneRange(def, raw.deviceTemperature, "deviceTemperature");
            ctx.liquidQuantity    = SaneRange(def, raw.liquidQuantity, "liquidQuantity");
            ctx.requiresLiquid    = raw.requiresLiquid;

            if (ctx.deviceClasses.Count() == 0 && ctx.deviceCategories.Count() == 0)
            {
                Warn(def, "Kontextregel " + i.ToString() + " nennt weder deviceClasses noch "
                    + "deviceCategories - sie gilt damit an JEDEM Kochgeraet. Ist das "
                    + "beabsichtigt? Ueblich ist eine Geraetekategorie (08 E4).");
            }

            rec.contexts.Insert(ctx);
        }

        if (rec.contexts.Count() == 0)
        {
            Fail(def, "Rezept hat nach dem Uebersetzen keine brauchbare Kontextregel mehr - "
                + "abgewiesen. Es zuendete sonst in jedem Topf, jeder Pfanne, jedem Kessel.");
            return false;
        }

        return true;
    }

    //==========================================================================
    // Policy (08 E2, V-B Auflage 2)
    //==========================================================================

    private bool CompilePolicy(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        ChefZ_CompiledPolicy policy = new ChefZ_CompiledPolicy();

        string mode = "";
        if (def.policy)
            mode = def.policy.extraItems;

        if (mode == "")
        {
            // V-B Auflage 2: der Betreiber-Default aus Core.json. Er ist der
            // EFFEKTIVE Wert, und genau er geht in die Spezifitaet ein
            // (V-B §3, Folge 4) - sonst haenge die Rezeptprioritaet daran, ob
            // ein Autor den Default ausgeschrieben hat.
            mode = SettingsExtraItems();
        }

        policy.extraItemsMode = ChefZ_ExtraItemsMode.FromName(mode);
        if (policy.extraItemsMode < 0)
        {
            Warn(def, "policy.extraItems = \"" + mode + "\" ist unbekannt - benutzt wird "
                + "\"forbid\". Gueltig: " + ChefZ_ExtraItemsMode.ValidNames() + ".");
            policy.extraItemsMode = ChefZ_ExtraItemsMode.FORBID;
        }

        if (def.policy)
        {
            if (def.policy.extraItemsAllowedIf)
            {
                string selError;
                policy.extraItemsAllowedIf =
                    ChefZ_SelectorCompiler.Compile(def.policy.extraItemsAllowedIf, m_Ctx, selError);

                if (!policy.extraItemsAllowedIf)
                {
                    Fail(def, "policy.extraItemsAllowedIf ist unbrauchbar: " + selError
                        + ". Das Rezept wird abgewiesen - ein Ventil, das nicht uebersetzt, "
                        + "waere schlimmer als keines: es duldete stillschweigend nichts oder "
                        + "alles, je nachdem, wie man den Fehler behandelt.");
                    return false;
                }
            }

            ChefZ_TextList.SymbolsOf(def.policy.forbiddenStates, policy.forbiddenStates);

            if (!ChefZ_Undefined.IsFloatUndefined(def.policy.minMatchedHealth01))
                policy.minMatchedHealth01 = Clamp01(def.policy.minMatchedHealth01);
            if (!ChefZ_Undefined.IsFloatUndefined(def.policy.liquidConsumed))
                policy.liquidConsumed = def.policy.liquidConsumed;
        }

        rec.policy = policy;
        return true;
    }

    private string SettingsExtraItems()
    {
        if (!m_Settings)
            return ChefZ_ExtraItemsMode.FORBID_NAME;
        if (m_Settings.defaultExtraItems == "")
            return ChefZ_ExtraItemsMode.FORBID_NAME;
        return m_Settings.defaultExtraItems;
    }

    //==========================================================================
    // Abschlussbedingung (08 §8, 10 §6)
    //==========================================================================

    private bool CompileCompletion(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        int mode = ChefZ_Completion.ON_STAGE;

        if (def.completion != "")
        {
            mode = ChefZ_Completion.FromName(def.completion);
            if (mode < 0)
            {
                Warn(def, "completion = \"" + def.completion + "\" ist unbekannt - benutzt wird "
                    + ChefZ_Completion.ON_STAGE_NAME + ". Gueltig: "
                    + ChefZ_Completion.ValidNames() + ".");
                mode = ChefZ_Completion.ON_STAGE;
            }
        }

        if (mode == ChefZ_Completion.TIMED)
        {
            if (def.cookSeconds <= 0.0)
            {
                Fail(def, "completion \"TIMED\" ohne brauchbares cookSeconds - abgewiesen. "
                    + "Ein Rezept mit eigener Uhr, die nie ablaeuft, ist ein Rezept, das nie "
                    + "fertig wird (08 §8).");
                return false;
            }

            if (m_Settings && !m_Settings.allowTimedRecipes)
            {
                // Betreiber-Notbremse (08 §8). Heruntergestuft statt
                // abgewiesen: der Betreiber wollte die eigene Uhr abschalten,
                // nicht das Gericht.
                Warn(def, "completion \"TIMED\" ist per Einstellung abgeschaltet "
                    + "(allowTimedRecipes = false) - das Rezept laeuft als \"ON_STAGE\" und "
                    + "wartet damit auf Vanillas FoodStages statt auf die eigene Uhr.");
                mode = ChefZ_Completion.ON_STAGE;
            }
        }

        rec.completion     = mode;
        rec.cookSeconds    = def.cookSeconds;
        rec.minTemperature = def.minTemperature;

        if (mode != ChefZ_Completion.ON_STAGE)
            return true;

        return CompileDoneStages(def, rec);
    }

    private bool CompileDoneStages(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        rec.doneStages.Clear();

        if (ChefZ_TextList.Count(def.doneStages) == 0)
        {
            Warn(def, "completion \"ON_STAGE\" ohne \"doneStages\" - benutzt wird der Default "
                + "{" + DEFAULT_STAGE_A + ", " + DEFAULT_STAGE_B + ", " + DEFAULT_STAGE_C + "} "
                + "(08 §8).");
            rec.doneStages.Insert(ChefZ_VanillaStage.FromName(DEFAULT_STAGE_A));
            rec.doneStages.Insert(ChefZ_VanillaStage.FromName(DEFAULT_STAGE_B));
            rec.doneStages.Insert(ChefZ_VanillaStage.FromName(DEFAULT_STAGE_C));
            return true;
        }

        for (int i = 0; i < def.doneStages.Count(); i++)
        {
            string name = def.doneStages.Get(i);
            int stage = ChefZ_VanillaStage.FromName(name);
            if (stage < 0)
            {
                Fail(def, "doneStages nennt die unbekannte Vanilla-Stufe \"" + name
                    + "\" - abgewiesen. Gueltig: " + ChefZ_VanillaStage.ValidNames()
                    + ". Ein Tippfehler wuerde das Rezept lautlos nie fertig werden lassen.");
                return false;
            }
            if (rec.doneStages.Find(stage) < 0)
                rec.doneStages.Insert(stage);
        }

        return true;
    }

    //==========================================================================
    // Ergebnisse (08 §8, V-B Auflage 4)
    //==========================================================================

    private bool CompileOutputs(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        rec.outputs.Clear();
        rec.byproducts.Clear();

        if (!CopyOutputs(def, def.outputs, rec.outputs, "outputs"))
            return false;
        if (!CopyOutputs(def, def.byproducts, rec.byproducts, "byproducts"))
            return false;

        if (rec.outputs.Count() == 0)
        {
            Fail(def, "Rezept hat nach dem Pruefen kein einziges brauchbares Ergebnis mehr - "
                + "abgewiesen. Es wuerde die Zutaten verbrauchen und nichts erzeugen.");
            return false;
        }

        return true;
    }

    private bool CopyOutputs(notnull ChefZ_RecipeDef def,
                             array<ref ChefZ_OutputDef> src,
                             notnull array<ref ChefZ_OutputDef> dst,
                             string field)
    {
        if (!src)
            return true;

        for (int i = 0; i < src.Count(); i++)
        {
            ChefZ_OutputDef o = src.Get(i);
            if (!o)
                continue;

            if (o.cls == "")
            {
                Fail(def, field + "[" + i.ToString() + "] hat kein \"cls\" - abgewiesen. "
                    + "Ein Ergebnis ohne Klasse ist kein Ergebnis.");
                return false;
            }

            if (!CheckOutputClass(def, o.cls, field + "[" + i.ToString() + "]"))
                return false;

            // Portionsfelder (15 §7). VOR den Varianten, weil eine fehlende
            // Portionsklasse das Rezept ohnehin abweist - dann muss die
            // Variantenliste gar nicht mehr geprueft werden.
            if (!CheckPortionFields(def, o, field + "[" + i.ToString() + "]"))
                return false;

            // Varianten je Qualitaetsstufe (12): dieselbe Pruefung. Eine
            // fehlende Variantenklasse ist genauso toedlich wie eine fehlende
            // Hauptklasse - nur faellt sie erst auf, wenn jemand PREMIUM
            // kocht, und das kann Wochen dauern.
            if (o.variants)
            {
                for (int v = 0; v < o.variants.Count(); v++)
                {
                    ChefZ_OutputVariant variant = o.variants.Get(v);
                    if (!variant || variant.cls == "")
                        continue;
                    if (!CheckOutputClass(def, variant.cls,
                            field + "[" + i.ToString() + "].variants[" + v.ToString() + "]"))
                        return false;
                }
            }

            dst.Insert(o);
        }

        return true;
    }

    /**
     * Die Portionsfelder eines Ergebnisses (15 §7).
     *
     * Die REGELN stehen in ChefZ_PortionOutputAudit, weil dieselben fuer
     * Transforms gelten (11 E4: dasselbe ChefZ_OutputDef). Hier steht nur, was
     * dieser Compiler damit tut: melden, und bei einer unbrauchbaren
     * Portionsklasse das Rezept abweisen.
     *
     * Warum ABWEISEN und nicht degradieren (15 §7, Zeile 2): "Sonst entstuende
     * ein Topf, aus dem man nichts entnehmen kann." Der Spieler haette sein
     * Essen gekocht und kaeme nicht daran - ein Zutatenverlust mit Verzoegerung.
     */
    private bool CheckPortionFields(notnull ChefZ_RecipeDef def,
                                    notnull ChefZ_OutputDef o,
                                    string where)
    {
        array<string> warnings = new array<string>();
        string portionClass;
        string rejectReason;

        if (!ChefZ_PortionOutputAudit.Audit(o, where, warnings, portionClass, rejectReason))
        {
            Fail(def, rejectReason);
            return false;
        }

        for (int w = 0; w < warnings.Count(); w++)
            Warn(def, warnings.Get(w));

        if (portionClass == "")
            return true;

        // Dieselbe Pruefung wie fuer die Ergebnisklasse, und aus demselben
        // Grund: 15 §7 Zeile 3 nennt ausdruecklich den Fall "portionClass ist
        // essbar, hat aber keinen Nutrition-Block" - die entnommene Portion
        // saettigte dann lautlos nicht (01 V7).
        return CheckOutputClass(def, portionClass, where + ".portionClass");
    }

    /**
     * Die beiden Pruefungen aus 08 §8, die CfgVehicles brauchen.
     *
     * 1. Die Klasse muss existieren. Sonst verbraucht das Rezept Zutaten und
     *    erzeugt nichts - und das faellt erst auf, wenn es jemand kocht.
     *
     * 2. Ist sie ESSBAR, muss der Magen sie registrieren koennen (01 V7,
     *    V-B Auflage 4). Das sind ZWEI Bedingungen und nicht eine:
     *
     *      - "class Nutrition" ODER "class Food" muss vorhanden sein,
     *      - scope darf nicht 0 sein.
     *
     *    PlayerStomach.InitData prueft beide (PlayerStomach.c:230-247), und
     *    faellt eine davon, registriert es die Klasse nicht. AddToStomach
     *    bricht dann OHNE MELDUNG ab: das Gericht wird gegessen, verschwindet
     *    und saettigt nichts. Es gibt keinen leiseren Fehler im ganzen System,
     *    und deshalb ist er hier ein ERROR.
     *
     *    Die scope-Bedingung ist seit S12 dabei. 13 §8 verlangt sie
     *    ausdruecklich ("Ergebnisklasse mit scope = 0 -> dito"), und sie fehlte
     *    bis dahin: eine Klasse mit vollstaendigem Nutrition-Block und
     *    scope = 0 waere durchgerutscht und haette sich im Spiel exakt so
     *    verhalten wie eine ohne Block.
     *
     * Der Configzugriff selbst steht seit S12 in ChefZ_VanillaNutrition -
     * derselbe Test, den auch der Startaudit benutzt (13 §3). Zwei
     * Nachbildungen derselben Engine-Bedingung waeren zwei Gelegenheiten, sie
     * unterschiedlich falsch zu lesen.
     */
    private bool CheckOutputClass(notnull ChefZ_RecipeDef def, string cls, string where)
    {
        if (!m_VerifyClasses || !g_Game)
            return true;

        if (!ChefZ_VanillaNutrition.ClassExists(cls))
        {
            Fail(def, where + " nennt die Klasse \"" + cls + "\", die es in CfgVehicles nicht "
                + "gibt - Rezept abgewiesen. Fehlt das Content-Modul, oder ist der Name "
                + "falsch geschrieben?");
            return false;
        }

        if (!ChefZ_VanillaNutrition.IsEdible(cls))
            return true;

        string reason;
        if (ChefZ_VanillaNutrition.WouldRegisterAtStomach(cls, reason))
            return true;

        Fail(def, where + ": die essbare Klasse \"" + cls + "\" " + reason
            + " - Rezept abgewiesen. PlayerStomach.InitData registriert sie damit nicht, und "
            + "AddToStomach bricht beim Verzehr OHNE MELDUNG ab: das Gericht verschwaende, "
            + "ohne zu saettigen (01 V7). Der Startaudit meldet denselben Fall ein zweites "
            + "Mal, mit Klassenname (13 §3).");
        return false;
    }

    //==========================================================================
    // Qualitaet, Faehigkeiten, Werkzeug, Durchreiche
    //==========================================================================

    private void CompileQuality(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        rec.gradeRules.Clear();
        if (def.gradeRules)
        {
            for (int i = 0; i < def.gradeRules.Count(); i++)
            {
                ChefZ_GradeRule rule = def.gradeRules.Get(i);
                if (rule)
                    rec.gradeRules.Insert(rule);
            }
        }

        // Die Regeln werden hier NICHT uebersetzt und nicht geprueft. Sie
        // gehoeren dem Quality Manager (12, S10), und der kompiliert ihre
        // Selektoren, wenn er gebaut wird. Sie jetzt zu uebersetzen hiesse,
        // eine Bedeutung festzulegen, die S10 noch gar nicht entschieden hat.

        if (def.qualityTierSet != "")
            rec.qualityTierSet = ChefZ_SymbolTable.Intern(def.qualityTierSet);
        rec.qualityBias = def.qualityBias;
    }

    private void CompileRequirements(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        rec.requires.Clear();
        if (!def.requires)
            return;

        for (int i = 0; i < def.requires.Count(); i++)
        {
            ChefZ_CapabilityReq req = def.requires.Get(i);
            if (!req)
                continue;
            if (req.capability == "")
            {
                Warn(def, "requires[" + i.ToString() + "] nennt keine \"capability\" - der "
                    + "Eintrag wird ausgelassen.");
                continue;
            }
            rec.requires.Insert(req);
        }
    }

    private void CompileTools(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        ChefZ_TextList.SymbolsOf(def.requiredToolGroups, rec.requiredToolGroups);
    }

    private void CompilePassThrough(notnull ChefZ_RecipeDef def, notnull ChefZ_CompiledRecipe rec)
    {
        // Effekt-IDs und Ereignisnamen sind fuer den Core UNDURCHSICHTIG
        // (Architekturplan: "Effekt-IDs, Core wertet sie nie aus"). Sie werden
        // getragen und weitergereicht, nie gedeutet - genau deshalb bleiben sie
        // Strings und werden nicht zu Symbolen.
        rec.effects.Clear();
        rec.emitEvents.Clear();

        int i;
        if (def.effects)
        {
            for (i = 0; i < def.effects.Count(); i++)
                rec.effects.Insert(def.effects.Get(i));
        }
        if (def.emitEvents)
        {
            for (i = 0; i < def.emitEvents.Count(); i++)
                rec.emitEvents.Insert(def.emitEvents.Get(i));
        }

        rec.nutritionModifier = def.nutritionModifier;
    }

    //==========================================================================
    // Index- und Rangzahlen (08 §5.1, 09 §4.1)
    //==========================================================================

    private void ComputeIndexFacts(notnull ChefZ_CompiledRecipe rec)
    {
        rec.specificity      = ChefZ_RecipeRanker.ComputeSpecificity(rec, Weights());
        rec.minItemCount     = ComputeMinItemCount(rec);
        rec.totalConstraints = ComputeTotalConstraints(rec);
        SelectGate(rec);
    }

    //! Wie viele Items muessen mindestens im Gefaess liegen (08 §5.1)?
    //! Die Summe der minCount aller Pflichtslots - weil ein Item hoechstens
    //! einen Slot bedient (07 §4).
    private int ComputeMinItemCount(notnull ChefZ_CompiledRecipe rec)
    {
        int n = 0;
        for (int i = 0; i < rec.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = rec.slots.Get(i);
            if (ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                n = n + slot.minCount;
        }
        return n;
    }

    private int ComputeTotalConstraints(notnull ChefZ_CompiledRecipe rec)
    {
        int n = rec.slots.Count();
        int i;

        for (i = 0; i < rec.contexts.Count(); i++)
            n = n + rec.contexts.Get(i).ConstraintCount();

        if (rec.policy)
            n = n + rec.policy.ConstraintCount();

        n = n + rec.requires.Count();
        n = n + rec.requiredToolGroups.Count();
        n = n + rec.doneStages.Count();
        return n;
    }

    /**
     * Das Torsymbol des invertierten Index (08 §5.1, "seltenster Pflichtslot").
     *
     * Gesucht wird unter allen PFLICHTslots das Blattpraedikat mit dem
     * kleinsten selectivityHint. Ein anyOf-Slot hat kein Tor: seine Zweige
     * sind Alternativen, und keine davon ist zwingend. Ein allOf-Slot dagegen
     * hat mehrere Tore zur Auswahl - jeder Zweig MUSS gelten, also darf jeder
     * als Tor dienen.
     *
     * Ein Rezept ohne Tor ist kein Fehler, nur teurer: es wird Kandidat,
     * sobald das Geraet passt.
     */
    private void SelectGate(notnull ChefZ_CompiledRecipe rec)
    {
        rec.gateKind = ChefZ_GateKind.NONE;
        rec.gateSym  = ChefZ_SymbolTable.INVALID;
        rec.gateBit  = -1;
        rec.gateHint = 0;

        int bestHint = 0;
        bool found   = false;

        for (int i = 0; i < rec.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = rec.slots.Get(i);
            if (!ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;
            if (!slot.selector)
                continue;

            ChefZ_CompiledSelector leaf = FindGateLeaf(slot.selector, 0);
            if (!leaf)
                continue;

            if (found && leaf.selectivityHint >= bestHint)
                continue;

            int kind = GateKindOf(leaf);
            if (kind == ChefZ_GateKind.NONE)
                continue;

            rec.gateKind = kind;
            rec.gateSym  = leaf.sym;
            rec.gateBit  = leaf.categoryBitIndex;
            rec.gateHint = leaf.selectivityHint;
            bestHint     = leaf.selectivityHint;
            found        = true;
        }
    }

    /**
     * Das engste zwingende Blatt eines Selektors.
     *
     * Es steigt ausschliesslich durch ALL_OF ab: dort muss JEDES Kind gelten,
     * also ist jedes Kind ein gueltiges Tor. Bei ANY_OF und NOT gibt es kein
     * zwingendes Blatt, und ein geratenes waere ein Tor, das gueltige Rezepte
     * aussperrt - der einzige Fehler, den dieser Vorfilter machen darf, ist
     * "zu wenig gefiltert".
     */
    private ChefZ_CompiledSelector FindGateLeaf(ChefZ_CompiledSelector node, int depth)
    {
        if (!node || depth > 8)
            return null;

        if (node.op == ChefZ_SelectorOp.ALL_OF)
        {
            if (!node.children)
                return null;

            ChefZ_CompiledSelector best;
            for (int i = 0; i < node.children.Count(); i++)
            {
                ChefZ_CompiledSelector child = FindGateLeaf(node.children.Get(i), depth + 1);
                if (!child)
                    continue;
                if (!best || child.selectivityHint < best.selectivityHint)
                    best = child;
            }
            return best;
        }

        if (GateKindOf(node) == ChefZ_GateKind.NONE)
            return null;
        return node;
    }

    private int GateKindOf(notnull ChefZ_CompiledSelector node)
    {
        if (node.op == ChefZ_SelectorOp.CLASS)
            return ChefZ_GateKind.CLASS;
        if (node.op == ChefZ_SelectorOp.CATEGORY)
            return ChefZ_GateKind.CATEGORY;
        if (node.op == ChefZ_SelectorOp.TAG)
            return ChefZ_GateKind.TAG;
        if (node.op == ChefZ_SelectorOp.STATE)
            return ChefZ_GateKind.STATE;
        return ChefZ_GateKind.NONE;
    }

    //==========================================================================
    // Kleinkram
    //==========================================================================

    private ChefZ_PriorityWeights Weights()
    {
        if (m_Ctx)
            return m_Ctx.Weights();
        return new ChefZ_PriorityWeights();
    }

    //! 09 §7: priority ausserhalb [-1000, 1000] wird geklemmt und gemeldet.
    //! Ein Wert wie 999999 schaltete die Spezifitaetsordnung faktisch ab.
    private int ClampPriority(notnull ChefZ_RecipeDef def)
    {
        int p = def.priority;
        if (ChefZ_Undefined.IsIntUndefined(p))
            return 0;

        if (p > 1000)
        {
            Warn(def, "priority = " + p.ToString() + " liegt ausserhalb von [-1000, 1000] und "
                + "wird auf 1000 geklemmt. Eine so grosse Zahl haette die berechnete "
                + "Spezifitaet faktisch abgeschaltet (09 §7).");
            return 1000;
        }
        if (p < -1000)
        {
            Warn(def, "priority = " + p.ToString() + " liegt ausserhalb von [-1000, 1000] und "
                + "wird auf -1000 geklemmt (09 §7).");
            return -1000;
        }
        return p;
    }

    //! 07 §7: vertauschte Bereichsgrenzen werden getauscht, nicht abgewiesen -
    //! die Absicht des Autors ist eindeutig.
    private ChefZ_Range SaneRange(notnull ChefZ_RecipeDef def, ChefZ_Range range, string field)
    {
        if (!range)
            return null;
        if (range.IsValid())
            return range;

        Warn(def, field + ": min (" + range.min.ToString() + ") ist groesser als max ("
            + range.max.ToString() + ") - die Grenzen werden getauscht.");

        ChefZ_Range fixedRange = new ChefZ_Range();
        fixedRange.Init(range.max, range.min);
        return fixedRange;
    }

    private float Clamp01(float v)
    {
        if (v < 0.0)
            return 0.0;
        if (v > 1.0)
            return 1.0;
        return v;
    }

    private void Fail(notnull ChefZ_RecipeDef def, string message)
    {
        if (m_Report)
            m_Report.AddError(def.sourceRef, def.id, message);
    }

    private void Warn(notnull ChefZ_RecipeDef def, string message)
    {
        if (m_Report)
            m_Report.AddWarn(def.sourceRef, def.id, message);
    }
}
