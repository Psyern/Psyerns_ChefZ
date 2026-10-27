//==============================================================================
// ChefZ_Butter - Skriptklasse nur fuer die Stufen-Optik (27.10.2026).
//
// Erbt weiter von Lard (siehe ChefZ_DairyIngredients.c: Bratfett-Verhalten,
// CanBeCooked). Das Butter-Modell ist MLOD und fuehrt deshalb keine
// Materialliste; die Stufen-Optik laeuft wie bei ChefZ_Edible_Base ueber
// ChefZ_StageVisuals.
//==============================================================================
class ChefZ_Butter extends Lard
{
    protected int m_ChefZ_VisualStageLast = FoodStageType.NONE;

    override void UpdateVisualsEx(bool forced = false)
    {
        if (!ChefZ_StageVisuals.UsesTextureOnly(this))
        {
            super.UpdateVisualsEx(forced);
            return;
        }
        FoodStage stage = GetFoodStage();
        if (!stage)
            return;
        int stageType = stage.GetFoodStageType();
        if (!forced)
        {
            if (stageType == m_ChefZ_VisualStageLast)
                return;
        }
        m_ChefZ_VisualStageLast = stageType;
        ChefZ_StageVisuals.Apply(this, stageType);
    }
}
