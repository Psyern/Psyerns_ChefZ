//==============================================================================
// Die beiden Stationen der Fleischkette.
//
// Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
//
//   config.cpp   class ChefZ_MeatGrinder : <eine Vanilla-Klasse> { ... };
//   Stationsdatensatz  id == Klassenname, processes[] = { ... }
//   Skript       class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base {}
//
// Mehr ist nicht noetig: kein Core-Code, keine eigene Action, keine eigene
// Persistenz. Welche Prozesse eine Station anbietet, steht ausschliesslich im
// Stationsdatensatz (Config/Processing/Stations.json) - nicht hier. Deshalb
// sind beide Klassen leer, und das ist kein Versehen: jede Zeile hier waere
// Content, der sich nicht mehr ueber Daten aendern liesse.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CuttingBoard extends ChefZ_ProcessingStation_Base
{
}

class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base
{
}
