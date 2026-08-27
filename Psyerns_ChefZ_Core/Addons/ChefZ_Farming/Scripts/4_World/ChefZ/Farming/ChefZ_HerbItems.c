//==============================================================================
// Skriptklassen der Kraeuter-Ernteprodukte.
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/State/
// ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_Parsley : Edible_Base { ... };   (Vanilla-Basis)
//     script       class ChefZ_Parsley extends ChefZ_Edible_Base { }
//
// Ohne diese Ableitung traegt das Item keinen ChefZ-Zustand - es waere ein
// gewoehnliches Vanilla-Nahrungsmittel. Kein Fehler, nur weniger.
//
// Kein modded class, kein Override, keine eigene Aktion: der gesamte
// Zustands-, Frische- und Verderbpfad liegt in ChefZ_Edible_Base. Was hier
// steht, ist ausschliesslich die Bindung.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der frischen Kraeuter. Entspricht der Configklasse
//! gleichen Namens (scope = 0, sie ist selbst kein Item).
class ChefZ_FreshHerbBase extends ChefZ_Edible_Base {}

class ChefZ_Parsley    extends ChefZ_FreshHerbBase {}
class ChefZ_Dill       extends ChefZ_FreshHerbBase {}
class ChefZ_Thyme      extends ChefZ_FreshHerbBase {}
class ChefZ_Rosemary   extends ChefZ_FreshHerbBase {}
class ChefZ_WildGarlic extends ChefZ_FreshHerbBase {}

//! Pfefferbeeren sind der Rohstoff der Pfefferkette (Production Map §16).
class ChefZ_PepperBerries extends ChefZ_FreshHerbBase {}

//! Frische Paprika ist Gemuese UND Ausgangsstoff des Paprikapulvers
//! (Production Map §15).
class ChefZ_Paprika extends ChefZ_FreshHerbBase {}
