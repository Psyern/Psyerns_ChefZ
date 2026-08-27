//==============================================================================
// ChefZ_FarmingItems - die Skriptklassen der Weizenkette.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse, die SKRIPTklasse von der ChefZ-Basis. Der Core bringt
// fuer keine der beiden einen CfgVehicles-Eintrag mit (Invariante I3).
//
// ChefZ_WheatPlant und ChefZ_WheatSeeds erben dagegen von den VANILLA-Basen:
// sie tragen keinen ChefZ-Zustand, sie sind Gartenobjekte. PlantBase und
// SeedBase bringen alles mit, was Anpflanzen und Ernten brauchen - eine
// ChefZ-Ableitung waere dort Gewicht ohne Gegenwert.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der essbaren Getreidewaren. Sie traegt den
//! ChefZ-Zustand; die Nahrungsdaten stehen in der config.cpp und die Zahlen in
//! der ChefZ-Nutrition-Registry.
class ChefZ_GrainFoodBase extends ChefZ_Edible_Base {}

//! Die Weizenpflanze im Beet. Wachstum, Krankheit und Ernte kommen
//! vollstaendig von PlantBase; die Zahlen stehen im Horticulture-Knoten.
class ChefZ_WheatPlant extends PlantBase {}

//! Weizensaatgut. ActionPlantSeed und ActionAttachSeeds haengen an SeedBase.
class ChefZ_WheatSeeds extends SeedBase {}

//! Das geerntete Korn und Eingang der Getreidemuehle.
class ChefZ_Wheat extends ChefZ_GrainFoodBase {}
