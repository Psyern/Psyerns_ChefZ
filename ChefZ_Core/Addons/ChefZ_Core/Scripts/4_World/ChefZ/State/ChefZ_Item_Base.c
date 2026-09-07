//==============================================================================
// ChefZ_Item_Base - Basis der NICHT essbaren ChefZ-Items
//
// Entwurf: 06 §4.3 ("Fuer nicht-essbare ChefZ-Items: Mehl, Salz, Wursthuelle,
// Teig. class ChefZ_Item_Base : ItemBase { dieselben Variablen, dieselbe
// API }").
//
// Der Unterschied zu ChefZ_Edible_Base ist genau einer: es gibt keine
// FoodStage. Damit entfaellt
//
//   - die Projektion auf eine Vanilla-Garstufe (06 §3, Schritt 3),
//   - OnFoodStageChange und die Regel "Vanilla gewinnt" (06 E5),
//   - CanDecay aus dem Zutatenbinding - ItemBase kennt keinen Nahrungsverfall.
//
// Alles andere ist dasselbe, und zwar buchstaeblich: beide Klassen halten
// denselben ChefZ_ItemStateComponent und leiten an dieselben statischen
// Methoden weiter. Die Doppelung beschraenkt sich damit auf Weiterleitungen -
// die Logik steht genau einmal (siehe Kopf von ChefZ_ItemStateComponent).
//
// Ein Zustand auf einem nicht essbaren Item ist kein Widerspruch: "Teig,
// gegangen" oder "Wursthuelle, gewaessert" sind Zwischenzustaende, die das
// System kennen muss und der Spieler nicht unbedingt sieht. Genau dafuer ist
// die Zustandsvariable da (06 §2).
//
// FUER CONTENT-AUTOREN: dieselbe Andockregel wie bei ChefZ_Edible_Base - die
// Configklasse erbt von einer Vanilla-Klasse, die Skriptklasse von hier. Der
// Core bringt keinen CfgVehicles-Eintrag mit.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Item_Base extends ItemBase
{
    protected ref ChefZ_ItemStateComponent m_ChefZ_State;

    void ChefZ_Item_Base()
    {
        m_ChefZ_State = new ChefZ_ItemStateComponent();
        ChefZ_ItemStateComponent.RegisterNetSync(this);
    }

    ChefZ_ItemStateComponent ChefZ_StateBlock()
    {
        return m_ChefZ_State;
    }

    //==========================================================================
    // Die API aus 06 §4.3
    //==========================================================================

    ChefZ_Sym ChefZ_GetState()
    {
        return ChefZ_ItemStateComponent.GetState(this);
    }

    //! applyVanillaTransition bleibt in der Signatur, obwohl es hier nichts
    //! bewirken kann: der Aufrufer (Applicator, ProcessRunner) haelt einen
    //! ItemBase und weiss nicht, welcher der beiden Aeste vor ihm steht. Eine
    //! Signatur, die sich unterscheidet, waere an jeder Aufrufstelle eine
    //! Fallunterscheidung.
    bool ChefZ_SetState(ChefZ_Sym state, bool applyVanillaTransition = false)
    {
        return ChefZ_ItemStateComponent.SetState(this, state, applyVanillaTransition);
    }

    ChefZ_Sym ChefZ_GetQuality()
    {
        return m_ChefZ_State.ResolveQualitySym();
    }

    void ChefZ_SetQuality(ChefZ_Sym tier)
    {
        ChefZ_ItemStateComponent.SetQuality(this, tier);
    }

    float ChefZ_GetFreshness01()
    {
        return m_ChefZ_State.GetFreshness01();
    }

    void ChefZ_SetFreshness01(float v)
    {
        ChefZ_ItemStateComponent.SetFreshness01(this, v);
    }

    int ChefZ_GetPortions()
    {
        return m_ChefZ_State.GetPortions();
    }

    /**
     * Portionszaehler setzen (15 §3).
     *
     * @param max  die Hoechstzahl fuer die Anzeige "3 / 8". Auf DIESER Klasse
     *        wird sie ignoriert: sie hat keinen Platz dafuer, und ein zweiter
     *        gesyncter int auf jedem ChefZ-Item waere Sync-Last fuer eine
     *        Zahl, die nur ein Portionsgericht braucht. Getragen wird sie von
     *        ChefZ_PortionedFood_Base, das sie ueberschreibt.
     *
     * Der Parameter steht trotzdem HIER in der Signatur, damit der Aufrufer
     * (Applicator, ProcessRunner) einen ItemBase halten und rufen kann, ohne
     * vorher zu wissen, welcher der Aeste vor ihm steht - dieselbe
     * Ueberlegung wie bei applyVanillaTransition in ChefZ_SetState.
     *
     * @return true, wenn der Zaehler gesetzt wurde. false heisst "kein
     *         ChefZ-Zustandsblock oder nicht auf dem Server" und ist kein
     *         Fehler.
     */
    bool ChefZ_SetPortions(int count, int max = -1)
    {
        return ChefZ_ItemStateComponent.SetPortions(this, count);
    }

    //! Die Hoechstzahl. Auf dieser Klasse gleich dem aktuellen Stand - sie
    //! fuehrt keinen eigenen Wert (siehe ChefZ_SetPortions).
    int ChefZ_GetPortionsMax()
    {
        return ChefZ_GetPortions();
    }

    //! true nur auf ChefZ_PortionedFood_Base (15 §3). Hier immer false: eine
    //! Klasse ohne Entnahmeaktion ist kein Portionsgericht, auch wenn ihr
    //! Zaehler zufaellig gesetzt ist.
    bool ChefZ_IsBulk()
    {
        return false;
    }

    string ChefZ_GetReturnContainer()
    {
        return m_ChefZ_State.GetReturnContainer();
    }

    void ChefZ_SetReturnContainer(string cls)
    {
        ChefZ_ItemStateComponent.SetReturnContainer(this, cls);
    }

    bool ChefZ_IsManaged()
    {
        return m_ChefZ_State != null;
    }

    void ChefZ_InheritFrom(notnull ItemBase src, float freshnessCarry)
    {
        ChefZ_ItemStateComponent.InheritFrom(this, src, freshnessCarry);
    }

    //==========================================================================
    // Persistenz (06 §5, V-B §2 Folge 2: super zuerst, immer)
    //==========================================================================

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ChefZ_ItemStateComponent.Save(this, ctx);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        if (!ChefZ_ItemStateComponent.Load(this, ctx, version))
            return false;

        return true;
    }

    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        ChefZ_ItemStateComponent.ResolveAfterLoad(this);
    }
}
