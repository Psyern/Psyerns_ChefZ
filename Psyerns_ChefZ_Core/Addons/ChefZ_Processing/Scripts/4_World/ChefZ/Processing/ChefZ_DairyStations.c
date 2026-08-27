//==============================================================================
// Die beiden Stationen der Milchkette (Slice "dairy").
//
// Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
//
//   config.cpp        class ChefZ_ButterChurn : <eine Vanilla-Klasse> { ... };
//   Stationsdatensatz id == Klassenname, processes[] = { ... }
//   Skript            class ChefZ_ButterChurn extends ChefZ_ProcessingStation_Base {}
//
// Beide Klassen sind leer, und das ist kein Versehen. Welche Prozesse eine
// Station anbietet, steht ausschliesslich im Stationsdatensatz
// (Config/Processing/Dairy_Stations.json); was aus welchem Eingang wird, steht
// im Transform (Config/Processing/Dairy_Transforms.json). Jede Zeile hier waere
// Content, der sich nicht mehr ueber Daten aendern liesse.
//
// Die Aktion, der Fortschritt, die Persistenz und der Abschluss kommen
// vollstaendig aus der Basis - kein Core-Code, keine eigene Action.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ButterChurn extends ChefZ_ProcessingStation_Base
{
}

class ChefZ_CheesePress extends ChefZ_ProcessingStation_Base
{
}
