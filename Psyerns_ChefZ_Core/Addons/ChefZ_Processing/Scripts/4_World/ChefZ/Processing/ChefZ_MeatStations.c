//==============================================================================
// Die Station der Fleischkette - und das Schneidebrett, das keine mehr ist.
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
// sind die Klassen leer, und das ist kein Versehen: jede Zeile hier waere
// Content, der sich nicht mehr ueber Daten aendern liesse.
//
// In dieser Datei steht nur noch EINE Station. Das Schneidebrett hat seinen
// Stationsdatensatz verloren, weil PROCESS_CLEAN_CASING auf HANDCRAFT
// umgestellt wurde; es bleibt als Klasse bestehen und steht deshalb weiter
// hier, direkt neben der Begruendung.
//
// Layer: 4_World.
//==============================================================================

//! Das Schneidebrett. Es erbt AUSDRUECKLICH NICHT mehr von
//! ChefZ_ProcessingStation_Base: PROCESS_CLEAN_CASING ist HANDCRAFT geworden,
//! das Brett hat keinen Stationsdatensatz mehr und koennte als Station auch
//! nichts ausrichten - die Configklasse deklariert keinen Cargo-Bereich, aus
//! dem ChefZ_FactCollector.CollectFromCargo Zutaten lesen koennte. Genau das
//! war der stille Ausfall der Wurstkette.
//!
//! Die Klasse bleibt als platzierbares Ausstattungsstueck bestehen; die
//! vollstaendige Begruendung steht an der Configklasse.
class ChefZ_CuttingBoard extends ItemBase
{
}

class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base
{
}
