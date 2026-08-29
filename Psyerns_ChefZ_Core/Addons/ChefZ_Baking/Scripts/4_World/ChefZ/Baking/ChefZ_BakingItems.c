//==============================================================================
// ChefZ_BakingItems - Teig, Pasta, Brot.
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

//! DER Teig: Mehl + Wasser. Brot, Fladenbrot, Nudeln und Teigtaschen kommen
//! alle aus ihm (Entscheidung vom 29.08.2026: eine Teigart, keine Hefe).
class ChefZ_Dough extends ChefZ_GrainFoodBase {}

//! Frische Nudeln. Kurze Haltbarkeit, direkt kochbar.
class ChefZ_RawPasta extends ChefZ_GrainFoodBase {}

//! Trockennudeln. Der Vorratsartikel der Kette.
class ChefZ_DriedPasta extends ChefZ_GrainFoodBase {}

//! Brot: der Teig im Topf oder Ofen.
class ChefZ_Bread extends ChefZ_GrainFoodBase {}

//! Fladenbrot: derselbe Teig in der Pfanne.
class ChefZ_Flatbread extends ChefZ_GrainFoodBase {}
