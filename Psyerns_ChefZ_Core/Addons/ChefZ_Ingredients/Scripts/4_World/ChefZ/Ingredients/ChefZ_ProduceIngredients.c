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

//! §13: Potato + Knife.
class ChefZ_SlicedPotato extends ChefZ_Edible_Base {}

//! §14: Tomato + Knife.
class ChefZ_ChoppedTomato extends ChefZ_Edible_Base {}

//! §15: ChefZ_Paprika (Slice herbs) oder Vanillas GreenBellPepper + Knife.
class ChefZ_ChoppedPaprika extends ChefZ_Edible_Base {}

//! §17: ChefZ_Onion + Knife.
class ChefZ_ChoppedOnion extends ChefZ_Edible_Base {}

//! §18: ChefZ_Garlic + Knife.
class ChefZ_ChoppedGarlic extends ChefZ_Edible_Base {}

//! §19: ChefZ_Carrot + Knife.
class ChefZ_ChoppedCarrot extends ChefZ_Edible_Base {}

//! §20: ChefZ_Cabbage + Knife.
class ChefZ_ChoppedCabbage extends ChefZ_Edible_Base {}
