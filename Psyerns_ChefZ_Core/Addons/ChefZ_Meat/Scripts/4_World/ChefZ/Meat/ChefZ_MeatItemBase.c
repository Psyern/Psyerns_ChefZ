//==============================================================================
// ChefZ_MeatItemBase - Skriptbasis aller essbaren Klassen dieses Moduls.
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Edible_Base { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// Genau EINE Skriptklasse fuer alle 21 Items, und das ist kein Sparzwang:
// DayZ sucht zu einer Configklasse die gleichnamige Skriptklasse und geht,
// wenn es keine gibt, die CONFIG-Elternkette hinauf. Jede Configklasse dieses
// Moduls erbt von ChefZ_MeatItemBase - also findet die Engine fuer jede von
// ihnen diese Klasse hier. So macht es Vanilla mit seinen Steaks auch.
//
// Sie ist leer, und auch das ist Absicht. Was eine Wurst IST - Kategorien,
// Tags, Zustand, Naehrwert - steht in Daten (Config/Ingredients/Meat.json und
// der config.cpp), nicht in Code. Jede Zeile hier waere Content, der sich
// nicht mehr ueber eine Datei aendern liesse.
//
// Ohne diese Ableitung traegt kein Item des Moduls einen ChefZ-Zustand; es
// waere dann gewoehnliche Vanilla-Nahrung - kein Fehler, nur weniger
// (ChefZ_Edible_Base.c, Kopf).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_MeatItemBase extends ChefZ_Edible_Base
{
}
