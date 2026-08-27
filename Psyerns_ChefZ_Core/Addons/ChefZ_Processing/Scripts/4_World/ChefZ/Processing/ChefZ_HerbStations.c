//==============================================================================
// Die beiden Stationen der Kraeuter- und Gewuerzkette (Slice "herbs").
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/Processing/
// ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_Mortar : <eine Vanilla-Klasse> { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Mortar", ... }] }
//     Skript       class ChefZ_Mortar extends ChefZ_ProcessingStation_Base {}
//
// "Mehr ist nicht noetig. Kein Core-Code, keine eigene Action, keine eigene
// Persistenz." Genau deshalb steht hier nichts ausser den beiden Bindungen -
// jede Zeile mehr waere Logik, die es im Core schon gibt.
//
// Der Trockenrahmen laeuft ueber PROCESS_DRY (STATION_TIMED): er tickt ohne
// Spieler weiter. Der Moerser laeuft ueber PROCESS_GRIND_SPICE und
// PROCESS_GRIND_HERB (STATION_ACTION): der Spieler arbeitet aktiv daran.
// Welche Station welchen Prozess anbietet, steht in
// Config/Processing/HerbStations.json - nicht hier.
//
// Beide Stationen fassen Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

//! Moerser und Stoessel (DME-Plan §6.3, Production Map §57).
class ChefZ_Mortar extends ChefZ_ProcessingStation_Base {}

//! Trockenrahmen (DME-Plan §6.7, Production Map §57).
class ChefZ_DryingRack extends ChefZ_ProcessingStation_Base {}
