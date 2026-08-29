//==============================================================================
// ChefZ_BowlDishItems - die Skriptseite der fuenf Bowl-Gerichte.
//
// Slice "dishes-c". Production Map §62 (Suppen und Eintoepfe), §60 (Behaelter);
// DME-Plan §41 (Qualitaetsstufen), §42 (Gerichtsnutzen), §44 (Gruppenrezepte);
// Entwuerfe 15 (Portion System) und 16 (Container System); OF-04, OF-05.
//
// Andockregel aus dem Kopf von ChefZ_PortionedFood_Base.c und aus
// ChefZ_ServingItems.c: die CONFIGklasse erbt von einer Vanilla-Klasse, die
// SKRIPTklasse von der ChefZ-Basis. Der Slice "serving" hat beide Basen bereits
// gebaut - hier wird nichts davon wiederholt:
//
//     ChefZ_PortionedDish_Base   der Kessel-/Topfinhalt mit Portionszaehler,
//                                Persistenz, Sync, Tooltip "3 / 8" und der
//                                Aktion ChefZ_ActionTakePortion (15 §3)
//     ChefZ_ServedDish_Base      die entnommene Schuessel; gibt beim
//                                vollstaendigen Verzehr ChefZ_EmptyBowl zurueck
//                                (16 §5, OF-04)
//
// KEINE Aktion, KEIN Override, KEIN modded class, KEINE Logik. Jedes Gericht
// ist genau eine Zeile. Was bei einer Entnahme entsteht, steht im REZEPT
// (outputs[].portionClass) - nicht in der Klasse und nirgends im Core (15 §3).
//
// Wenn hier je eine Zeile Logik entsteht, ist etwas Generisches im Content
// gelandet und gehoert in den Core zurueck (Workflow §10.3).
//
// WARUM ES KEINE _Premium-KLASSEN GIBT (OF-05, Entscheidung B):
// PlayerStomach.AddToStomach holt den Naehrwert ueber Klasse x Foodstage und
// kennt keine Instanzdaten (01 V6). Eine Qualitaetsstufe kann den Naehrwert je
// Bissen nicht anheben - sie wirkt ueber die AUSBEUTE. Ein PREMIUM-Kessel
// liefert mehr Portionen desselben Gerichts. Vier Stufen mal fuenf Gerichte
// waeren sonst zwanzig zusaetzliche Klassen mit Modell, Stringtable und
// Naehrwertblock gewesen.
//
// Layer: 4_World.
//==============================================================================

//------------------------------------------------------------------------------
// 1. Die Bulk-Gerichte - was im Topf oder Kessel entsteht (15 §2)
//
// Sie duerfen am Feuer stehen bleiben (CanBeCooked() == true) und ueberhitzen
// dabei auch; die dafuer noetigen Garstufenuebergaenge stehen EINMAL an
// ChefZ_PortionedDish_Base in der config.cpp (01 V4). Wer den Kessel vergisst,
// verliert ihn - das ist gewollt und ist zugleich das Verbrennungssystem, das
// OF-09 ausdruecklich NICHT nachbaut.
//------------------------------------------------------------------------------

//! §62 Hunter Stew - Wildfleisch, Wurzelgemuese, Pilze, Thymian.

//! §62 Fisherman's Stew - Fisch, Kartoffel, Karotte, Petersilie.

//! §62 Vegetable Soup - das Basic-Rezept aus DME §37, ohne Fleisch.

//! §62 Bone Broth Soup - Knochenbruehe als Traeger (§55).

//! §62 Chernarus Chili - Fleisch, Bohnen, Tomate, Paprika.

//------------------------------------------------------------------------------
// 2. Die servierten Portionen - was der Spieler in der Hand haelt (15 §2)
//
// Bewusst OHNE Food-Knoten in der config.cpp: eine servierte Schuessel liegt
// nicht mehr im Feuer. Ohne "Food FoodStages" liefert ItemBase.HasFoodStage()
// false (ItemBase.c:2654), es entsteht gar kein FoodStage-Objekt - und damit
// kann die Portion auch nicht verbrennen.
//
// Die Rueckgabe der leeren Schuessel steht am GERICHT und nicht im Rezept
// (16 §3.2): CfgChefZIngredients traegt fuer jede dieser fuenf Klassen
// containerCategory = "BOWL" und returnContainer = "ChefZ_EmptyBowl".
//------------------------------------------------------------------------------

//! Eine Schuessel Hunter Stew.
class ChefZ_HunterStewBowl extends ChefZ_ServedDish_Base {}

//! Eine Schuessel Fisherman's Stew.
class ChefZ_FishermanStewBowl extends ChefZ_ServedDish_Base {}

//! Eine Schuessel Vegetable Soup.
class ChefZ_VegetableSoupBowl extends ChefZ_ServedDish_Base {}

//! Eine Schuessel Bone Broth Soup.
class ChefZ_BoneBrothSoupBowl extends ChefZ_ServedDish_Base {}

//! Eine Schuessel Chernarus Chili.
class ChefZ_ChernarusChiliBowl extends ChefZ_ServedDish_Base {}
