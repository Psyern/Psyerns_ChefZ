//==============================================================================
// ChefZ_SauceItemBase - Skriptbasis der Bruehen und Saucen.
//
// Slice "sauces". Production Map §52-§55, DME-Plan §34.
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Marmalade { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// Genau EINE Skriptklasse fuer alle vier Items, und das ist kein Sparzwang:
// DayZ sucht zu einer Configklasse die gleichnamige Skriptklasse und geht,
// wenn es keine gibt, die CONFIG-Elternkette hinauf. Jede Configklasse dieses
// Slice erbt von ChefZ_SauceItemBase - also findet die Engine fuer jede von
// ihnen diese Klasse hier.
//
// Sie ist leer, und auch das ist Absicht. Was eine Sauce IST - Kategorien,
// Tags, Zustand, Naehrwert - steht in Daten (CfgChefZIngredients in der
// config.cpp und Config/Recipes/Sauces.json), nicht in Code. Jede Zeile hier
// waere Content, der sich nicht mehr ueber eine Datei aendern liesse.
//
// Ohne diese Ableitung traegt keine der vier Klassen einen ChefZ-Zustand; sie
// waeren gewoehnliche Vanilla-Nahrung (ChefZ_Edible_Base.c, Kopf). Fuer die
// Saucen waere das besonders teuer: sie sind Eingang der Gerichte der zweiten
// Welle, und deren Ergebnis schreibt Frische und Zustand fort.
//
// KEIN CanBeCooked()-Override: ob eine Sauce im Topf weitergart, entscheidet
// Vanilla anhand des FoodStages-Knotens der config.cpp - dort, wo auch die
// Uebergaenge stehen. Zwei Quellen fuer dieselbe Aussage waeren zwei
// Gelegenheiten, sie unterschiedlich falsch zu beantworten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_SauceItemBase extends ChefZ_Edible_Base
{
}
