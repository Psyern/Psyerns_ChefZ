//==============================================================================
// ChefZ_BakingItems - Hefe, Teige, Pasta, Brot.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse (hier ueber ChefZ_GrainFoodBase aus ChefZ_Farming),
// die SKRIPTklasse von der ChefZ-Basis. Der Core bringt fuer keine von beiden
// einen CfgVehicles-Eintrag mit (Invariante I3).
//
// Jede Klasse hier ist eine leere Ableitung, und das ist Absicht: was woraus
// wird, steht in Config/GrainTransforms.json und Config/GrainRecipes.json. Ein neues
// Gericht darf niemals eine Code-Aenderung noetig machen (Workflow §10.3).
//
// Layer: 4_World.
//==============================================================================

//! Hefe. V1 ausschliesslich Loot (Production Map §9).
class ChefZ_Yeast extends ChefZ_GrainFoodBase {}

//! Einfacher Teig: Mehl + Wasser. Zutat des Fladenbrots.
class ChefZ_SimpleDough extends ChefZ_GrainFoodBase {}

//! Hefeteig: einfacher Teig + Hefe. Zutat des Brots.
class ChefZ_YeastDough extends ChefZ_GrainFoodBase {}

//! Nudelteig: ausgerollter einfacher Teig.
class ChefZ_PastaDough extends ChefZ_GrainFoodBase {}

//! Frische Nudeln. Kurze Haltbarkeit, direkt kochbar.
class ChefZ_RawPasta extends ChefZ_GrainFoodBase {}

//! Trockennudeln. Der Vorratsartikel der Kette.
class ChefZ_DriedPasta extends ChefZ_GrainFoodBase {}

//! Brot aus Hefeteig.
class ChefZ_Bread extends ChefZ_GrainFoodBase {}

//! Fladenbrot aus einfachem Teig - der Weg ohne Hefe.
class ChefZ_Flatbread extends ChefZ_GrainFoodBase {}
