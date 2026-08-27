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
class ChefZ_PaprikaPowder extends ChefZ_Edible_Base {}
