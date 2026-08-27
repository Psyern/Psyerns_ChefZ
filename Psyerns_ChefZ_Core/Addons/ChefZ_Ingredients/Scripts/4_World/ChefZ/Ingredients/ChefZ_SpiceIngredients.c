//==============================================================================
// ChefZ_SpiceIngredients - die Skriptseite des EINEN garbaren Gewuerzes.
//
// Slice "herbs". Production Map §15 (Paprikakette), §62 (Chernarus Chili).
//
// ---------------------------------------------------------------------------
// Die Frage, die vor dieser Datei stand: soll ein Gewuerzpulver ueberhaupt
// garen?
// ---------------------------------------------------------------------------
// Fuer die uebrigen Gewuerze lautet die Antwort weiter NEIN, und sie steht
// nicht hier, sondern in den Daten: ChefZ_SpiceBase, ChefZ_BlackPepper,
// ChefZ_DriedPeppercorns, ChefZ_HerbMix, ChefZ_HunterSeasoning und die ganze
// ChefZ_DriedHerbBase-Familie haben KEINEN Food-Knoten. HasFoodStage() ist
// damit false (ItemBase.c:2654), es entsteht kein FoodStage-Objekt, und
// CanBeCooked() waere selbst auf ChefZ_Edible_Base false - der erste Test dort
// ist "if (!GetFoodStage()) return false;". Sie bekommen deshalb hier gar
// keine Skriptklasse: kein Zustandsblock, keine Sync-Kost, keine Aenderung.
// Genau das ist gemeint mit "der Core schaltet nur bei vorhandenen
// Uebergaengen ein".
//
// Fuer ChefZ_PaprikaPowder lautet sie JA. Der Grund ist keine Meinung ueber
// Gewuerze, sondern die Bauform von zwei Rezepten:
//
//   RCP_ChefZ_ChernarusChili       Slot "powder", minCount 1, kein "optional"
//   RCP_ChefZ_ChernarusChiliGroup  Slot "powder", minCount 2, kein "optional"
//
// Beide sind completion "ON_STAGE". Damit gilt ChefZ_RecipeEvaluator.
// CheckStages: JEDE gebundene PFLICHTzutat muss in einer erlaubten Endstufe
// stehen. Das Pulver liegt beim Kochen im Kessel, waehrend Vanilla den
// Garzustand fortschreibt - es ist ein Garobjekt, ob man das von einem Pulver
// erwartet oder nicht.
//
// ---------------------------------------------------------------------------
// Warum NICHT der andere saubere Weg (Slot optional machen)
// ---------------------------------------------------------------------------
// Er war ernsthaft im Rennen und ist an drei Punkten gescheitert:
//
//   1. Er loest den Befund nicht. Der Pruefer haengt an der KLASSE, nicht am
//      Rezept: ChefZ_PaprikaPowder deklariert Food > FoodStages UND
//      FoodStageTransitions. Solange dieser Block dasteht und die Skriptkette
//      die Kochbarkeit nicht einschaltet, sind die Uebergaenge toter Text -
//      unabhaengig davon, in welchem Slot die Klasse steht. Der Weg haette
//      also zusaetzlich verlangt, den Food-Knoten wieder zu entfernen.
//   2. Er nimmt dem Gericht seinen Namen. Paprikapulver IST das Chernarus
//      Chili (Production Map §62); optional waere es eine Beilage. Der Slot
//      traegt gradePoints 2 - die hoechste Einzelwertung des Rezepts - genau
//      weil er die Kernzutat bindet.
//   3. Er ist die groessere Aenderung. Zwei Rezepte in einem FREMDEN Modul
//      (ChefZ_Cooking) umschreiben plus einen Food-Knoten in diesem Modul
//      loeschen - gegen EINE Skriptklasse, die die vorhandenen Daten wirksam
//      macht.
//
// Fachlich haelt die Entscheidung stand: eingeruehrtes Paprikapulver kocht im
// Chili mit. Die Naehrwerte des Autors sind ueber die Stufen bewusst fast
// gleich (2/10/0 roh wie gegart) - garen aendert am Pulver nichts ausser
// seiner Vanilla-Garstufe, und genau die braucht das Rezept.
//
// ---------------------------------------------------------------------------
// Warum die Skriptklasse an ChefZ_PaprikaPowder haengt und nicht an
// ChefZ_SpiceBase
// ---------------------------------------------------------------------------
// Die Regel, nach der dieses Modul jetzt durchgehend gebaut ist: die
// ChefZ-Skriptklasse haengt an der Configklasse, die den Knoten
// Food > FoodStageTransitions WIRKLICH BESITZT.
//
// Bei den geschnittenen Gemuesen und beim Farm-Gemuese besitzt ihn die
// gemeinsame Basis - dort haengt die Skriptklasse an der Basis. Beim Pulver
// besitzt ihn die Klasse selbst; ChefZ_SpiceBase hat keinen Food-Knoten.
// Haenge man die Skriptklasse dort an, bekaemen vier weitere Gewuerze einen
// Zustandsblock und einen OnStoreSave-Eintrag, ohne dass sich fuer sie
// irgendetwas aendert. Kochbar wuerden sie dadurch NICHT - der Core prueft je
// Klasse - aber es waere Sync-Kost fuer nichts.
//
// Layer: 4_World.
//==============================================================================

//! Paprikapulver - das einzige Gewuerz mit Garstufen und damit das einzige,
//! das eine ChefZ-Skriptklasse braucht. CanBeCooked() folgt den Daten und
//! liefert hier true; die Uebergaenge Raw -> Baked (BAKING) und
//! Raw -> Boiled (BOILING) stehen in der config.cpp dieses Moduls.
class ChefZ_PaprikaPowder extends ChefZ_Edible_Base
{
    //! Siehe die Begruendung an ChefZ_SpiceBase.SetActions(). Das Pulver
    //! bekommt dieselbe Aktion wie die uebrigen Gewuerze - seine Skriptkette
    //! laeuft nur aus dem oben genannten Grund ueber ChefZ_Edible_Base statt
    //! ueber ChefZ_SpiceBase und erbt sie deshalb nicht.
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeedSmall);
        AddAction(ActionEatSmall);
    }
}

//==============================================================================
// Die beiden Gewuerzbasen - und warum sie doch eine Skriptklasse bekommen
//==============================================================================
// Der Kopf dieser Datei begruendet, warum die Gewuerze ausser dem
// Paprikapulver KEINE Skriptklasse bekommen: sie haben keinen Food-Knoten,
// waeren nie kochbar, und ein ChefZ-Zustandsblock waere Sync-Kost fuer nichts.
// Diese Begruendung gilt unveraendert - fuer den ZUSTAND.
//
// Sie gilt nicht fuer die Essaktion. Vanilla registriert die Essaktion auf
// jeder Nahrungsklasse einzeln und NICHT auf Edible_Base (Potato.c:26-31).
// ChefZ_SpiceBase und ChefZ_DriedHerbBase tragen beide einen
// "class Nutrition"-Block; sie sind damit Nahrung im Sinne von
// PlayerStomach.InitData, werden aber ohne eine ActionEat*-Registrierung nie
// zum Essen angeboten - kein Fehlerbild, keine Logzeile.
//
// Deshalb erben diese beiden Klassen von Edible_Base und NICHT von
// ChefZ_Edible_Base. Das ist der ganze Unterschied und der Grund, warum die
// Entscheidung des Dateikopfs bestehen bleibt:
//
//   Edible_Base       nur die Aktion. Kein m_ChefZ_State, kein OnStoreSave,
//                     kein OnVariablesSynchronized, keine Registry-Abfrage je
//                     Verfallstakt. Genau das, was der Kopf vermeiden wollte.
//   ChefZ_Edible_Base haette zusaetzlich den vollen Zustandsblock gebracht.
//
// Elf Klassen haengen an diesen beiden Basen; die Engine findet sie fuer jede
// ueber die Config-Elternkette.
//
// WARUM NICHT "gar nicht essbar": fuer Salz, Pfeffer und Gewuerzpulver waere
// das die ehrlichere Antwort, und der saubere Weg dorthin ist, ihnen die
// Naehrwerte zu nehmen statt ihnen eine Essaktion zu geben. Dieser Weg ist
// diesem Slice verschlossen:
//
//   1. Die Naehrwerte stehen nicht nur in der config.cpp, sondern auch in der
//      zentralen ChefZ_Registry/Config/Nutrition.json - und die darf dieser
//      Slice nicht anfassen (Workflow §5).
//   2. Jedes dieser Gewuerze ist ERGEBNIS eines Rezepts oder Transforms
//      (ChefZ_BlackPepper, ChefZ_HerbMix, ChefZ_HunterSeasoning,
//      ChefZ_PaprikaPowder, ChefZ_DriedThyme und die uebrige
//      DriedHerb-Familie). Fuer Ergebnisklassen verlangt 01 V7 einen
//      Nutrition- oder Food-Knoten; ohne ihn registriert PlayerStomach sie
//      nie. Die Naehrwerte zu entfernen hiesse, diese Regel zu brechen.
//
// Die Zahlen sagen ohnehin, was gemeint ist: fullnessIndex 2, energy 8 bis 10.
// Ein geloeffeltes Gewuerz saettigt nicht - es ist essbar und wertlos, und
// genau das soll der Spieler merken duerfen.
//==============================================================================

//! Basis der Gewuerzpulver (§71): schwarzer Pfeffer, Pfefferkoerner,
//! Kraeutermischung, Jaegergewuerz. Traegt NUR die Essaktion.
class ChefZ_SpiceBase extends Edible_Base
{
    /**
     * ActionEatSmall + ActionForceFeedSmall, das Vanilla-Paar fuer die kleinste
     * Portion. Vorbild ist PackagedFood.c - Honey, Zagorky_ColorBase und
     * Snack_ColorBase registrieren genau diese beiden und keine anderen.
     *
     * Die kleine Variante und nicht ActionEat oder ActionEatBig, weil
     * UAQuantityConsumed.EAT_SMALL (10) gegen EAT_NORMAL (15) und EAT_BIG (25)
     * die Bissgroesse ist: eine Tuete Pfeffer wird geloeffelt, nicht
     * ausgetrunken. Dass die Klassen varQuantityMax = 1 tragen und damit
     * ohnehin in einem Zug leer sind, macht die Wahl nicht beliebig - sie ist
     * die Aussage darueber, was das Item ist, und sie bleibt richtig, wenn
     * jemand die Menge spaeter hochsetzt.
     *
     * Bewusst KEIN ActionForceFeed (gross): Vanilla paart immer beide Haelften
     * derselben Groesse. Gefuettert wird, was auch selbst gegessen werden
     * kann, und in derselben Portion.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeedSmall);
        AddAction(ActionEatSmall);
    }
}

//! Basis der getrockneten Kraeuter (§16, §9): Petersilie, Dill, Thymian,
//! Rosmarin, Baerlauch und getrocknete Paprikaschoten. Traegt NUR die
//! Essaktion; Begruendung der Variante siehe ChefZ_SpiceBase.
class ChefZ_DriedHerbBase extends Edible_Base
{
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeedSmall);
        AddAction(ActionEatSmall);
    }
}
