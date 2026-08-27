//==============================================================================
// ChefZ_ProduceIngredients - die Skriptklassen der geschnittenen Gemuesestufen.
//
// Slice "produce". Production Map §13, §14, §15, §17-§20.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse (Edible_Base), die SKRIPTklasse von ChefZ_Edible_Base.
// Der Core bringt fuer keine der beiden einen CfgVehicles-Eintrag mit
// (Invariante I3) - deshalb steht die Configseite in der config.cpp dieses
// Moduls und die Skriptseite hier.
//
// Ohne diese Ableitung traegt das Item KEINEN ChefZ-Zustand: es waere ein
// gewoehnliches Vanilla-Nahrungsmittel. Geschnittenes Gemuese ist aber Eingang
// spaeterer Rezepte, deren Ergebnis Frische und Zustand fortschreibt - der
// Zustand muss also mitlaufen.
//
// Kein modded class, kein Override, keine eigene Aktion: alles, was diese
// Klassen koennen, kommt aus ChefZ_Edible_Base (06 §2).
//
// Layer: 4_World.
//==============================================================================

//! Die Skriptbasis der ganzen Familie - dieselbe Bauform wie
//! ChefZ_MeatItemBase und ChefZ_GrainFoodBase.
//!
//! Sie ist gleichnamig zur CONFIGbasis ChefZ_ChoppedVegetableBase (scope = 0),
//! und das ist der Punkt: die Configbasis ist die Klasse, die
//! Food > FoodStages UND Food > FoodStageTransitions traegt. Ohne eine
//! Skriptklasse an genau diesem Namen endete die Skriptkette der Familie bei
//! Vanillas Edible_Base, und Edible_Base.CanBeCooked() liefert false
//! (Edible_Base.c:129). Cooking.ProcessItemToCook (Cooking.c:47) ginge dann an
//! dem Item vorbei, die Garstufe bliebe auf Raw stehen und jedes
//! ON_STAGE-Rezept mit einer dieser Klassen in einem Pflicht-Slot
//! (RCP_ChefZ_FarmersBreakfast, RCP_ChefZ_ChernarusChili) wuerde nie fertig.
//!
//! ChefZ_Edible_Base.CanBeCooked() rechnet die Antwort aus den Daten der
//! Klasse aus. Fuer diese Familie lautet sie fuer JEDE Erbin "ja", und das ist
//! richtig: alle sieben sind Schnittgut aus dem Topf oder der Pfanne, keine
//! davon wird ausschliesslich roh verzehrt. Eine Erbin ohne Uebergaenge gibt es
//! nicht - der Knoten steht auf der gemeinsamen Basis, keine Configklasse kann
//! ihn wieder entfernen.
class ChefZ_ChoppedVegetableBase extends ChefZ_Edible_Base {}

//! §13: Potato + Knife.
class ChefZ_SlicedPotato extends ChefZ_ChoppedVegetableBase {}

//! §14: Tomato + Knife.
class ChefZ_ChoppedTomato extends ChefZ_ChoppedVegetableBase {}

//! §15: ChefZ_Paprika (Slice herbs) oder Vanillas GreenBellPepper + Knife.
class ChefZ_ChoppedPaprika extends ChefZ_ChoppedVegetableBase {}

//! §17: ChefZ_Onion + Knife.
class ChefZ_ChoppedOnion extends ChefZ_ChoppedVegetableBase {}

//! §18: ChefZ_Garlic + Knife.
class ChefZ_ChoppedGarlic extends ChefZ_ChoppedVegetableBase {}

//! §19: ChefZ_Carrot + Knife.
class ChefZ_ChoppedCarrot extends ChefZ_ChoppedVegetableBase {}

//! §20: ChefZ_Cabbage + Knife.
class ChefZ_ChoppedCabbage extends ChefZ_ChoppedVegetableBase {}
