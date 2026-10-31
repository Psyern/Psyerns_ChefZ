//==============================================================================
// ChefZ_StageVisuals - Garstufen-Optik nur ueber Texturen (27.10.2026)
//
// Anlass: "alle Fischfilets sind unsichtbar". Die ChefZ-Modelle sind MLOD.
// Seit ihre Selektionen in einer model.cfg als section stehen, greift
// hiddenSelectionsMaterials wirklich - und JEDES so gesetzte Material (auch
// dz\data\data\default.rvmat) macht diese Modelle unsichtbar. Eine reine
// Textur bleibt sichtbar (Sichtpruefung im Autotest, Klassen ChefZ_AT_Fil*).
//
// Deshalb fuehren diese Klassen KEINE Materialliste mehr
// (hiddenSelectionsMaterials[] = {}). Vanillas FoodStage.UpdateVisualsEx liest
// die Liste aber ungeprueft per Index (FoodStage.c:598) - ein leeres Array
// waere dort ein Zugriff ausserhalb der Grenzen. Klassen ohne Materialliste
// laufen darum hier: dieselbe Auswahl von Selektion und Textur wie Vanilla
// (visual_properties der Stufe), nur ohne SetObjectMaterial.
//==============================================================================
class ChefZ_StageVisuals
{
    //! true = die Klasse hat Selektionen und Texturen, aber keine Materialliste.
    static bool UsesTextureOnly(EntityAI item)
    {
        if (!item)
            return false;
        if (item.GetHiddenSelectionsMaterials().Count() > 0)
            return false;
        if (item.GetHiddenSelections().Count() == 0)
            return false;
        return item.GetHiddenSelectionsTextures().Count() > 0;
    }

    //! Setzt Selektion und Textur der Stufe wie FoodStage.UpdateVisualsEx,
    //! ohne Material. Ungueltige Indizes fallen auf 0 zurueck.
    static void Apply(Edible_Base item, FoodStageType stageType)
    {
        TStringArray selections = item.GetHiddenSelections();
        TStringArray textures = item.GetHiddenSelectionsTextures();
        string path = string.Format("CfgVehicles %1 Food FoodStages %2 visual_properties", item.GetType(), FoodStage.GetFoodStageName(stageType));
        TFloatArray visual = new TFloatArray();
        g_Game.ConfigGetFloatArray(path, visual);

        int selectionIndex = 0;
        int textureIndex = 0;
        if (visual.Count() > 1)
        {
            int sel = visual.Get(0);
            int tex = visual.Get(1);
            bool selValid = sel >= 0;
            if (sel >= selections.Count())
                selValid = false;
            if (selValid)
                selectionIndex = sel;
            bool texValid = tex >= 0;
            if (tex >= textures.Count())
                texValid = false;
            if (texValid)
                textureIndex = tex;
        }

        string shown = selections.Get(selectionIndex);
        for (int i = 0; i < selections.Count(); i++)
        {
            if (selections.Get(i) != shown)
                item.SetAnimationPhase(selections.Get(i), 1);
        }
        item.SetAnimationPhase(shown, 0);
        item.SetObjectTexture(selectionIndex, textures.Get(textureIndex));
    }
}

// Spieltest 27.10.2026 "keine Textur" (RedMulletFillet, TarponFillet ...),
// nur in der COT-Vorschau: der Objekt-Spawner ruft nach dem Stufenwechsel
// FoodStage.UpdateVisualsEx direkt (JMObjectSpawnerProperties.
// RefreshPreviewFoodStage) und umgeht damit ChefZ_Edible_Base.UpdateVisualsEx.
// Vanilla setzt dort ein Material aus der leeren Liste - und das macht diese
// Modelle unsichtbar. Deshalb laufen Klassen ohne Materialliste auch hier
// ueber ChefZ_StageVisuals, egal wer die Optik anstoesst.
modded class FoodStage
{
    override void UpdateVisualsEx(bool forced = false)
    {
        Edible_Base food = GetFoodItem();
        // Klassen auf einem Modell ohne Selektion: nichts zu
        // wechseln, und Vanilla laese die leere Liste per Index (FoodStage.c:595).
        if (food && food.IsInherited(ChefZ_Edible_Base) && food.GetHiddenSelections().Count() == 0)
        {
            m_FoodStageTypeClientLast = m_FoodStageType;
            return;
        }
        if (!food || !ChefZ_StageVisuals.UsesTextureOnly(food))
        {
            super.UpdateVisualsEx(forced);
            return;
        }
        if (!forced && m_FoodStageTypeClientLast == m_FoodStageType)
            return;
        ChefZ_StageVisuals.Apply(food, GetFoodStageType());
        m_FoodStageTypeClientLast = m_FoodStageType;
    }
}
