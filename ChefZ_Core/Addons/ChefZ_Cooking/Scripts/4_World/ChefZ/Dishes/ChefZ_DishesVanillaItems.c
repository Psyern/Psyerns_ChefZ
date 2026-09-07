//==============================================================================
// ChefZ_DishesVanillaItems - die Skriptseite der drei Gerichte aus den bisher
// ungenutzten Vanilla-Assets.
//
// Slice "dishes-vanilla". Quelle: Vanilla-Audit §3.
// Anschlussbeschreibung: Config/Recipes/README_Serving.md §1, woertlich.
//
// Andockregel:
//
//     config.cpp   class ChefZ_PumpkinSoupBulk : ChefZ_PortionedDish_Base { ... };
//     Skript       class ChefZ_PumpkinSoupBulk extends ChefZ_PortionedDish_Base {}
//
// JEDE Klasse hier ist leer, und das ist die Aussage dieser Datei, nicht ihr
// Mangel - dieselbe Begruendung wie in ChefZ_DishesBItems.c: Zaehler,
// Entnahmeaktion, Persistenz, Sync, Behaelterpruefung und Behaelterrueckgabe
// kommen vollstaendig aus dem Core. Was ein Gericht IST, steht in Daten
// (Config/Recipes/DishesVanilla.json und CfgChefZIngredients).
//
// KEIN modded class, KEIN Override, KEINE Terje-Referenz.
//
// Layer: 4_World.
//==============================================================================

//------------------------------------------------------------------------------
// 1. Die Bulk-Gerichte - das, was im Kochgeraet entsteht (15 §2)
//------------------------------------------------------------------------------

//! Kuerbissuppe aus Vanillas SlicedPumpkin - Topf, gekocht, Schuessel.

//! Kleinfischpfanne aus Sardines und Bitterlings - Pfanne, gebacken, Teller.

//! Obstkompott - Topf, gekocht, Schuessel. Das erste suesse Gericht des Mods.

//------------------------------------------------------------------------------
// 2. Die servierten Portionen - das, was der Spieler isst (15 §2, 16 §5)
//
// Beim letzten Bissen geben sie ueber ChefZ_Edible_Base.OnConsume den leeren
// Behaelter zurueck - Schuessel bei Suppe und Kompott, Teller bei der Pfanne.
//------------------------------------------------------------------------------

class ChefZ_PumpkinSoupBowl  extends ChefZ_ServedDish_Base {}
class ChefZ_SmallFishPan     extends ChefZ_ServedDish_Base {}
class ChefZ_FruitCompoteBowl extends ChefZ_ServedDish_Base {}
