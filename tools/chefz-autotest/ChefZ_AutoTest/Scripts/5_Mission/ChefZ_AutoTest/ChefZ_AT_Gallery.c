//! Sichtpruefung (27.10.2026, "alle Fischfilets sind unsichtbar"): zeigt alle
//! essbaren ChefZ-Klassen mit eigenem (MLOD-)Modell als 3D-Vorschau in einem
//! Raster ueber dem ganzen Bildschirm, nacheinander roh, gebacken, getrocknet.
//! Das Bildschirmfoto macht der Testlauf von aussen (Clientlog "client gallery").
//! Eine leere Kachel = unsichtbares Modell. CarpFilletMeat ist die Vanilla-Referenz.
//! Ein echtes Menue: ItemPreviewWidgets rendern ausserhalb eines Menues nichts
//! (erster Versuch: alle Kacheln leer, auch die Vanilla-Referenz).
class ChefZ_AT_Gallery extends UIScriptedMenu
{
    static ref ChefZ_AT_Gallery s_Instance;

    Widget m_Root;
    TextWidget m_Title;
    ref array<EntityAI> m_Items;
    ref TStringArray m_Classes;
    int m_Phase;

    static void Begin()
    {
        g_Game.GetUIManager().CloseAll();
        s_Instance = new ChefZ_AT_Gallery();
        g_Game.GetUIManager().ShowScriptedMenu(s_Instance, null);
    }

    override Widget Init()
    {
        Build();
        layoutRoot = m_Root;
        return layoutRoot;
    }

    void Build()
    {
        m_Classes = new TStringArray();
        m_Classes.Insert("CarpFilletMeat");
        m_Classes.Insert("ChefZ_Bread");
        m_Classes.Insert("ChefZ_Dough");
        m_Classes.Insert("ChefZ_DriedPasta");
        m_Classes.Insert("ChefZ_Flatbread");
        m_Classes.Insert("ChefZ_RawPasta");
        m_Classes.Insert("ChefZ_BoneBroth");
        m_Classes.Insert("ChefZ_TomatoSauce");
        m_Classes.Insert("ChefZ_Blackberry");
        m_Classes.Insert("ChefZ_Blueberry");
        m_Classes.Insert("ChefZ_Cabbage");
        m_Classes.Insert("ChefZ_Carrot");
        m_Classes.Insert("ChefZ_Corn");
        m_Classes.Insert("ChefZ_Garlic");
        m_Classes.Insert("ChefZ_Onion");
        m_Classes.Insert("ChefZ_Raspberry");
        m_Classes.Insert("ChefZ_Strawberry");
        m_Classes.Insert("ChefZ_BlueMarlinFillet");
        m_Classes.Insert("ChefZ_BlueSentinelCrab");
        m_Classes.Insert("ChefZ_BluefinTunaFillet");
        m_Classes.Insert("ChefZ_BonitoFillet");
        m_Classes.Insert("ChefZ_CharFillet");
        m_Classes.Insert("ChefZ_CoastalCrab");
        m_Classes.Insert("ChefZ_CodFillet");
        m_Classes.Insert("ChefZ_CongerFillet");
        m_Classes.Insert("ChefZ_CrimsonKingCrab");
        m_Classes.Insert("ChefZ_CrimsonSquid");
        m_Classes.Insert("ChefZ_EelFillet");
        m_Classes.Insert("ChefZ_GiantGrouperFillet");
        m_Classes.Insert("ChefZ_GiantTrevallyFillet");
        m_Classes.Insert("ChefZ_GiltheadBreamFillet");
        m_Classes.Insert("ChefZ_HalibutFillet");
        m_Classes.Insert("ChefZ_HorseMackerelFillet");
        m_Classes.Insert("ChefZ_JapaneseSardineFillet");
        m_Classes.Insert("ChefZ_Langoustine");
        m_Classes.Insert("ChefZ_MahiMahiFillet");
        m_Classes.Insert("ChefZ_MidnightLobster");
        m_Classes.Insert("ChefZ_MuskellungeFillet");
        m_Classes.Insert("ChefZ_NorthernCrimsonSquid");
        m_Classes.Insert("ChefZ_Octopus");
        m_Classes.Insert("ChefZ_PerchFillet");
        m_Classes.Insert("ChefZ_PlaiceFillet");
        m_Classes.Insert("ChefZ_RainbowTroutFillet");
        m_Classes.Insert("ChefZ_RedMulletFillet");
        m_Classes.Insert("ChefZ_RedSnapperFillet");
        m_Classes.Insert("ChefZ_RockfishFillet");
        m_Classes.Insert("ChefZ_SailfishFillet");
        m_Classes.Insert("ChefZ_SalmonFillet");
        m_Classes.Insert("ChefZ_SeaBassFillet");
        m_Classes.Insert("ChefZ_SnakeheadFillet");
        m_Classes.Insert("ChefZ_SnowCrab");
        m_Classes.Insert("ChefZ_SockeyeSalmonFillet");
        m_Classes.Insert("ChefZ_SoleFillet");
        m_Classes.Insert("ChefZ_SpeckledSquid");
        m_Classes.Insert("ChefZ_SpinyLobster");
        m_Classes.Insert("ChefZ_SpinyOrangeCrab");
        m_Classes.Insert("ChefZ_Squid");
        m_Classes.Insert("ChefZ_StripedTigerPrawn");
        m_Classes.Insert("ChefZ_TarponFillet");
        m_Classes.Insert("ChefZ_YellowfinTunaFillet");
        m_Classes.Insert("ChefZ_YellowtailFillet");
        m_Classes.Insert("ChefZ_Butter");
        m_Classes.Insert("ChefZ_Cheese");
        m_Classes.Insert("ChefZ_Egg");
        m_Classes.Insert("ChefZ_BeefLeg");
        m_Classes.Insert("ChefZ_BoarSausage");
        m_Classes.Insert("ChefZ_CookedSausage");
        m_Classes.Insert("ChefZ_DicedMeat");
        m_Classes.Insert("ChefZ_HunterSausage");
        m_Classes.Insert("ChefZ_MincedBear");
        m_Classes.Insert("ChefZ_MincedBoar");
        m_Classes.Insert("ChefZ_MincedChicken");
        m_Classes.Insert("ChefZ_MincedMeat");
        m_Classes.Insert("ChefZ_MincedPork");
        m_Classes.Insert("ChefZ_MincedVenison");
        m_Classes.Insert("ChefZ_PorkLeg");
        m_Classes.Insert("ChefZ_PorkSausage");
        m_Classes.Insert("ChefZ_RawBoarSausage");
        m_Classes.Insert("ChefZ_RawHunterSausage");
        m_Classes.Insert("ChefZ_RawPorkSausage");
        m_Classes.Insert("ChefZ_RawSausage");
        m_Classes.Insert("ChefZ_RawSpicySausage");
        m_Classes.Insert("ChefZ_RawVenisonSausage");
        m_Classes.Insert("ChefZ_SpicySausage");
        m_Classes.Insert("ChefZ_VenisonLeg");
        m_Classes.Insert("ChefZ_VenisonSausage");
        m_Classes.Insert("ChefZ_DrySausage");
        m_Classes.Insert("ChefZ_SmokedSausage");
        m_Items = new array<EntityAI>();
        m_Root = g_Game.GetWorkspace().CreateWidgets("ChefZ_AutoTest/gui/at_gallery.layout");
        m_Title = TextWidget.Cast(m_Root.FindAnyWidget("ATGalleryTitle"));
        int cols = 11;
        int rows = 8;
        float cw = 1.0 / cols;
        float ch = 0.97 / rows;
        int made = 0;
        for (int i = 0; i < m_Classes.Count(); i++)
        {
            Widget cell = g_Game.GetWorkspace().CreateWidgets("ChefZ_AutoTest/gui/at_gallery_cell.layout", m_Root);
            int col = i % cols;
            int row = i / cols;
            cell.SetPos(col * cw, 0.03 + row * ch);
            cell.SetSize(cw, ch);
            TextWidget label = TextWidget.Cast(cell.FindAnyWidget("ATLabel"));
            string shortName = m_Classes.Get(i);
            shortName.Replace("ChefZ_", "");
            label.SetText(shortName);
            ItemPreviewWidget preview = ItemPreviewWidget.Cast(cell.FindAnyWidget("ATPreview"));
            // Aufrufreihenfolge wie COT JMLocalPreview (dort zeigt die Vorschau Modelle).
            EntityAI item = EntityAI.Cast(g_Game.CreateObject(m_Classes.Get(i), vector.Zero, true, false, false));
            m_Items.Insert(item);
            if (item)
            {
                item.DisableSimulation(true);
                preview.SetForceFlipEnable(true);
                preview.SetForceFlip(false);
                preview.SetItem(item);
                preview.SetModelOrientation(vector.Zero);
                preview.SetView(item.GetViewIndex());
                preview.Show(true);
                made++;
            }
        }
        m_Phase = 0;
        ShowPhase();
        Print("[CHEFZTEST] client gallery aufgebaut klassen=" + m_Classes.Count().ToString() + " erzeugt=" + made.ToString());
    }

    void ShowPhase()
    {
        FoodStageType stage = FoodStageType.RAW;
        string name = "ROH";
        if (m_Phase == 1)
        {
            stage = FoodStageType.BAKED;
            name = "GEBACKEN";
        }
        if (m_Phase == 2)
        {
            stage = FoodStageType.DRIED;
            name = "GETROCKNET";
        }
        foreach (EntityAI item : m_Items)
        {
            Edible_Base food = Edible_Base.Cast(item);
            if (!food)
                continue;
            if (!food.HasFoodStage())
                continue;
            food.ChangeFoodStage(stage);
            food.UpdateVisualsEx(true);
        }
        m_Title.SetText("ChefZ Sichtpruefung - Stufe " + name + " (leere Kachel = unsichtbar, Carp = Vanilla-Referenz)");
        Print("[CHEFZTEST] client gallery stufe " + name);
        if (m_Phase < 2)
        {
            m_Phase++;
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ShowPhase, 12000, false);
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(End, 12000, false);
    }

    void End()
    {
        foreach (EntityAI item : m_Items)
        {
            if (item)
                g_Game.ObjectDelete(item);
        }
        m_Items = null;
        Print("[CHEFZTEST] client gallery ende");
        Close();
    }
}
