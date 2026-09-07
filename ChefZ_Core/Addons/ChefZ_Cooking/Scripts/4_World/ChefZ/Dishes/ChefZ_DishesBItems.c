//==============================================================================
// ChefZ_DishesBItems - die Skriptseite der Tellergerichte 11-20.
//
// Slice "dishes-b". Production Map §61.11-§61.20, DME-Plan §38, §41, §53.
// Anschlussbeschreibung: Config/Recipes/README_Serving.md §1, woertlich.
//
// Andockregel:
//
//     config.cpp   class ChefZ_MushroomPanBulk : ChefZ_PortionedDish_Base { ... };
//     Skript       class ChefZ_MushroomPanBulk extends ChefZ_PortionedDish_Base {}
//
//     config.cpp   class ChefZ_MushroomPan : ChefZ_ServedDish_Base { ... };
//     Skript       class ChefZ_MushroomPan extends ChefZ_ServedDish_Base {}
//
// JEDE Klasse hier ist leer, und das ist die Aussage dieser Datei, nicht ihr
// Mangel. Zaehler, Entnahmeaktion, Persistenz, Sync, Tooltip "1 / 2",
// Behaelterpruefung und Behaelterrueckgabe kommen vollstaendig aus dem Core
// (ChefZ_PortionedFood_Base, ChefZ_ActionTakePortion, ChefZ_ContainerService,
// ChefZ_Edible_Base.OnConsume). Was ein Gericht IST - Zutaten, Qualitaets-
// stufen, Naehrwert, Behaelter, Portionszahl - steht in Daten:
// Config/Recipes/DishesB.json und CfgChefZIngredients in der config.cpp.
//
// Entsteht hier je eine Zeile Logik, ist das ein Hinweis darauf, dass etwas
// Generisches im Content gelandet ist (Workflow §10.3).
//
// WARUM DIE ABLEITUNGEN TROTZDEM DASTEHEN, obwohl DayZ zu einer Configklasse
// ohne gleichnamige Skriptklasse die CONFIG-Elternkette hinaufgeht und dort
// ChefZ_PortionedDish_Base bzw. ChefZ_ServedDish_Base faende:
// README_Serving.md §1 nennt beide Zeilen als den Anschluss eines Gerichts,
// und eine ausgeschriebene Ableitung ist die Stelle, an der ein spaeteres
// Signature-Gericht sein eigenes Verhalten bekommen kann, ohne dass zuerst
// eine Klasse angelegt werden muss. Der Preis sind zwanzig leere Zeilen.
//
// KEIN modded class, KEIN Override, KEINE Terje-Referenz.
//
// Layer: 4_World.
//==============================================================================

//------------------------------------------------------------------------------
// 1. Die Bulk-Gerichte - das, was im Kochgeraet entsteht (15 §2)
//------------------------------------------------------------------------------

//! §61.11 Tactical Bacon Breakfast - Pfanne, gebacken.

//! §61.12 Ruehrei mit Wurst.

//! §61.13 Bauernfruehstueck - das ergiebigste Gericht dieses Slice (3 Portionen).

//! §61.14 Kaese-Fladenbrot.

//! §61.15 Wurstbrot-Teller - kalt angerichtet, completion INSTANT.

//! §61.16 Pilzpfanne.

//! §61.17 Kartoffelpuffer.

//! §61.18 Fleisch-Teigtaschen - das einzige Gericht dieses Slice, das auch
//! gekocht statt gebraten fertig wird (doneStages Boiled UND Baked).

//! §61.19 Milchreis - Topf, gekocht, Schuessel.

//! §61.20 Honigbrot-Platte - kalt angerichtet, completion INSTANT.

//------------------------------------------------------------------------------
// 2. Die servierten Portionen - das, was der Spieler isst (15 §2, 16 §5)
//
// Sie tragen den Namen aus Production Map §72 und DME-Plan §53. Beim letzten
// Bissen geben sie ueber ChefZ_Edible_Base.OnConsume den leeren Behaelter
// zurueck - Teller oder, beim Milchreis, Schuessel (OF-04: reusable = 1).
//------------------------------------------------------------------------------

class ChefZ_TacticalBreakfast   extends ChefZ_ServedDish_Base {}
class ChefZ_ScrambledEggSausage extends ChefZ_ServedDish_Base {}
class ChefZ_FarmersBreakfast    extends ChefZ_ServedDish_Base {}
class ChefZ_CheeseFlatbread     extends ChefZ_ServedDish_Base {}
class ChefZ_SausageBreadPlate   extends ChefZ_ServedDish_Base {}
class ChefZ_MushroomPan         extends ChefZ_ServedDish_Base {}
class ChefZ_PotatoPancakes      extends ChefZ_ServedDish_Base {}
class ChefZ_MeatDumplings       extends ChefZ_ServedDish_Base {}
class ChefZ_MilkRice            extends ChefZ_ServedDish_Base {}
class ChefZ_HoneyBreadPlate     extends ChefZ_ServedDish_Base {}
