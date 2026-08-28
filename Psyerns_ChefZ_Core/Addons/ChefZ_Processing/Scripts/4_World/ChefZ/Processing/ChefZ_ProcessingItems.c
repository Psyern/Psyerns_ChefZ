//==============================================================================
// ChefZ_ProcessingItems - Getreidemuehle, Pastamaschine, Mehl.
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

//! Die Pastamaschine (frueher ChefZ_RollingPin). Reines Werkzeug - sie traegt
//! keinen ChefZ-Zustand und wird nie verbraucht, nur ueber die Werkzeuggruppe
//! ROLLING_PIN gefunden. Die Gruppe heisst weiter so, weil PROCESS_ROLL in
//! ChefZ_Baking auf diesen Namen zeigt; die Begruendung steht an der Gruppe.
//!
//! KEINE Station und deshalb kein ChefZ_ProcessingStation_Base: die beiden
//! Transforms, die sie bedient, haben je EINEN Eingang. Die Begruendung steht
//! vollstaendig an der Configklasse.
class ChefZ_PastaMachine extends ItemBase {}

//! Mehl. Ergebnis des Mahlens und Eingang jedes Teigs.
class ChefZ_Flour extends ChefZ_GrainFoodBase {}
