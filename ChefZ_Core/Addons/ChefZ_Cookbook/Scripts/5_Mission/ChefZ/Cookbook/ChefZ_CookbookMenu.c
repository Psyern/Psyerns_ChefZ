//==============================================================================
// ChefZ_CookbookMenu - die Oberflaeche des Kochbuchs
//
// 27.10.2026, Spieltest "Das Kochbuch muss noch das UI bekommen". Bisher war
// ChefZ_CookbookOpener.Open() nur eine Logzeile; das hier vorgesehene Addon
// ChefZ_Cookbook_UI auf Dabs-Basis gab es nie. Dieses Menue braucht kein Dabs:
// ein Vanilla-UIScriptedMenu wie BookMenu.c, ein Layout, eine Liste.
//
// Links die Rezepte, die der Spieler kennt (ChefZ_RecipeStatus.IsListed),
// rechts die aufgeschlagene Seite. Alles wird AUS DEM CLIENTSPIEGEL abgeleitet
// (ChefZ_PlayerKnowledge, FULL_STATE) - genau die Rechnung, die der Server
// auch macht (ChefZ_KnowledgeManager.DeriveStatus). Ist der Spiegel leer, wird
// einmal REQUEST_STATE geschickt und die Liste neu gebaut, sobald er sich
// fuellt.
//
// Schliessen: Esc, die Kochbuchtaste (F9) oder der Knopf. Waehrend das Menue
// offen ist, liest ChefZ_CookbookInput.Poll die Taste nicht (CanReact: "Menue
// offen"), deshalb fragt das Menue sie in Update() selbst ab.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_CookbookMenu extends UIScriptedMenu
{
    static const string LAYOUT = "ChefZ_Cookbook/GUI/layouts/chefz_cookbook.layout";

    //! Wie oft der Clientspiegel auf Aenderungen geprueft wird.
    static const float REFRESH_SEC = 1.0;

    protected TextWidget          m_Progress;
    protected TextListboxWidget   m_List;
    protected TextWidget          m_DetailName;
    protected TextWidget          m_DetailStatus;
    protected MultilineTextWidget m_DetailBody;
    protected ButtonWidget        m_CloseButton;

    //! Je Zeile der Rezeptindex in ChefZ_RecipeEngine. Gehalten, weil die Liste
    //! nur einen schwachen Zeiger auf ihre Nutzdaten hat.
    protected ref array<ref Param1<int>> m_RowData;

    protected bool  m_StateRequested;
    protected float m_RefreshAcc;
    protected int   m_SeenIngredients;
    protected int   m_SeenMastered;
    protected int   m_SelectedRecipe;

    void ChefZ_CookbookMenu()
    {
        m_RowData = new array<ref Param1<int>>();
        m_SelectedRecipe = -1;
        m_SeenIngredients = -1;
        m_SeenMastered = -1;
    }

    override Widget Init()
    {
        layoutRoot = g_Game.GetWorkspace().CreateWidgets(LAYOUT);
        Class.CastTo(m_Progress, layoutRoot.FindAnyWidget("Progress"));
        Class.CastTo(m_List, layoutRoot.FindAnyWidget("RecipeList"));
        Class.CastTo(m_DetailName, layoutRoot.FindAnyWidget("DetailName"));
        Class.CastTo(m_DetailStatus, layoutRoot.FindAnyWidget("DetailStatus"));
        Class.CastTo(m_DetailBody, layoutRoot.FindAnyWidget("DetailBody"));
        Class.CastTo(m_CloseButton, layoutRoot.FindAnyWidget("CloseButton"));
        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();
        g_Game.GetMission().AddActiveInputExcludes({"menu"});
        Rebuild();
    }

    override void OnHide()
    {
        super.OnHide();
        g_Game.GetMission().RemoveActiveInputExcludes({"menu"}, false);
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        UAInputAPI api = GetUApi();
        if (api)
        {
            UAInput toggle = api.GetInputByName(ChefZ_CookbookInput.TOGGLE_INPUT);
            if (toggle && toggle.LocalPress())
            {
                Close();
                return;
            }
            UAInput back = api.GetInputByID(UAUIBack);
            if (back && back.LocalPress())
            {
                Close();
                return;
            }
        }

        m_RefreshAcc = m_RefreshAcc + timeslice;
        if (m_RefreshAcc < REFRESH_SEC)
            return;
        m_RefreshAcc = 0;

        ChefZ_KnowledgeState stand = GetKnowledge();
        if (!stand)
            return;
        if (stand.IngredientCount() != m_SeenIngredients || stand.MasteredCount() != m_SeenMastered)
            Rebuild();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_CloseButton)
        {
            Close();
            return true;
        }
        return super.OnClick(w, x, y, button);
    }

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w != m_List)
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);

        ShowRow(row);
        return true;
    }

    //==========================================================================
    // Liste
    //==========================================================================

    protected ChefZ_KnowledgeState GetKnowledge()
    {
        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return null;
        return spieler.ChefZ_GetKnowledge();
    }

    protected void Rebuild()
    {
        if (!m_List)
            return;

        m_List.ClearItems();
        m_RowData.Clear();

        ChefZ_KnowledgeState stand = GetKnowledge();
        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        ChefZ_KnowledgeManager km = ChefZ_KnowledgeManager.Get();
        if (!stand || !engine || !engine.IsReady() || !km)
        {
            m_Progress.SetText(Tr("STR_CHEFZ_COOKBOOK_LOADING", "Loading..."));
            ShowEmptyPage();
            return;
        }

        m_SeenIngredients = stand.IngredientCount();
        m_SeenMastered = stand.MasteredCount();

        // Leerer Spiegel: einmal beim Server nachfragen. Ein Spieler ohne jede
        // bekannte Zutat bekommt dieselbe leere Antwort - harmlos.
        if (m_SeenIngredients == 0 && m_SeenMastered == 0 && !m_StateRequested)
        {
            m_StateRequested = true;
            RequestState();
        }

        int gelistet = 0;
        int gesamt = 0;
        int gemeistert = 0;
        km.CountProgress(stand, gelistet, gesamt, gemeistert);
        m_Progress.SetText(string.Format(Tr("STR_CHEFZ_COOKBOOK_PROGRESS", "%1 of %2 recipes discovered, %3 mastered"), gelistet.ToString(), gesamt.ToString(), gemeistert.ToString()));

        int selectRow = -1;
        int n = engine.GetRecipeCount();
        for (int i = 0; i < n; i++)
        {
            ChefZ_CompiledRecipe rezept = engine.GetRecipeAt(i);
            if (!rezept)
                continue;
            int status = km.DeriveStatus(stand, rezept);
            if (!ChefZ_RecipeStatus.IsListed(status))
                continue;

            Param1<int> data = new Param1<int>(i);
            m_RowData.Insert(data);
            int row = m_List.AddItem(RowText(stand, km, rezept, status), data, 0);
            m_List.SetItemColor(row, 0, StatusColor(status));
            if (i == m_SelectedRecipe)
                selectRow = row;
        }

        if (m_RowData.Count() == 0)
        {
            ShowEmptyPage();
            return;
        }

        if (selectRow < 0)
            selectRow = 0;
        m_List.SelectRow(selectRow);
        ShowRow(selectRow);
    }

    protected string RowText(ChefZ_KnowledgeState stand, ChefZ_KnowledgeManager km, ChefZ_CompiledRecipe rezept, int status)
    {
        if (ChefZ_RecipeStatus.ShowsName(status))
        {
            string name = RecipeName(rezept);
            if (status == ChefZ_RecipeStatus.MASTERED)
                return "* " + name;
            return name;
        }

        int slotsGesamt = 0;
        int slotsBekannt = 0;
        km.CountSlots(stand, rezept, slotsGesamt, slotsBekannt);
        return Tr("STR_CHEFZ_COOKBOOK_UNKNOWN_NAME", "Unknown recipe") + "  (" + slotsBekannt.ToString() + "/" + slotsGesamt.ToString() + ")";
    }

    protected int StatusColor(int status)
    {
        if (status == ChefZ_RecipeStatus.MASTERED)
            return ARGB(255, 232, 196, 104);
        if (status == ChefZ_RecipeStatus.KNOWN)
            return ARGB(255, 242, 230, 204);
        return ARGB(255, 156, 140, 112);
    }

    protected void RequestState()
    {
        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return;
        Param1<int> leer = new Param1<int>(0);
        g_Game.RPCSingleParam(spieler, ChefZ_CookbookRPC.REQUEST_STATE, leer, true);
    }

    //==========================================================================
    // Seite
    //==========================================================================

    protected void ShowEmptyPage()
    {
        m_SelectedRecipe = -1;
        m_DetailName.SetText(Tr("STR_CHEFZ_BOOK_COOKING", "Cookbook"));
        m_DetailStatus.SetText("");
        m_DetailBody.SetText(Tr("STR_CHEFZ_COOKBOOK_EMPTY", "No pages yet. Pick up ingredients and cook - every ingredient you get to know fills a gap in this book."));
    }

    protected void ShowRow(int row)
    {
        if (row < 0 || row >= m_List.GetNumItems())
            return;

        Class raw;
        m_List.GetItemData(row, 0, raw);
        Param1<int> data = Param1<int>.Cast(raw);
        if (!data)
            return;

        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        ChefZ_KnowledgeManager km = ChefZ_KnowledgeManager.Get();
        ChefZ_KnowledgeState stand = GetKnowledge();
        if (!engine || !km || !stand)
            return;

        ChefZ_CompiledRecipe rezept = engine.GetRecipeAt(data.param1);
        if (!rezept)
            return;

        m_SelectedRecipe = data.param1;
        int status = km.DeriveStatus(stand, rezept);
        bool lesbar = ChefZ_RecipeStatus.ShowsName(status);

        if (lesbar)
            m_DetailName.SetText(RecipeName(rezept));
        else
            m_DetailName.SetText(Tr("STR_CHEFZ_COOKBOOK_UNKNOWN_NAME", "Unknown recipe"));

        int slotsGesamt = 0;
        int slotsBekannt = 0;
        km.CountSlots(stand, rezept, slotsGesamt, slotsBekannt);
        if (status == ChefZ_RecipeStatus.MASTERED)
            m_DetailStatus.SetText(Tr("STR_CHEFZ_COOKBOOK_STATUS_MASTERED", "Mastered - you have cooked this before"));
        else if (status == ChefZ_RecipeStatus.KNOWN)
            m_DetailStatus.SetText(Tr("STR_CHEFZ_COOKBOOK_STATUS_KNOWN", "Known - all ingredients known"));
        else
            m_DetailStatus.SetText(string.Format(Tr("STR_CHEFZ_COOKBOOK_STATUS_PARTIAL", "Partly known - %1 of %2 ingredients"), slotsBekannt.ToString(), slotsGesamt.ToString()));

        string body = Tr("STR_CHEFZ_COOKBOOK_INGREDIENTS", "Ingredients") + ":\n";
        if (rezept.slots)
        {
            for (int i = 0; i < rezept.slots.Count(); i++)
            {
                ChefZ_CompiledSlot slot = rezept.slots.Get(i);
                if (!slot)
                    continue;
                body = body + "  - " + SlotLine(stand, km, slot) + "\n";
            }
        }

        string zubereitung = PreparationText(rezept);
        if (zubereitung != "")
            body = body + "\n" + Tr("STR_CHEFZ_COOKBOOK_PREPARATION", "Preparation") + ":\n" + zubereitung;

        m_DetailBody.SetText(body);
    }

    protected string SlotLine(ChefZ_KnowledgeState stand, ChefZ_KnowledgeManager km, ChefZ_CompiledSlot slot)
    {
        string line;
        if (km.SlotIsKnown(stand, slot))
            line = DescribeSelector(slot.selector);
        else
            line = "???";

        if (line == "")
            line = "???";
        if (slot.minCount > 1)
            line = slot.minCount.ToString() + "x " + line;
        if (slot.optional)
            line = line + " (" + Tr("STR_CHEFZ_COOKBOOK_OPTIONAL", "optional") + ")";
        return line;
    }

    protected string PreparationText(ChefZ_CompiledRecipe rezept)
    {
        string text = "";
        if (rezept.contexts && rezept.contexts.Count() > 0)
        {
            ChefZ_CompiledContext ctx = rezept.contexts.Get(0);
            if (ctx && ctx.deviceClasses && ctx.deviceClasses.Count() > 0)
            {
                string geraete = "";
                for (int i = 0; i < ctx.deviceClasses.Count(); i++)
                {
                    if (i > 0)
                        geraete = geraete + " / ";
                    geraete = geraete + ClassDisplayName(ChefZ_SymbolTable.Name(ctx.deviceClasses.Get(i)));
                }
                text = text + "  " + Tr("STR_CHEFZ_COOKBOOK_DEVICE", "Device") + ": " + geraete + "\n";
            }
            if (ctx && ctx.methods && ctx.methods.Count() > 0)
            {
                string methoden = "";
                for (int m = 0; m < ctx.methods.Count(); m++)
                {
                    if (m > 0)
                        methoden = methoden + " / ";
                    string methode = ChefZ_SymbolTable.Name(ctx.methods.Get(m));
                    methoden = methoden + Tr("STR_CHEFZ_METHOD_" + methode, Pretty(methode));
                }
                text = text + "  " + Tr("STR_CHEFZ_COOKBOOK_METHOD", "Method") + ": " + methoden + "\n";
            }
        }
        if (rezept.cookSeconds > 0)
        {
            float minuten = Math.Ceil(rezept.cookSeconds / 60.0);
            text = text + "  " + string.Format(Tr("STR_CHEFZ_COOKBOOK_TIME", "About %1 min"), minuten.ToString()) + "\n";
        }
        return text;
    }

    //==========================================================================
    // Texte
    //==========================================================================

    //! Name eines Rezepts: der Anzeigename seines ersten Ergebnisses, sonst die ID.
    protected string RecipeName(ChefZ_CompiledRecipe rezept)
    {
        if (rezept.outputs && rezept.outputs.Count() > 0)
        {
            ChefZ_OutputDef out0 = rezept.outputs.Get(0);
            if (out0 && out0.cls != "")
            {
                string name = ClassDisplayName(out0.cls);
                if (name != "")
                    return name;
            }
        }
        return Pretty(rezept.id);
    }

    protected string DescribeSelector(ChefZ_CompiledSelector sel)
    {
        if (!sel)
            return "";

        string name = ChefZ_SymbolTable.Name(sel.sym);
        if (sel.op == ChefZ_SelectorOp.CLASS)
            return ClassDisplayName(name);
        if (sel.op == ChefZ_SelectorOp.CATEGORY)
            return Tr("STR_CHEFZ_CAT_" + name, Pretty(name));
        if (sel.op == ChefZ_SelectorOp.TAG)
            return Pretty(name);
        if (sel.op == ChefZ_SelectorOp.LIQUID)
            return Pretty(name);

        if (sel.op == ChefZ_SelectorOp.ANY_OF || sel.op == ChefZ_SelectorOp.ALL_OF)
        {
            string glue = " / ";
            if (sel.op == ChefZ_SelectorOp.ALL_OF)
                glue = ", ";
            string joined = "";
            if (sel.children)
            {
                for (int i = 0; i < sel.children.Count(); i++)
                {
                    string part = DescribeSelector(sel.children.Get(i));
                    if (part == "")
                        continue;
                    if (joined != "")
                        joined = joined + glue;
                    joined = joined + part;
                }
            }
            return joined;
        }

        if (sel.op == ChefZ_SelectorOp.STATE)
            return Tr("STR_CHEFZ_STATE_" + name, Pretty(name));

        // TRUE_OP, VANILLA_STAGE, NOT: nichts, was ein Koch liest.
        return "";
    }

    protected string ClassDisplayName(string cls)
    {
        if (cls == "")
            return "";
        string text;
        if (!g_Game.ConfigGetText("CfgVehicles " + cls + " displayName", text) || text == "")
            return Pretty(cls);
        if (text.IndexOf("#") == 0 || text.IndexOf("$") == 0)
            text = Widget.TranslateString(text);
        return text;
    }

    //! Stringtable mit Rueckfall: ein unbekannter Schluessel kommt von der
    //! Engine unveraendert zurueck - dann gilt der englische Rueckfalltext.
    static string Tr(string key, string fallback)
    {
        string text = Widget.TranslateString("#" + key);
        if (text == "" || text == key || text == "#" + key)
            return fallback;
        return text;
    }

    //! "CHEFZ_SMOKED_MEAT" -> "Smoked meat". Nur Rueckfall fuer fehlende Texte.
    static string Pretty(string raw)
    {
        string s = raw;
        if (s.IndexOf("ChefZ_") == 0 || s.IndexOf("CHEFZ_") == 0)
            s = s.Substring(6, s.Length() - 6);
        if (s.IndexOf("TR_") == 0)
            s = s.Substring(3, s.Length() - 3);
        s.Replace("_", " ");
        s.ToLower();
        if (s.Length() > 0)
        {
            string first = s.Substring(0, 1);
            first.ToUpper();
            s = first + s.Substring(1, s.Length() - 1);
        }
        return s;
    }
}

//! Zeigt das Menue, falls nicht schon offen. Aufgerufen aus
//! ChefZ_CookbookInput.Poll, wenn ChefZ_CookbookOpener.s_OpenRequested steht.
class ChefZ_CookbookMenuOpener
{
    //! Der UIManager haelt Menues nur schwach: ohne diese Referenz wurde das
    //! Menue direkt nach ShowScriptedMenu wieder freigegeben und ging nie auf
    //! (27.10.2026, Autotest cookbook.menu). Vanilla haelt m_InventoryMenu
    //! genauso. Beim naechsten Oeffnen ersetzt ein frisches Menue das alte.
    static ref ChefZ_CookbookMenu s_Menu;

    static void Show()
    {
        UIManager ui = g_Game.GetUIManager();
        if (!ui)
            return;
        if (ChefZ_CookbookMenu.Cast(ui.GetMenu()))
            return;
        if (ui.GetMenu())
            return;

        s_Menu = new ChefZ_CookbookMenu();
        ui.ShowScriptedMenu(s_Menu, null);
    }
}
