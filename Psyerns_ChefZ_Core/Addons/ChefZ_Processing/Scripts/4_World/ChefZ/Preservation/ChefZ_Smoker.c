//==============================================================================
// ChefZ_Smoker - der Raeucherschrank (Slice "preservation").
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/Processing/
// ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_Smoker : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Smoker", ... }] }
//     Skript       class ChefZ_Smoker extends ChefZ_ProcessingStation_Base {}
//
// "Mehr ist nicht noetig. Kein Core-Code, keine eigene Action, keine eigene
// Persistenz." Genau deshalb steht hier nichts ausser der Bindung.
//
// Der Schrank laeuft ueber PROCESS_SMOKE (STATION_TIMED): er tickt ohne
// Spieler weiter und verlangt Waerme. Was er raeuchert, steht in
// ChefZ_Preservation/Config/Processing/Smoking.json - nicht hier und nicht in
// der config.cpp.
//
// Der Trockenrahmen bekommt KEINE zweite Skriptklasse: er hat bereits eine in
// ChefZ_HerbStations.c. Er ist dieselbe Station und bekommt von diesem Slice
// nur neue Transforms.
//
// Diese Station fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Smoker extends ChefZ_ProcessingStation_Base {}
