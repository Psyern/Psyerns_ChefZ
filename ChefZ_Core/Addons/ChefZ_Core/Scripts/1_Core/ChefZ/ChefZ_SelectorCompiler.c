//==============================================================================
// ChefZ_SelectorLimits / ChefZ_SelectorCompiler
//
// Entwurf: 07 §2.2 (Schnittstelle), 07 §5 (BOOT-Datenfluss), 07 §7
// (Fehlerverhalten, Zeile fuer Zeile), 09 §4.1 (Spezifitaetsformel),
// 07 E4 (selectivityHint).
//
// ---------------------------------------------------------------------------
// Die Leitlinie dieser Datei: ein Fehler weist ab, er repariert nicht
// ---------------------------------------------------------------------------
// 07 §7 ist an einer Stelle ungewoehnlich streng, und das ist Absicht:
//
//     "Unbekannte Kategorie / Tag / Zustand / Qualitaet im Selektor
//      -> Kompilierfehler, REZEPT ABGEWIESEN. Nicht 'Slot ignorieren':
//      ein Rezept mit weggefallenem Pflichtslot waere viel zu leicht
//      ausloesbar."
//
// Ein Tippfehler in einer Kategorie darf also nicht dazu fuehren, dass aus
// "Fleisch + Kartoffel + Pilz" ein Rezept wird, das nur noch "Kartoffel"
// verlangt. Deshalb gibt jede Aufloesung, die fehlschlaegt, null zurueck und
// setzt error - und der Aufrufer (S6) verwirft das ganze Rezept.
//
// Geklammert statt abgewiesen wird nur, wo der Entwurf es ausdruecklich sagt:
// vertauschte Bereichsgrenzen, minCount > maxCount, amount <= 0. Das sind
// Faelle, in denen die Absicht des Autors eindeutig ist.
//
// ---------------------------------------------------------------------------
// Warum die Rechnungen hier stehen und nicht im Matcher
// ---------------------------------------------------------------------------
// Spezifitaet und selectivityHint haengen NUR vom Rezept ab, nie vom
// Topfinhalt (09 E2). Beides einmal beim Boot zu rechnen kostet nichts und
// erspart pro Kesseltick eine Rechnung je Slot.
//
// Layer: 1_Core. Statisch und zustandslos - alles Veraenderliche steht im
// ChefZ_CompileContext.
//==============================================================================

class ChefZ_SelectorLimits
{
    //! CoreSettings.maxSelectorDepth, Code-Default aus 07 §7.
    static const int DEFAULT_MAX_DEPTH = 8;

    //! Notbremse gegen absurde Kinderlisten aus kaputten Daten. Weit oberhalb
    //! jedes sinnvollen Wertes - ein anyOf mit 64 Alternativen ist bereits ein
    //! Content-Problem, aber keines, das der Core entscheiden muss.
    static const int MAX_CHILDREN = 64;
}

//------------------------------------------------------------------------------

class ChefZ_SelectorCompiler
{
    //==========================================================================
    // Selektor
    //==========================================================================

    /**
     * Rohform -> kompilierte Form. null bei jedem Fehler, error traegt dann
     * die Begruendung im Klartext.
     *
     * Der Aufrufer meldet den Fehler (er kennt Rezept- und Slot-ID); zusaetzlich
     * landet er ueber ctx.Fail() im Ladebericht, sofern ein Bericht gesetzt
     * ist. Doppelt gemeldet ist besser als gar nicht: der Ladebericht ist die
     * Quelle fuer Gate 2 und Gate 3 des Validators.
     */
    static ChefZ_CompiledSelector Compile(ChefZ_SelectorNode src, ChefZ_CompileContext ctx, out string error)
    {
        error = "";

        if (!ctx)
        {
            // Ohne Kontext gibt es keinen Nachschlager, und ohne Nachschlager
            // waere jede Kategorie unbekannt. Das ist ein Programmierfehler,
            // kein Datenfehler - deshalb hart und ohne Ratespiel.
            error = "kein ChefZ_CompileContext uebergeben";
            return null;
        }

        return CompileNode(src, ctx, 0, error);
    }

    private static ChefZ_CompiledSelector CompileNode(ChefZ_SelectorNode src, notnull ChefZ_CompileContext ctx, int depth, out string error)
    {
        error = "";

        if (!src)
        {
            error = "Selektor fehlt";
            return Reject(ctx, error);
        }

        if (depth > ctx.MaxSelectorDepth())
        {
            error = "Selektor ist tiefer als maxSelectorDepth (" + ctx.MaxSelectorDepth().ToString() + ") - meist zyklisches Copy-Paste";
            return Reject(ctx, error);
        }

        int predicates = src.CountPredicates();

        if (predicates > 1)
        {
            error = "Selektor setzt mehrere Praedikate gleichzeitig (" + src.PredicateNames() + ") - nutze allOf, wenn beides gelten soll";
            return Reject(ctx, error);
        }

        if (predicates == 0 && !src.HasConstraints())
        {
            error = "leerer Selektor - er wuerde auf ALLES passen";
            return Reject(ctx, error);
        }

        ChefZ_CompiledSelector node = new ChefZ_CompiledSelector();

        if (predicates == 0)
        {
            // Nur Wertebereiche oder minQuality. Zulaessig, aber fast immer
            // unbeabsichtigt: so ein Slot nimmt JEDE Zutat, die frisch genug
            // ist. TRUE_OP existiert im Entwurf genau fuer diesen Fall
            // (07 §2.2); der Hinweis kostet nichts und faellt im Ladebericht
            // auf.
            node.op = ChefZ_SelectorOp.TRUE_OP;
            ctx.Warn("Selektor ohne Praedikat (nur Wertebereiche/minQuality) - " + "er passt auf jede Zutat, die die Bereiche erfuellt. Beabsichtigt?");
        }
        else if (!CompilePredicate(src, node, ctx, depth, error))
        {
            return Reject(ctx, error);
        }

        if (!CompileRanges(src, node, ctx))
        {
            error = "Wertebereich unbrauchbar";
            return Reject(ctx, error);
        }

        if (!CompileMinQuality(src, node, ctx, error))
            return Reject(ctx, error);

        node.specificity     = ComputeSpecificity(node, ctx);
        node.selectivityHint = EstimateSelectivity(node, ctx);
        return node;
    }

    //! Ein Fehler wird EINMAL gemeldet - an der Stelle, an der er entsteht.
    private static ChefZ_CompiledSelector Reject(ChefZ_CompileContext ctx, string message)
    {
        if (ctx)
            ctx.Fail(message);
        return null;
    }

    private static bool CompilePredicate(notnull ChefZ_SelectorNode src, notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx, int depth, out string error)
    {
        error = "";
        ChefZ_SymbolResolver res = ctx.Resolver();

        if (src.cls != "")
        {
            node.op  = ChefZ_SelectorOp.CLASS;
            node.sym = ctx.Intern(src.cls);
            if (!res.ClassDeclared(node.sym))
            {
                // KEIN Fehler: 07 §7 nennt die unbekannte Klasse nicht. Ein
                // class-Selektor auf eine Klasse aus einem Content-Modul, das
                // auf diesem Server nicht laeuft, matcht schlicht nie - und
                // das Rezept deswegen zu verwerfen waere schlimmer als es
                // liegen zu lassen.
                ctx.Warn("Selektor nennt die Klasse \"" + src.cls + "\", die keine " + "deklarierte Zutat ist - dieser Slot wird nie gefuellt, " + "solange das Content-Modul fehlt.");
            }
            return true;
        }

        if (src.category != "")
        {
            node.op  = ChefZ_SelectorOp.CATEGORY;
            node.sym = ctx.Intern(src.category);
            node.categoryBitIndex = res.CategoryBit(node.sym);
            if (node.categoryBitIndex < 0)
            {
                error = "unbekannte Kategorie \"" + src.category + "\"";
                return false;
            }
            return true;
        }

        if (src.tag != "")
        {
            node.op  = ChefZ_SelectorOp.TAG;
            node.sym = ctx.Intern(src.tag);
            if (!res.TagExists(node.sym))
            {
                error = "unbekannter Tag \"" + src.tag + "\"";
                return false;
            }
            return true;
        }

        if (src.state != "")
        {
            node.op  = ChefZ_SelectorOp.STATE;
            node.sym = ctx.Intern(src.state);
            if (!res.StateExists(node.sym))
            {
                error = "unbekannter Zustand \"" + src.state + "\"";
                return false;
            }
            return true;
        }

        if (src.vanillaStage != "")
        {
            node.op = ChefZ_SelectorOp.VANILLA_STAGE;
            node.vanillaStageValue = ChefZ_VanillaStage.FromName(src.vanillaStage);
            if (node.vanillaStageValue < 0)
            {
                error = "unbekannte Vanilla-Garstufe \"" + src.vanillaStage + "\" - gueltig: " + ChefZ_VanillaStage.ValidNames();
                return false;
            }
            return true;
        }

        if (src.HasLiquidPredicate())
        {
            node.op = ChefZ_SelectorOp.LIQUID;
            node.requireLiquidContainer = src.isLiquidContainer;
            if (src.liquidType != "")
                node.sym = ctx.Intern(src.liquidType);
            return true;
        }

        // Die Kinder werden eingesammelt, nicht direkt gelesen: jede Ebene der
        // Selektorkette hat ihren eigenen Typ, und Enforce-Templates sind
        // invariant - array<ref ChefZ_SelectorL2> ist kein
        // array<ref ChefZ_SelectorNode>. Der Compiler bleibt dadurch EINE
        // Funktion und muss die Ebenen nicht kennen. Warum es die Kette gibt,
        // steht im Kopf von ChefZ_SelectorNode.c.
        if (src.HasAnyOf())
        {
            node.op = ChefZ_SelectorOp.ANY_OF;
            array<ref ChefZ_SelectorNode> anyKinder = new array<ref ChefZ_SelectorNode>();
            src.CollectAnyOf(anyKinder);
            return CompileChildren(anyKinder, node, ctx, depth, "anyOf", error);
        }

        if (src.HasAllOf())
        {
            node.op = ChefZ_SelectorOp.ALL_OF;
            array<ref ChefZ_SelectorNode> allKinder = new array<ref ChefZ_SelectorNode>();
            src.CollectAllOf(allKinder);
            return CompileChildren(allKinder, node, ctx, depth, "allOf", error);
        }

        if (src.GetNot())
        {
            node.op = ChefZ_SelectorOp.NOT;
            string childError;
            ChefZ_CompiledSelector child = CompileNode(src.GetNot(), ctx, depth + 1, childError);
            if (!child)
            {
                // 07 §7: "not mit ungueltigem Kind -> Fehler propagiert nach
                // oben. Kein 'not von nichts'."
                error = "not: " + childError;
                return false;
            }
            node.negated = child;
            return true;
        }

        error = "Selektor ohne auswertbares Praedikat";
        return false;
    }

    private static bool CompileChildren(notnull array<ref ChefZ_SelectorNode> list, notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx, int depth, string label, out string error)
    {
        error = "";

        if (list.Count() == 0)
        {
            error = label + " ist leer";
            return false;
        }

        if (list.Count() > ChefZ_SelectorLimits.MAX_CHILDREN)
        {
            error = label + " hat " + list.Count().ToString() + " Kinder - Obergrenze ist " + ChefZ_SelectorLimits.MAX_CHILDREN.ToString();
            return false;
        }

        node.children = new array<ref ChefZ_CompiledSelector>();

        for (int i = 0; i < list.Count(); i++)
        {
            string childError;
            ChefZ_CompiledSelector child = CompileNode(list.Get(i), ctx, depth + 1, childError);
            if (!child)
            {
                error = label + "[" + i.ToString() + "]: " + childError;
                return false;
            }
            node.children.Insert(child);
        }

        return true;
    }

    /**
     * Wertebereiche anhaengen. Vertauschte Grenzen werden getauscht und
     * gemeldet (07 §7) - die Absicht des Autors ist in dem Fall eindeutig, und
     * ein Rezept dafuer zu verwerfen waere unverhaeltnismaessig.
     *
     * Liefert immer true; der Rueckgabewert steht fuer den Fall bereit, dass
     * ein spaeterer Bereich einmal abweisen soll.
     */
    private static bool CompileRanges(notnull ChefZ_SelectorNode src, notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx)
    {
        AddRange(node, ctx, ChefZ_RangeConstraint.HEALTH,       src.health);
        AddRange(node, ctx, ChefZ_RangeConstraint.FRESHNESS,    src.freshness);
        AddRange(node, ctx, ChefZ_RangeConstraint.TEMPERATURE,  src.temperature);
        AddRange(node, ctx, ChefZ_RangeConstraint.WETNESS,      src.wetness);
        AddRange(node, ctx, ChefZ_RangeConstraint.CLEANNESS,    src.cleanness);
        AddRange(node, ctx, ChefZ_RangeConstraint.QUANTITY,     src.quantity);
        AddRange(node, ctx, ChefZ_RangeConstraint.QUANTITY_PCT, src.quantityPct);
        return true;
    }

    private static void AddRange(notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx, int field, ChefZ_Range range)
    {
        if (!range)
            return;

        // KEINE Warnung. Ein unbegrenzter Bereich ist nicht von einem
        // weggelassenen zu unterscheiden: JsonSerializer legt jedes
        // ref-Feld an, ob es im JSON steht oder nicht (ba6a9d4), und seit
        // ChefZ_Undefined.FLOAT == 0.0 traegt so ein Bereich zwei Sentinel.
        // Ein Selektor ohne "wetness" bekam dadurch eine Warnung fuer
        // "wetness" - siebenmal je Selektor, viertausendmal je Serverstart.
        // Beide Faelle wirken ohnehin gleich: der Bereich schraenkt nichts
        // ein. Wer wirklich "wetness": {} schreibt, bekommt genau das, was
        // dasteht.
        if (range.IsUnbounded())
            return;

        if (!range.IsValid())
        {
            float lo = range.max;
            float hi = range.min;
            ctx.Warn("Wertebereich \"" + ChefZ_RangeConstraint.FieldName(field) + "\" hat min > max (" + range.min.ToString() + " > " + range.max.ToString() + ") - Grenzen getauscht.");
            range.Init(lo, hi);
        }

        if (!node.ranges)
            node.ranges = new array<ref ChefZ_RangeConstraint>();

        ChefZ_RangeConstraint rc = new ChefZ_RangeConstraint();
        rc.Init(field, range);
        node.ranges.Insert(rc);
    }

    /**
     * Qualitaetsschwelle aufloesen.
     *
     * Ergebnis ist eine Aufzaehlung der zulaessigen Stufen, kein Rang -
     * Begruendung im Kopf von ChefZ_CompiledSelector.
     */
    private static bool CompileMinQuality(notnull ChefZ_SelectorNode src, notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx, out string error)
    {
        error = "";

        if (src.minQuality == "")
            return true;

        ChefZ_SymbolResolver res = ctx.Resolver();
        ChefZ_Sym q = ctx.Intern(src.minQuality);

        if (!res.QualityExists(q))
        {
            error = "unbekannte Qualitaetsstufe \"" + src.minQuality + "\"";
            return false;
        }

        node.minQualitySym  = q;
        node.minQualityRank = res.QualityRank(q);

        array<ChefZ_Sym> tiers = new array<ChefZ_Sym>();
        res.QualitiesAtOrAbove(q, tiers);

        if (tiers.Count() == 0)
        {
            // Kann nur passieren, wenn der Nachschlager sich widerspricht -
            // die Stufe existiert, liegt aber in keinem Stufensatz. Abweisen
            // statt "matcht nie" zu bauen.
            error = "Qualitaetsstufe \"" + src.minQuality + "\" laesst sich zu keiner " + "Stufenliste aufloesen";
            return false;
        }

        node.acceptedQualities = new array<ChefZ_Sym>();
        for (int i = 0; i < tiers.Count(); i++)
            node.acceptedQualities.Insert(tiers.Get(i));

        return true;
    }

    //==========================================================================
    // Spezifitaet (09 §4.1)
    //==========================================================================

    /**
     * Rekursive Spezifitaet eines Selektors.
     *
     * Gegenueber 07 §2.2 hat diese Funktion einen zweiten Parameter. Grund:
     * die Gewichte kommen aus Core.json (09 §3), nicht aus dem Code, und ein
     * statischer Zwischenspeicher fuer sie waere globaler Zustand in einer
     * Klasse, die keinen haben darf (07 §6). Der Kontext traegt sie ohnehin.
     *
     * ANY_OF nimmt das MINIMUM der Kinder, nicht die Summe (09 §4.1): der
     * schwaechste Zweig bestimmt, denn der Selektor akzeptiert alles, was
     * dieser Zweig akzeptiert.
     */
    static float ComputeSpecificity(notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx)
    {
        ChefZ_PriorityWeights w = ctx.Weights();
        float s = 0.0;
        int i;

        switch (node.op)
        {
            case ChefZ_SelectorOp.TRUE_OP:
                s = 0.0;
                break;

            case ChefZ_SelectorOp.CLASS:
                s = w.wClass;
                break;

            case ChefZ_SelectorOp.STATE:
                s = w.wState;
                break;

            case ChefZ_SelectorOp.TAG:
                s = w.wTag;
                break;

            case ChefZ_SelectorOp.VANILLA_STAGE:
                s = w.wVanillaStage;
                break;

            case ChefZ_SelectorOp.CATEGORY:
                s = w.wCategoryBase + w.wCategoryPerDepth * CategoryDepthOf(node, ctx);
                break;

            case ChefZ_SelectorOp.LIQUID:
                // 09 §4.1 fuehrt LIQUID nicht auf. Naechstliegendes Gewicht ist
                // wContextBound ("je Temperatur-/Fluessigkeitsbedingung") - eine
                // Fluessigkeitsbedingung ist genau das, nur am Item statt am
                // Geraet.
                s = w.wContextBound;
                break;

            case ChefZ_SelectorOp.ALL_OF:
                if (node.children)
                {
                    for (i = 0; i < node.children.Count(); i++)
                        s = s + ComputeSpecificity(node.children.Get(i), ctx);
                }
                break;

            case ChefZ_SelectorOp.ANY_OF:
                s = MinChildSpecificity(node, ctx);
                break;

            case ChefZ_SelectorOp.NOT:
                s = w.wNot;
                break;
        }

        if (node.ranges)
            s = s + w.wRangePerBound * node.ranges.Count();

        if (ChefZ_SymbolTable.IsValid(node.minQualitySym))
            s = s + w.wMinQuality;

        return s;
    }

    private static float MinChildSpecificity(notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx)
    {
        if (!node.children || node.children.Count() == 0)
            return 0.0;

        float best = ComputeSpecificity(node.children.Get(0), ctx);
        for (int i = 1; i < node.children.Count(); i++)
        {
            float cur = ComputeSpecificity(node.children.Get(i), ctx);
            if (cur < best)
                best = cur;
        }
        return best;
    }

    private static int CategoryDepthOf(notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx)
    {
        int depth = ctx.Resolver().CategoryDepth(node.sym);
        if (depth < 0)
            return 0;
        return depth;
    }

    //==========================================================================
    // selectivityHint (07 E4)
    //==========================================================================

    /**
     * Geschaetzte Zahl der Zutatenklassen, die dieser Selektor treffen kann.
     *
     * Der Matcher sortiert die Slots danach aufsteigend und probiert den am
     * staerksten eingeschraenkten zuerst. 07 E4: "Das ist kein Feinschliff,
     * sondern der Unterschied zwischen 'meist zwei Knoten' und 'regelmaessig
     * Budgetueberschreitung'."
     *
     * Wertebereiche und minQuality gehen NICHT ein: wie viele Items im Topf
     * frisch genug sind, ist beim Boot nicht bekannt. Die Schaetzung ist damit
     * eine Obergrenze, und das ist die richtige Richtung - sie sortiert nie
     * einen Slot nach vorne, der in Wahrheit breit ist.
     */
    static int EstimateSelectivity(notnull ChefZ_CompiledSelector node, notnull ChefZ_CompileContext ctx)
    {
        ChefZ_SymbolResolver res = ctx.Resolver();
        int universe = res.UniverseSize();
        int i;
        int n = 0;
        int cur = 0;

        switch (node.op)
        {
            case ChefZ_SelectorOp.CLASS:
                return 1;

            case ChefZ_SelectorOp.CATEGORY:
            case ChefZ_SelectorOp.TAG:
                return res.EstimateCandidates(node.sym);

            case ChefZ_SelectorOp.ANY_OF:
                if (node.children)
                {
                    for (i = 0; i < node.children.Count(); i++)
                        n = n + EstimateSelectivity(node.children.Get(i), ctx);
                }
                if (n > universe)
                    return universe;
                return n;

            case ChefZ_SelectorOp.ALL_OF:
                if (!node.children || node.children.Count() == 0)
                    return universe;
                n = EstimateSelectivity(node.children.Get(0), ctx);
                for (i = 1; i < node.children.Count(); i++)
                {
                    cur = EstimateSelectivity(node.children.Get(i), ctx);
                    if (cur < n)
                        n = cur;
                }
                return n;
        }

        // STATE, VANILLA_STAGE, LIQUID, NOT, TRUE_OP: nicht ueber die
        // Rueckwaertsindizes schaetzbar. Sie schraenken die KLASSE nicht ein,
        // sondern den Zustand eines Items - deshalb die Obergrenze.
        return universe;
    }

    //==========================================================================
    // Slots
    //==========================================================================

    /**
     * Eine Slotdefinition kompilieren. null bei Fehler.
     *
     * declIndex ist die Stellung in der Rezeptdatei. Sie bleibt erhalten, weil
     * der Verbrauch in DEKLARATIONSreihenfolge geschieht (07 §4, Schritt 6) -
     * die Suchreihenfolge ist eine voellig andere.
     */
    static ChefZ_CompiledSlot CompileSlot(ChefZ_SlotDef src, int declIndex, ChefZ_CompileContext ctx, out string error)
    {
        error = "";

        if (!ctx)
        {
            error = "kein ChefZ_CompileContext uebergeben";
            return null;
        }

        if (!src)
        {
            error = "Slot fehlt";
            ctx.Fail(error);
            return null;
        }

        src.Normalize();
        src.ResolveDefaults();

        ChefZ_CompiledSlot slot = new ChefZ_CompiledSlot();
        slot.slotIndex = declIndex;
        slot.slotId    = src.slotId;

        if (slot.slotId == "")
        {
            // Kein Abbruch: der Slot ist auswertbar. Aber Qualitaetsregeln (12)
            // und der Trace sprechen ihn ueber die ID an, und "" ist als
            // Ansprache unbrauchbar.
            slot.slotId = "slot" + declIndex.ToString();
            ctx.Warn("Slot " + declIndex.ToString() + " hat keine slotId - vergeben wurde \"" + slot.slotId + "\". Grade-Regeln und Trace sprechen Slots ueber die ID an.");
        }
        slot.slotIdSym = ctx.Intern(slot.slotId);

        string selError;
        slot.selector = Compile(src.match, ctx, selError);
        if (!slot.selector)
        {
            error = "Slot \"" + slot.slotId + "\": " + selError;
            return null;
        }

        if (!CompileCounts(src, slot, ctx))
        {
            error = "Slot \"" + slot.slotId + "\": Zaehlbedingung unbrauchbar";
            ctx.Fail(error);
            return null;
        }

        CompileAmount(src, slot, ctx);
        CompileUnit(src, slot, ctx);

        string consumeError;
        if (!CompileConsume(src, slot, ctx, consumeError))
        {
            error = "Slot \"" + slot.slotId + "\": " + consumeError;
            ctx.Fail(error);
            return null;
        }

        string excludeError;
        if (!CompileExcludeStates(src, slot, ctx, excludeError))
        {
            error = "Slot \"" + slot.slotId + "\": " + excludeError;
            ctx.Fail(error);
            return null;
        }

        slot.optional     = src.optional;
        slot.allowPartial = src.allowPartial;
        slot.gradePoints  = src.gradePoints;

        slot.specificity     = slot.selector.specificity;
        slot.selectivityHint = slot.selector.selectivityHint;

        return slot;
    }

    /**
     * Eine ganze Slotliste kompilieren. false, sobald EIN Slot scheitert -
     * 07 §7: "Slot ungueltig -> GANZES Rezept abgewiesen".
     */
    static bool CompileSlotList(array<ref ChefZ_SlotDef> src, ChefZ_CompileContext ctx, out array<ref ChefZ_CompiledSlot> outSlots, out string error)
    {
        error = "";
        if (!outSlots)
            outSlots = new array<ref ChefZ_CompiledSlot>();
        outSlots.Clear();

        if (!src || src.Count() == 0)
        {
            error = "Slotliste ist leer";
            if (ctx)
                ctx.Fail(error);
            return false;
        }

        for (int i = 0; i < src.Count(); i++)
        {
            string slotError;
            ChefZ_CompiledSlot slot = CompileSlot(src.Get(i), i, ctx, slotError);
            if (!slot)
            {
                error = slotError;
                outSlots.Clear();
                return false;
            }

            if (HasDuplicateId(outSlots, slot.slotId) && ctx)
            {
                ctx.Warn("Slot-ID \"" + slot.slotId + "\" kommt mehrfach vor - " + "Grade-Regeln und Trace koennen die beiden nicht unterscheiden.");
            }

            outSlots.Insert(slot);
        }

        return true;
    }

    private static bool HasDuplicateId(notnull array<ref ChefZ_CompiledSlot> slots, string id)
    {
        for (int i = 0; i < slots.Count(); i++)
        {
            if (slots.Get(i).slotId == id)
                return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------

    private static bool CompileCounts(notnull ChefZ_SlotDef src, notnull ChefZ_CompiledSlot slot, notnull ChefZ_CompileContext ctx)
    {
        slot.minCount = src.minCount;
        slot.maxCount = src.maxCount;

        if (slot.minCount < 0)
        {
            ctx.Warn("Slot \"" + slot.slotId + "\": minCount " + slot.minCount.ToString() + " ist negativ - auf 0 gesetzt.");
            slot.minCount = 0;
        }

        if (slot.maxCount < slot.minCount)
        {
            // 07 §7: "minCount > maxCount -> maxCount = minCount, WARN."
            ctx.Warn("Slot \"" + slot.slotId + "\": maxCount " + slot.maxCount.ToString() + " liegt unter minCount " + slot.minCount.ToString() + " - maxCount auf minCount gesetzt.");
            slot.maxCount = slot.minCount;
        }

        if (!src.optional && slot.minCount == 0)
        {
            ctx.Warn("Slot \"" + slot.slotId + "\": minCount 0 an einem Pflichtslot - " + "er ist damit faktisch optional. Deutlicher waere \"optional\": true.");
        }

        return true;
    }

    private static void CompileAmount(notnull ChefZ_SlotDef src, notnull ChefZ_CompiledSlot slot, notnull ChefZ_CompileContext ctx)
    {
        if (!src.amount)
            return;

        // Wie bei AddRange: unbegrenzt und weggelassen sind dasselbe, und
        // eine Warnung darueber ist nicht zu befolgen.
        if (src.amount.IsUnbounded())
            return;

        if (!src.amount.IsValid())
        {
            float lo = src.amount.max;
            float hi = src.amount.min;
            ctx.Warn("Slot \"" + slot.slotId + "\": amount hat min > max - Grenzen getauscht.");
            src.amount.Init(lo, hi);
        }

        if (src.amount.HasMin() && src.amount.min <= 0.0)
        {
            // 07 §7: "amount <= 0 -> auf 1 gesetzt, WARN."
            ctx.Warn("Slot \"" + slot.slotId + "\": amount.min " + src.amount.min.ToString() + " ist nicht positiv - auf 1 gesetzt.");
            src.amount.min = 1.0;
            if (src.amount.HasMax() && src.amount.max < 1.0)
                src.amount.max = 1.0;
        }

        slot.amount = src.amount;
    }

    private static void CompileUnit(notnull ChefZ_SlotDef src, notnull ChefZ_CompiledSlot slot, notnull ChefZ_CompileContext ctx)
    {
        if (src.unit == "")
            return;

        slot.unitSym = ctx.Intern(src.unit);

        if (!ctx.Resolver().UnitExists(slot.unitSym))
        {
            // 07 §7: "unit gesetzt, aber keine Klasse fuehrt diese Einheit ->
            // kein Kandidat -> Slot nie erfuellbar -> kein Match -> Vanilla.
            // Beim Build WARN, weil es fast sicher ein Autorenfehler ist."
            ctx.Warn("Slot \"" + slot.slotId + "\": Einheit \"" + src.unit + "\" wird von keiner " + "deklarierten Zutat gefuehrt - dieser Slot kann nie gefuellt werden.");
        }
    }

    private static bool CompileConsume(notnull ChefZ_SlotDef src, notnull ChefZ_CompiledSlot slot, notnull ChefZ_CompileContext ctx, out string error)
    {
        error = "";
        slot.consumeMode = ChefZ_ConsumeMode.FromName(src.consume);
        if (slot.consumeMode < 0)
        {
            ctx.Warn("Slot \"" + slot.slotId + "\": unbekannter consume-Wert \"" + src.consume + "\" - benutzt wird \"" + ChefZ_ConsumeMode.WHOLE_NAME + "\". Gueltig: " + ChefZ_ConsumeMode.ValidNames() + ".");
            slot.consumeMode = ChefZ_ConsumeMode.WHOLE;
        }

        if (slot.consumeMode == ChefZ_ConsumeMode.AMOUNT)
        {
            slot.consumeAmount = ChefZ_Undefined.FloatOr(src.consumeAmount, 0.0);
            if (slot.consumeAmount <= 0.0)
            {
                // Ohne Menge waere "amount" ein stilles "none" - genau die Art
                // Abweichung, die niemand bemerkt, bis Zutaten nicht mehr
                // verbraucht werden.
                float fallback = slot.RequiredUnits();
                if (fallback <= 0.0)
                    fallback = 1.0;
                ctx.Warn("Slot \"" + slot.slotId + "\": consume \"amount\" ohne brauchbares " + "consumeAmount - benutzt wird " + fallback.ToString() + ".");
                slot.consumeAmount = fallback;
            }
        }

        if (src.setStateAfter != "")
        {
            slot.setStateAfter = ctx.Intern(src.setStateAfter);
            if (!ctx.Resolver().StateExists(slot.setStateAfter))
            {
                error = "unbekannter Zustand \"" + src.setStateAfter + "\" in setStateAfter";
                return false;
            }

            if (slot.consumeMode == ChefZ_ConsumeMode.WHOLE)
            {
                ctx.Warn("Slot \"" + slot.slotId + "\": setStateAfter an einem Slot mit " + "consume \"whole\" - das Item wird geloescht, der Zustandswechsel " + "verpufft. Gemeint ist vermutlich consume \"none\".");
            }
        }

        return true;
    }

    /**
     * excludeStates aufloesen (07 E5, 07 §7).
     *
     * Drei unterscheidbare Faelle, und die Unterscheidung ist der ganze Punkt:
     *   Feld fehlt   -> globaler Default aus CoreSettings
     *   []           -> ausdrueckliche Freigabe, bleibt leer
     *   [Tippfehler] -> Eintrag raus; wird die Liste dadurch LEER, ist das ein
     *                   ERROR, sonst schaltete ein Tippfehler den Filter still ab
     */
    private static bool CompileExcludeStates(notnull ChefZ_SlotDef src, notnull ChefZ_CompiledSlot slot, notnull ChefZ_CompileContext ctx, out string error)
    {
        error = "";
        slot.excludeStates = new array<ChefZ_Sym>();

        if (!src.excludeStates)
        {
            array<ChefZ_Sym> defaults = ctx.DefaultExcludedStates();
            for (int d = 0; d < defaults.Count(); d++)
                slot.excludeStates.Insert(defaults.Get(d));
            return true;
        }

        if (src.excludeStates.Count() == 0)
            return true;

        int dropped = 0;
        for (int i = 0; i < src.excludeStates.Count(); i++)
        {
            string name = src.excludeStates.Get(i);
            if (name == "")
            {
                dropped++;
                continue;
            }

            ChefZ_Sym sym = ctx.Intern(name);
            if (!ctx.Resolver().StateExists(sym))
            {
                ctx.Warn("Slot \"" + slot.slotId + "\": unbekannter Zustand \"" + name + "\" in excludeStates - Eintrag entfernt.");
                dropped++;
                continue;
            }

            if (slot.excludeStates.Find(sym) < 0)
                slot.excludeStates.Insert(sym);
        }

        if (slot.excludeStates.Count() == 0 && dropped > 0)
        {
            error = "excludeStates enthaelt ausschliesslich unbekannte Zustaende - der Filter " + "waere damit still abgeschaltet";
            return false;
        }

        return true;
    }
}
