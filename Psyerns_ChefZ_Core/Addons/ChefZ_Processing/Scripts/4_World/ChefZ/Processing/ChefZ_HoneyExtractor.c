//==============================================================================
// ChefZ_HoneyExtractor - die Honigschleuder (Slice "apiary").
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// Processing/ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_HoneyExtractor : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_HoneyExtractor" }] }
//     Skript       class ChefZ_HoneyExtractor extends ChefZ_ProcessingStation_Base {}
//
// "Mehr ist nicht noetig. Kein Core-Code, keine eigene Action, keine eigene
// Persistenz." Genau deshalb steht hier nichts ausser der Bindung.
//
// Die Schleuder laeuft ueber PROCESS_SPIN_HONEY (STATION_ACTION): der Spieler
// dreht die Kurbel. WAS aus WAS wird, steht in Config/Processing/Honey.json -
// nicht hier und nicht in der config.cpp.
//
// KEIN ChefZ_HasHeat: Schleudern ist Mechanik, kein Feuer. Kein Prozess dieser
// Station setzt requiresHeat, die Basisantwort "nein" bleibt richtig.
//
// Die uebrigen Klassen der Imkerei liegen in ChefZ_Farming; die Begruendung
// fuer die Aufteilung steht an der Configklasse.
//
// Diese Station fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HoneyExtractor extends ChefZ_ProcessingStation_Base {}
