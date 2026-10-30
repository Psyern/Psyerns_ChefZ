//==============================================================================
// ChefZ_CookbookUIMenu - das aufgeschlagene Kochbuch
//
// Hiess bis zum 30.10.2026 ChefZ_CookbookMenu - derselbe Name wie das Menue in
// ChefZ_Cookbook/Scripts/5_Mission. Mit beiden Addons brach der Mission-Compile
// ab ("Multiple declaration of class 'ChefZ_CookbookMenu'", Serverstart
// 30.10.2026). Die Datei behaelt ihren Namen.
//
// Links die Rezepte, die der Charakter kennt; rechts die Seite: Bildanleitung
// (vier Felder, ohne Schrift), daneben Geraet, Bedingungen, Zutaten und die
// vier Schritte in der Sprache des Spielers.
//
// ---------------------------------------------------------------------------
// WARUM VANILLA UND NICHT DABS
// ---------------------------------------------------------------------------
// ChefZ_Cookbook_Workflow.md §1 sah Dabs vor und §14 eine Vanilla-Fassung erst
// "wenn sich zeigt, dass Dabs fehlt". Fuer eine Liste plus eine Seite reicht
// UIScriptedMenu mit FindAnyWidget vollstaendig, und ChefZ behaelt null
// Fremdabhaengigkeiten (Workflow §2.3). Das Muster ist Vanillas BookMenu
// (5_Mission/DayZ/GUI/BookMenu.c) und DigitalCodeLockUI fuer das Schliessen.
//
// ---------------------------------------------------------------------------
// WAS ANGEZEIGT WIRD
// ---------------------------------------------------------------------------
// Der Zustand je Rezept kommt aus ChefZ_KnowledgeManager.DeriveStatus, also
// aus derselben Ableitung wie die Fortschrittszeile. UNKNOWN fehlt in der
// Liste, PARTIAL steht als "???" mit verdeckten Zutaten und blassem Bild,
// KNOWN voll, MASTERED zusaetzlich markiert (Workflow §4.2).
//
// Der Clientspiegel ist nie die Wahrheit (Workflow §5). Ist er beim Oeffnen
// leer, fragt das Menue den Server einmal nach dem Stand und baut die Liste
// neu, sobald er ankommt.
//
// Layer: 4_World - die Klasse braucht PlayerBase, und ChefZ_CookbookOpener,
// der sie oeffnet, liegt ebenfalls in 4_World.
//==============================================================================

class ChefZ_CookbookUIMenu : UIScriptedMenu
{
    static const string LAYOUT       = "ChefZ_Cookbook_UI/GUI/layouts/chefz_cookbook.layout";
    static const string ENTRY_LAYOUT = "ChefZ_Cookbook_UI/GUI/layouts/chefz_cookbook_entry.layout";

    //! Dieselbe Eingabe wie ChefZ_CookbookInput.TOGGLE_INPUT. Abgeschrieben,
    //! nicht referenziert: jene Klasse liegt in 5_Mission, dieses Menue in
    //! 4_World, und eine untere Ebene sieht die obere nicht.
    static const string TOGGLE_INPUT = "UAChefZCookbookToggle";

    //! Kennung der Listenknoepfe. Weit weg von den IDC_-Werten der Vanilla-
    //! Menues, damit ein Klick nie als deren Knopf gelesen wird.
    static const int ENTRY_ID_BASE = 7700;

    //! Wie oft der Clientspiegel auf Aenderung geprueft wird, in Sekunden.
    static const float REFRESH_SECONDS = 0.5;

    //! Alpha des Bildes auf einer teilbekannten Seite.
    static const float PARTIAL_ALPHA = 0.25;

    protected ref array<ref ChefZ_CookbookPage> m_ChefZ_Pages;
    protected int   m_ChefZ_Selected;
    protected int   m_ChefZ_StateStamp;
    protected float m_ChefZ_RefreshTimer;
    protected bool  m_ChefZ_StateRequested;

    //! Erstes Bild nach dem Oeffnen: keine Schliesstaste gilt. Vorbild ist
    //! Vanillas MapMenu (m_IsOpenning, MapMenu.c:53/:296/:306) - die Taste,
    //! die das Buch geoeffnet hat, ist in diesem Bild noch "gedrueckt".
    protected bool  m_ChefZ_Opening;
    //! Die Kochbuchtaste wurde WAEHREND das Buch offen war gedrueckt.
    //! Geschlossen wird erst beim Loslassen: im Bild des Loslassens meldet
    //! LocalPress() nichts mehr, und ChefZ_CookbookInput.Poll oeffnet nicht
    //! sofort wieder, falls die Mission nach dem Menue aktualisiert wird.
    protected bool  m_ChefZ_ToggleArmed;

    protected TextWidget          m_ChefZ_Progress;
    protected WrapSpacerWidget    m_ChefZ_List;
    protected TextWidget          m_ChefZ_DishName;
    protected ImageWidget         m_ChefZ_Image;
    protected TextWidget          m_ChefZ_NoImage;
    protected TextWidget          m_ChefZ_Device;
    protected TextWidget          m_ChefZ_Condition;
    protected MultilineTextWidget m_ChefZ_Ingredients;
    protected MultilineTextWidget m_ChefZ_Steps;
    protected TextWidget          m_ChefZ_Empty;
    protected ButtonWidget        m_ChefZ_Close;

    void ChefZ_CookbookUIMenu()
    {
        m_ChefZ_Pages    = new array<ref ChefZ_CookbookPage>();
        m_ChefZ_Selected = -1;
        m_ChefZ_Opening  = true;
    }

    override Widget Init()
    {
        layoutRoot = g_Game.GetWorkspace().CreateWidgets(LAYOUT);

        Class.CastTo(m_ChefZ_Progress,    layoutRoot.FindAnyWidget("Progress"));
        Class.CastTo(m_ChefZ_List,        layoutRoot.FindAnyWidget("RecipeList"));
        Class.CastTo(m_ChefZ_DishName,    layoutRoot.FindAnyWidget("DishName"));
        Class.CastTo(m_ChefZ_Image,       layoutRoot.FindAnyWidget("PageImage"));
        Class.CastTo(m_ChefZ_NoImage,     layoutRoot.FindAnyWidget("NoImageHint"));
        Class.CastTo(m_ChefZ_Device,      layoutRoot.FindAnyWidget("DeviceLine"));
        Class.CastTo(m_ChefZ_Condition,   layoutRoot.FindAnyWidget("ConditionLine"));
        Class.CastTo(m_ChefZ_Ingredients, layoutRoot.FindAnyWidget("Ingredients"));
        Class.CastTo(m_ChefZ_Steps,       layoutRoot.FindAnyWidget("Steps"));
        Class.CastTo(m_ChefZ_Empty,       layoutRoot.FindAnyWidget("EmptyHint"));
        Class.CastTo(m_ChefZ_Close,       layoutRoot.FindAnyWidget("CloseButton"));

        ChefZ_Rebuild();
        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();
        ChefZ_RequestStateIfEmpty();
    }

    override void OnHide()
    {
        super.OnHide();
        // Die Instanz haelt ChefZ_CookbookUIHost. Freigegeben wird im naechsten
        // Bild, nicht mitten in dem Aufruf, der sie gerade schliesst.
        if (g_Game)
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).Call(ChefZ_CookbookUIHost.Release);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        super.OnClick(w, x, y, button);
        if (!w)
            return false;

        if (w == m_ChefZ_Close)
        {
            Close();
            return true;
        }

        int index = w.GetUserID() - ENTRY_ID_BASE;
        if (index >= 0 && index < m_ChefZ_Pages.Count())
        {
            ChefZ_Select(index);
            return true;
        }
        return false;
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        if (m_ChefZ_Opening)
        {
            m_ChefZ_Opening = false;
        }
        else if (ChefZ_WantsClose())
        {
            Close();
            return;
        }

        m_ChefZ_RefreshTimer = m_ChefZ_RefreshTimer + timeslice;
        if (m_ChefZ_RefreshTimer < REFRESH_SECONDS)
            return;
        m_ChefZ_RefreshTimer = 0;

        if (ChefZ_CurrentStamp() != m_ChefZ_StateStamp)
            ChefZ_Rebuild();
    }

    //==========================================================================
    // Schliessen: ESC, Menuetaste und die Kochbuchtaste selbst
    //==========================================================================

    protected bool ChefZ_WantsClose()
    {
        if (!g_Game)
            return true;

        Input input = g_Game.GetInput();
        if (input)
        {
            if (input.LocalPress("UAUIBack", false))
                return true;
            if (input.LocalPress("UAUIMenu", false))
                return true;
        }

        UAInputAPI api = GetUApi();
        if (api)
        {
            UAInput toggle = api.GetInputByName(TOGGLE_INPUT);
            if (toggle)
            {
                if (toggle.LocalPress())
                    m_ChefZ_ToggleArmed = true;
                else if (m_ChefZ_ToggleArmed && (toggle.LocalRelease() || toggle.LocalValue() == 0))
                    return true;
            }
        }

        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler || !spieler.IsAlive() || spieler.IsUnconscious())
            return true;
        return false;
    }

    //==========================================================================
    // Liste
    //==========================================================================

    protected ChefZ_KnowledgeState ChefZ_State()
    {
        if (!g_Game)
            return null;
        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return null;
        return spieler.ChefZ_GetKnowledge();
    }

    //! Aendert sich, sobald der Server einen neuen Stand geschickt hat.
    protected int ChefZ_CurrentStamp()
    {
        ChefZ_KnowledgeState stand = ChefZ_State();
        if (!stand)
            return -1;
        return stand.IngredientCount() * 1000 + stand.MasteredCount();
    }

    protected void ChefZ_RequestStateIfEmpty()
    {
        if (m_ChefZ_StateRequested || !g_Game)
            return;
        ChefZ_KnowledgeState stand = ChefZ_State();
        if (!stand || stand.IngredientCount() > 0 || stand.MasteredCount() > 0)
            return;

        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return;
        m_ChefZ_StateRequested = true;
        Param1<bool> leer = new Param1<bool>(true);
        g_Game.RPCSingleParam(spieler, ChefZ_CookbookRPC.REQUEST_STATE, leer, true);
    }

    protected void ChefZ_Rebuild()
    {
        string vorher = "";
        if (m_ChefZ_Selected >= 0 && m_ChefZ_Selected < m_ChefZ_Pages.Count())
            vorher = m_ChefZ_Pages.Get(m_ChefZ_Selected).recipe.id;

        ChefZ_ClearList();
        m_ChefZ_Pages.Clear();
        m_ChefZ_Selected = -1;
        m_ChefZ_StateStamp = ChefZ_CurrentStamp();

        ChefZ_KnowledgeState stand = ChefZ_State();
        ChefZ_KnowledgeManager km = ChefZ_KnowledgeManager.Get();
        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        if (!stand || !km || !engine || !engine.IsReady())
        {
            ChefZ_ShowEmpty();
            return;
        }

        // Sortiert wird ueber einen Schluessel: bekannte vor teilbekannten,
        // innerhalb davon nach Name. Die laufende Nummer haelt gleichnamige
        // Varianten auseinander.
        map<string, ref ChefZ_CookbookPage> nachSchluessel = new map<string, ref ChefZ_CookbookPage>();
        array<string> schluessel = new array<string>();
        for (int i = 0; i < engine.GetRecipeCount(); i++)
        {
            ChefZ_CompiledRecipe rezept = engine.GetRecipeAt(i);
            if (!rezept)
                continue;
            int status = km.DeriveStatus(stand, rezept);
            if (!ChefZ_RecipeStatus.IsListed(status))
                continue;

            ChefZ_CookbookPage seite = ChefZ_CookbookPage.Build(rezept, stand, status);
            string rang = "0";
            if (!ChefZ_RecipeStatus.ShowsName(status))
                rang = "1";
            string k = rang + seite.title + "|" + i.ToStringLen(4);
            nachSchluessel.Insert(k, seite);
            schluessel.Insert(k);
        }
        schluessel.Sort();

        for (int j = 0; j < schluessel.Count(); j++)
            m_ChefZ_Pages.Insert(nachSchluessel.Get(schluessel.Get(j)));

        ChefZ_MarkVariants();
        ChefZ_FillList();
        ChefZ_UpdateProgress(stand, km);

        if (m_ChefZ_Pages.Count() == 0)
        {
            ChefZ_ShowEmpty();
            return;
        }

        int ziel = 0;
        for (int n = 0; n < m_ChefZ_Pages.Count(); n++)
        {
            if (m_ChefZ_Pages.Get(n).recipe.id == vorher)
                ziel = n;
        }
        ChefZ_Select(ziel);
    }

    //! Fuenf Gerichte haben mehrere Rezepte (kleine Portion, Gruppenportion,
    //! Bruehe-Variante). Zwei gleich betitelte Eintraege waeren fuer den Leser
    //! ein Fehler; die zweite und jede weitere bekommt eine Nummer.
    protected void ChefZ_MarkVariants()
    {
        map<string, int> gesehen = new map<string, int>();
        for (int i = 0; i < m_ChefZ_Pages.Count(); i++)
        {
            ChefZ_CookbookPage seite = m_ChefZ_Pages.Get(i);
            if (!ChefZ_RecipeStatus.ShowsName(seite.status))
                continue;
            int schon = 0;
            if (gesehen.Contains(seite.title))
                schon = gesehen.Get(seite.title);
            gesehen.Set(seite.title, schon + 1);
            if (schon > 0)
            {
                int nummer = schon + 1;
                seite.title = seite.title + " " + string.Format(ChefZ_CookbookPage.Tr("#STR_CHEFZ_CB_VARIANT"), nummer.ToString());
            }
        }
    }

    protected void ChefZ_FillList()
    {
        if (!m_ChefZ_List)
            return;
        for (int i = 0; i < m_ChefZ_Pages.Count(); i++)
        {
            ChefZ_CookbookPage seite = m_ChefZ_Pages.Get(i);
            Widget zeile = g_Game.GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_ChefZ_List);
            if (!zeile)
                continue;

            ButtonWidget knopf = ButtonWidget.Cast(zeile.FindAnyWidget("EntryButton"));
            if (knopf)
                knopf.SetUserID(ENTRY_ID_BASE + i);

            TextWidget name = TextWidget.Cast(zeile.FindAnyWidget("EntryName"));
            if (name)
                name.SetText(seite.title);

            TextWidget marke = TextWidget.Cast(zeile.FindAnyWidget("EntryMark"));
            if (marke && seite.status == ChefZ_RecipeStatus.MASTERED)
                marke.SetText(ChefZ_CookbookPage.Tr("#STR_CHEFZ_CB_MASTERED_MARK"));
        }
        m_ChefZ_List.Update();
    }

    protected void ChefZ_ClearList()
    {
        if (!m_ChefZ_List)
            return;
        Widget kind = m_ChefZ_List.GetChildren();
        while (kind)
        {
            Widget naechstes = kind.GetSibling();
            kind.Unlink();
            kind = naechstes;
        }
    }

    protected void ChefZ_UpdateProgress(ChefZ_KnowledgeState stand, ChefZ_KnowledgeManager km)
    {
        if (!m_ChefZ_Progress)
            return;
        int gelistet;
        int gesamt;
        int gemeistert;
        km.CountProgress(stand, gelistet, gesamt, gemeistert);
        string zeile = string.Format(ChefZ_CookbookPage.Tr("#STR_CHEFZ_CB_PROGRESS"), gelistet.ToString(), gesamt.ToString(), gemeistert.ToString());
        m_ChefZ_Progress.SetText(zeile);
    }

    //==========================================================================
    // Seite
    //==========================================================================

    protected void ChefZ_Select(int index)
    {
        if (index < 0 || index >= m_ChefZ_Pages.Count())
            return;
        m_ChefZ_Selected = index;
        ChefZ_CookbookPage seite = m_ChefZ_Pages.Get(index);

        ChefZ_ShowPageWidgets(true);
        m_ChefZ_DishName.SetText(seite.title);
        m_ChefZ_Device.SetText(seite.deviceLine);
        m_ChefZ_Condition.SetText(seite.conditionLine);
        m_ChefZ_Ingredients.SetText(seite.ingredients);
        m_ChefZ_Steps.SetText(seite.steps);

        bool hatBild = seite.image != "";
        if (hatBild)
            hatBild = m_ChefZ_Image.LoadImageFile(0, seite.image);
        m_ChefZ_Image.Show(hatBild);
        m_ChefZ_NoImage.Show(!hatBild);
        if (hatBild)
        {
            m_ChefZ_Image.SetImage(0);
            if (seite.status == ChefZ_RecipeStatus.PARTIAL)
                m_ChefZ_Image.SetAlpha(PARTIAL_ALPHA);
            else
                m_ChefZ_Image.SetAlpha(1.0);
        }
    }

    protected void ChefZ_ShowEmpty()
    {
        ChefZ_ShowPageWidgets(false);
        if (m_ChefZ_Empty)
            m_ChefZ_Empty.Show(true);
    }

    protected void ChefZ_ShowPageWidgets(bool zeigen)
    {
        if (m_ChefZ_Empty)
            m_ChefZ_Empty.Show(!zeigen);
        m_ChefZ_DishName.Show(zeigen);
        m_ChefZ_Image.Show(zeigen);
        m_ChefZ_NoImage.Show(false);
        m_ChefZ_Device.Show(zeigen);
        m_ChefZ_Condition.Show(zeigen);
        m_ChefZ_Ingredients.Show(zeigen);
        m_ChefZ_Steps.Show(zeigen);

        Widget kopf = layoutRoot.FindAnyWidget("IngredientsHeader");
        if (kopf)
            kopf.Show(zeigen);
        kopf = layoutRoot.FindAnyWidget("StepsHeader");
        if (kopf)
            kopf.Show(zeigen);
    }
}
