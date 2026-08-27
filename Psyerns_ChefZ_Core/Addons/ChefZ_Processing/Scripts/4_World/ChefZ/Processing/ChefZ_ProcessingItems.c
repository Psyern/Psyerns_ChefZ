//==============================================================================
// ChefZ_ProcessingItems - Getreidemuehle, Nudelholz, Mehl.
//
// Andockregel aus dem Kopf von ChefZ_ProcessingStation_Base.c: die
// CONFIGklasse erbt von einer Vanilla-Klasse, die SKRIPTklasse von der
// ChefZ-Basis. Der Core bringt fuer keine von beiden einen CfgVehicles-
// Eintrag mit (Invariante I3).
//
// Was hier NICHT steht: eine eigene Action, ein eigener Tick, eine eigene
// Persistenz. Alles drei liegt in ChefZ_ProcessingStation_Base, und welche
// Prozesse die Muehle anbietet, steht in CfgChefZStations - nicht im Code.
//
// Layer: 4_World.
//==============================================================================

//! Die Getreidemuehle. Sie bietet PROCESS_MILL an; welcher Transform daran
//! zuendet, entscheidet der ChefZ_ProcessingManager aus den Daten.
class ChefZ_GrainMill extends ChefZ_ProcessingStation_Base {}

//! Das Nudelholz. Reines Werkzeug - es traegt keinen ChefZ-Zustand und wird
//! nie verbraucht, nur ueber die Werkzeuggruppe ROLLING_PIN gefunden.
class ChefZ_RollingPin extends ItemBase {}

//! Mehl. Ergebnis des Mahlens und Eingang jedes Teigs.
class ChefZ_Flour extends ChefZ_GrainFoodBase {}
