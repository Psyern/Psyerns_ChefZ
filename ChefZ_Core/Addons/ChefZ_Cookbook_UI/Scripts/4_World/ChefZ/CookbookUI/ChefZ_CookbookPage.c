//==============================================================================
// ChefZ_CookbookPage - eine Seite des Kochbuchs, aus dem Rezept abgeleitet
//
// ---------------------------------------------------------------------------
// WOHER DER TEXT KOMMT
// ---------------------------------------------------------------------------
// Kein Wort dieser Seite steht im Bild. Die Illustration zeigt nur die vier
// nummerierten Felder (Zutaten, Geraet, Feuer, Ergebnis); alles, was sich
// aendern kann - Mengen, Garzeit, Temperatur, Wasser -, wird HIER aus dem
// kompilierten Rezept der Registry gelesen. Aendert jemand das JSON, stimmt
// das Buch beim naechsten Laden, ohne dass ein Bild neu entsteht.
//
// Die Schrittzeilen 1-4 gehoeren zu den Feldern 1-4 der Illustration. Wer an
// der Bildvorlage (tools/chefz-cookbook-art/prompts.mjs) die Felder aendert,
// muss die Schritte hier mitziehen.
//
// ---------------------------------------------------------------------------
// WOHER DAS BILD KOMMT
// ---------------------------------------------------------------------------
// CfgChefZCookbookPages in der config.cpp dieses Addons, ein Eintrag je
// Ergebnisklasse. Ein neues Gericht bekommt eine Seite, indem dort eine Zeile
// dazukommt - kein Code. Fehlt der Eintrag, zeigt die Seite einen Hinweis
// statt eines Bildes; der Text ist trotzdem vollstaendig.
//
// Layer: 4_World. Liest nur, schreibt nichts.
//==============================================================================

class ChefZ_CookbookPage : Managed
{
    static const string PAGES_CFG = "CfgChefZCookbookPages";

    //! Kein ref: das Rezept gehoert der Engine, die Seite borgt es nur.
    ChefZ_CompiledRecipe recipe;
    int    status;
    string outputClass;
    string title;
    string image;
    string deviceLine;
    string conditionLine;
    string ingredients;
    string steps;

    static ChefZ_CookbookPage Build(notnull ChefZ_CompiledRecipe rezept, ChefZ_KnowledgeState stand, int zustand)
    {
        ChefZ_CookbookPage seite = new ChefZ_CookbookPage();
        seite.recipe = rezept;
        seite.status = zustand;

        if (rezept.outputs && rezept.outputs.Count() > 0 && rezept.outputs.Get(0))
            seite.outputClass = rezept.outputs.Get(0).cls;

        if (ChefZ_RecipeStatus.ShowsName(zustand))
            seite.title = ItemName(seite.outputClass);
        else
            seite.title = Tr("#STR_CHEFZ_CB_UNKNOWN");

        seite.image = ImageOf(seite.outputClass);

        string geraet = DeviceText(rezept);
        string bedingung = ConditionText(rezept);
        string wasser = WaterText(rezept);

        seite.deviceLine = string.Format(Tr("#STR_CHEFZ_CB_DEVICE"), geraet);
        seite.conditionLine = bedingung;
        if (wasser != "")
            seite.conditionLine = bedingung + "  |  " + wasser;

        seite.ingredients = IngredientText(rezept, stand, zustand);
        seite.steps = StepText(seite.title, geraet, bedingung, wasser != "");
        return seite;
    }

    //==========================================================================
    // Bild
    //==========================================================================

    static string ImageOf(string ergebnis)
    {
        if (ergebnis == "" || !g_Game)
            return "";
        string pfad = PAGES_CFG + " " + ergebnis + " image";
        if (!g_Game.ConfigIsExisting(pfad))
            return "";
        string bild;
        g_Game.ConfigGetText(pfad, bild);
        return bild;
    }

    //==========================================================================
    // Geraet, Bedingungen, Wasser
    //==========================================================================

    //! Hoechstens zwei Geraete: "Bratpfanne / Kochtopf". Ein drittes macht die
    //! Zeile laenger, ohne dem Spieler etwas zu sagen, das er nicht schon weiss.
    static string DeviceText(notnull ChefZ_CompiledRecipe rezept)
    {
        if (!rezept.contexts || rezept.contexts.Count() == 0)
            return Tr("#STR_CHEFZ_CB_ANYDEVICE");
        ChefZ_CompiledContext ctx = rezept.contexts.Get(0);
        if (!ctx || ctx.deviceClasses.Count() == 0)
            return Tr("#STR_CHEFZ_CB_ANYDEVICE");

        string s = "";
        int n = ctx.deviceClasses.Count();
        if (n > 2)
            n = 2;
        for (int i = 0; i < n; i++)
        {
            if (s != "")
                s = s + " / ";
            s = s + ItemName(ChefZ_SymbolTable.Name(ctx.deviceClasses.Get(i)));
        }
        return s;
    }

    static string ConditionText(notnull ChefZ_CompiledRecipe rezept)
    {
        string s = "";
        bool zeit = rezept.completion == ChefZ_Completion.TIMED && IsSet(rezept.cookSeconds) && rezept.cookSeconds > 0;
        if (zeit)
        {
            int minuten = Math.Ceil(rezept.cookSeconds / 60.0);
            s = string.Format(Tr("#STR_CHEFZ_CB_TIME"), minuten.ToString());
        }
        else
        {
            s = Tr("#STR_CHEFZ_CB_UNTILDONE");
        }

        if (IsSet(rezept.minTemperature) && rezept.minTemperature > 0)
        {
            int grad = Math.Round(rezept.minTemperature);
            s = s + "  |  " + string.Format(Tr("#STR_CHEFZ_CB_TEMP"), grad.ToString());
        }
        return s;
    }

    static string WaterText(notnull ChefZ_CompiledRecipe rezept)
    {
        if (!rezept.contexts || rezept.contexts.Count() == 0)
            return "";
        ChefZ_CompiledContext ctx = rezept.contexts.Get(0);
        if (!ctx || !ctx.requiresLiquid)
            return "";
        if (ctx.liquidQuantity && ctx.liquidQuantity.HasMin())
        {
            int ml = Math.Round(ctx.liquidQuantity.Lower());
            return string.Format(Tr("#STR_CHEFZ_CB_WATER"), ml.ToString());
        }
        return Tr("#STR_CHEFZ_CB_WATER_ANY");
    }

    //==========================================================================
    // Zutaten
    //==========================================================================

    //! Pflichtzutaten zuerst, dann die optionalen unter eigener Ueberschrift.
    //! Auf einer teilbekannten Seite steht "???" fuer jeden Slot, den der
    //! Wissensstand nicht deckt - entschieden vom Wissensmanager, nicht hier.
    static string IngredientText(notnull ChefZ_CompiledRecipe rezept, ChefZ_KnowledgeState stand, int zustand)
    {
        string pflicht = "";
        string optional = "";
        ChefZ_KnowledgeManager km = ChefZ_KnowledgeManager.Get();
        bool verdecken = zustand == ChefZ_RecipeStatus.PARTIAL && stand && km;

        for (int i = 0; i < rezept.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = rezept.slots.Get(i);
            if (!slot)
                continue;

            string zeile = SlotLine(slot);
            if (verdecken && !km.SlotIsKnown(stand, slot))
                zeile = Tr("#STR_CHEFZ_CB_UNKNOWN");

            if (slot.optional)
                optional = optional + "- " + zeile + "\n";
            else
                pflicht = pflicht + "- " + zeile + "\n";
        }

        if (optional != "")
            return pflicht + "\n" + Tr("#STR_CHEFZ_CB_OPTIONAL") + "\n" + optional;
        return pflicht;
    }

    //! "2-3x Tintenfisch", "100 g Mehl", "Fett / Butter".
    static string SlotLine(notnull ChefZ_CompiledSlot slot)
    {
        string name = SelectorName(slot.selector);

        if (slot.HasAmountRequirement() && slot.amount && slot.amount.HasMin())
        {
            int menge = Math.Round(slot.amount.Lower());
            return menge.ToString() + " " + UnitText(slot.unitSym) + " " + name;
        }

        if (slot.maxCount > slot.minCount && slot.minCount > 0)
            return slot.minCount.ToString() + "-" + slot.maxCount.ToString() + "x " + name;
        if (slot.minCount > 1)
            return slot.minCount.ToString() + "x " + name;
        return name;
    }

    static string UnitText(ChefZ_Sym unit)
    {
        string u = ChefZ_SymbolTable.Name(unit);
        if (u == "GRAM")
            return "g";
        if (u == "ML" || u == "MILLILITER")
            return "ml";
        u.ToLower();
        return u;
    }

    //! Der Selektor in Worten. Bei ODER stehen hoechstens drei Glieder da;
    //! bei UND traegt das erste positive Glied die Bedeutung, die NICHT-Glieder
    //! schraenken nur ein und werden nicht genannt.
    static string SelectorName(ChefZ_CompiledSelector sel)
    {
        if (!sel)
            return "?";

        if (sel.op == ChefZ_SelectorOp.CLASS)
            return ItemName(ChefZ_SymbolTable.Name(sel.sym));
        if (sel.op == ChefZ_SelectorOp.CATEGORY)
            return CategoryName(sel.sym);
        if (sel.op == ChefZ_SelectorOp.TAG)
            return TagName(sel.sym);
        if (sel.op == ChefZ_SelectorOp.LIQUID)
            return Pretty(ChefZ_SymbolTable.Name(sel.sym));

        if (sel.op == ChefZ_SelectorOp.ANY_OF && sel.children)
        {
            string oder = "";
            int n = sel.children.Count();
            if (n > 3)
                n = 3;
            for (int i = 0; i < n; i++)
            {
                if (oder != "")
                    oder = oder + " / ";
                oder = oder + SelectorName(sel.children.Get(i));
            }
            return oder;
        }

        if (sel.op == ChefZ_SelectorOp.ALL_OF && sel.children)
        {
            for (int j = 0; j < sel.children.Count(); j++)
            {
                ChefZ_CompiledSelector kind = sel.children.Get(j);
                if (kind && kind.op != ChefZ_SelectorOp.NOT)
                    return SelectorName(kind);
            }
        }
        return "?";
    }

    static string CategoryName(ChefZ_Sym kategorie)
    {
        ChefZ_CategoryManager cm = ChefZ_CategoryManager.Get();
        if (cm)
        {
            string key = cm.GetDisplayKey(kategorie);
            if (key.IndexOf("#") == 0)
                return Widget.TranslateString(key);
        }
        return Pretty(ChefZ_SymbolTable.Name(kategorie));
    }

    //! Der Core fuehrt den Anzeigenamen eines Tags nicht mit. Die Registry
    //! benennt ihn aber durchgehend STR_CHEFZ_TAG_<id ohne CHEFZ_>; fehlt der
    //! Schluessel, gibt die Engine ihn unuebersetzt zurueck, und es bleibt beim
    //! lesbar gemachten Namen.
    static string TagName(ChefZ_Sym tag)
    {
        string id = ChefZ_SymbolTable.Name(tag);
        string kurz = id;
        if (kurz.IndexOf("CHEFZ_") == 0)
            kurz = kurz.Substring(6, kurz.Length() - 6);
        string key = "STR_CHEFZ_TAG_" + kurz;
        string text = Widget.TranslateString("#" + key);
        if (text == "" || text == key || text == "#" + key)
            return Pretty(id);
        return text;
    }

    //==========================================================================
    // Schritte - eine Zeile je Feld der Illustration
    //==========================================================================

    static string StepText(string gericht, string geraet, string bedingung, bool wasser)
    {
        string s = Tr("#STR_CHEFZ_CB_STEP1") + "\n\n";
        if (wasser)
            s = s + string.Format(Tr("#STR_CHEFZ_CB_STEP2_WATER"), geraet) + "\n\n";
        else
            s = s + string.Format(Tr("#STR_CHEFZ_CB_STEP2"), geraet) + "\n\n";
        s = s + string.Format(Tr("#STR_CHEFZ_CB_STEP3"), bedingung) + "\n\n";
        s = s + string.Format(Tr("#STR_CHEFZ_CB_STEP4"), gericht);
        return s;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    //! Schluessel MIT "#" - so findet der Stringtable-Validator jede
    //! Verwendung und meldet einen Schluessel, der in keiner Tabelle steht.
    static string Tr(string key)
    {
        return Widget.TranslateString(key);
    }

    static string ItemName(string cls)
    {
        if (cls == "" || !g_Game)
            return "?";
        string name;
        string pfad = "CfgVehicles " + cls + " displayName";
        if (g_Game.ConfigIsExisting(pfad))
            g_Game.ConfigGetText(pfad, name);
        if (name == "")
            return Pretty(cls);
        if (name.IndexOf("#") == 0)
            return Widget.TranslateString(name);
        return name;
    }

    //! "ROOT_VEGETABLE" -> "Root vegetable", "ChefZ_Dough" -> "Dough".
    static string Pretty(string id)
    {
        string s = id;
        if (s.IndexOf("ChefZ_") == 0 || s.IndexOf("CHEFZ_") == 0)
            s = s.Substring(6, s.Length() - 6);
        s.Replace("_", " ");
        if (s.Length() == 0)
            return "?";
        string kopf = s.Substring(0, 1);
        string rest = s.Substring(1, s.Length() - 1);
        rest.ToLower();
        return kopf + rest;
    }

    static bool IsSet(float v)
    {
        return !ChefZ_Undefined.IsFloatUndefined(v);
    }
}
