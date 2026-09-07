//==============================================================================
// ChefZ_ServingItems - die Skriptseite des Servierens.
//
// Slice "serving". Entwuerfe 15 (Portion System) und 16 (Container System),
// Entscheidung OF-04 (Behaelter beim Servieren, wiederverwendbar).
//
// Andockregel aus dem Kopf von ChefZ_Container_Base.c und
// ChefZ_PortionedFood_Base.c: die CONFIGklasse erbt von einer Vanilla-Klasse,
// die SKRIPTklasse von der ChefZ-Basis. Der Core bringt fuer keine der beiden
// einen CfgVehicles-Eintrag mit (Invariante I3) - deshalb steht die
// Configseite in der config.cpp dieses Moduls und die Skriptseite hier.
//
// KEINE Aktion, KEIN Override, KEIN modded class. Alles, was diese Klassen
// koennen, kommt aus dem Core:
//
//   ChefZ_ActionTakePortion    die eine Entnahmeaktion fuer ALLE portionierten
//                              Gerichte (15 E5)
//   ChefZ_PortionedFood_Base   Zaehler, Persistenz, Sync, Tooltip "3 / 8"
//   ChefZ_ContainerService     Suche Haende -> Inventar, Verbrauch, Rueckgabe
//   ChefZ_Edible_Base.OnConsume  gibt den leeren Behaelter zurueck, sobald die
//                              Menge auf 0 faellt (16 §5, E4)
//
// Wenn hier je eine Zeile Logik entsteht, ist das ein Hinweis darauf, dass
// etwas Generisches im Content gelandet ist.
//
// Layer: 4_World.
//==============================================================================

//------------------------------------------------------------------------------
// 1. Die Behaelter (Production Map §60, Architekturplan §18)
//
// ChefZ_Container_Base leitet von ItemBase ab, nicht von ChefZ_Item_Base
// (16 §3.2): ein leerer Teller braucht weder Zustand noch Qualitaet noch
// Frische, und jedes dieser Felder waere Sync- und Spielstandslast auf einem
// Item, dessen ganzer Zweck es ist, leer zu sein.
//
// ChefZ_IsEmpty() wird bewusst NICHT ueberschrieben: die allgemeine Antwort
// des ChefZ_ContainerService ("kein Cargo und keine Menge") trifft auf alle
// fuenf zu. Keiner von ihnen hat Cargo, keiner traegt eine Fluessigkeit.
//------------------------------------------------------------------------------

//! PLATE - Traeger der Tellergerichte (§61). reusable, siehe CfgChefZContainers.
class ChefZ_EmptyPlate extends ChefZ_Container_Base {}

//! BOWL - Traeger der Suppen, Eintoepfe und aller Portionsgerichte (§62, §17).
class ChefZ_EmptyBowl extends ChefZ_Container_Base {}

//! CAN - der Konservenfall: reusable = 0, es kommt nichts zurueck (OF-04).
class ChefZ_EmptyCan extends ChefZ_Container_Base {}

//! JAR - Eingemachtes haelt laenger; der Faktor steht am Behaelter (16 E7).
class ChefZ_EmptyJar extends ChefZ_Container_Base {}

//! BOX - Trockenware, DME-Plan §32.
class ChefZ_EmptyBox extends ChefZ_Container_Base {}

//------------------------------------------------------------------------------
// 2. Die beiden Skriptbasen der Gerichte
//
// Sie sind der Anschluss fuer die Slices dishes-a/b/c und sauces. Ein Gericht
// schreibt genau zwei Zeilen:
//
//     config.cpp   class ChefZ_HunterStewBulk : ChefZ_PortionedDish_Base { ... };
//     script       class ChefZ_HunterStewBulk extends ChefZ_PortionedDish_Base {}
//
// und ist damit vollstaendig angeschlossen. WAS bei einer Entnahme entsteht,
// steht im REZEPT (outputs[].portionClass) - nicht in der Klasse und nirgends
// im Core (15 §3).
//
// Beide Configklassen haben scope = 0 und sind damit keine Items. Sie stehen
// hier, damit nicht jedes Gerichtemodul dieselbe Ableitung neu erfindet - und
// damit die Garstufenuebergaenge des Bulk (01 V4, sonst verbrennt es) an genau
// EINER Stelle stehen.
//------------------------------------------------------------------------------

//! Das Bulk-Gericht im Kochgefaess: Zaehler, Entnahmeaktion, "3 / 8" (15 §2).
class ChefZ_PortionedDish_Base extends ChefZ_PortionedFood_Base {}

//! Die servierte Portion auf dem Teller oder in der Schuessel. Traegt beim
//! vollstaendigen Verzehr den leeren Behaelter zurueck - das leistet
//! ChefZ_Edible_Base.OnConsume anhand von m_ChefZ_ReturnContainer (16 §3.2).
class ChefZ_ServedDish_Base extends ChefZ_Edible_Base
{
    /**
     * Die Essaktion aller 25 Teller- und Schuesselgerichte, an genau einer
     * Stelle.
     *
     * Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (Rice.c:3-10). Ohne sie bietet das Spiel den
     * fertigen Teller nicht zum Essen an - kein Fehlerbild, keine Logzeile,
     * die Aktion fehlt einfach. Sie steht hier und nicht an 25 Klassen, weil
     * die Engine zu einer Configklasse ohne eigene Skriptklasse die
     * Config-Elternkette hinaufgeht und jedes Gericht ueber
     * ChefZ_ServedDish_Base laeuft.
     *
     * ActionEatBig ist die Variante, die Vanilla fuer grosse Mahlzeiten
     * nimmt: Rice.c, Marmalade.c, PowderedMilk.c und Guts.c registrieren sie,
     * und sie verbraucht UAQuantityConsumed.EAT_BIG (25) statt EAT_NORMAL
     * (15). Ein Teller Eintopf isst sich nicht wie eine Beere.
     *
     * KEINE Gattungszusage (IsMeat/IsFruit): ein zubereitetes Gericht ist
     * keins von beidem, und Vanillas zubereitete Nahrung - Rice, Marmalade,
     * die Konserven - sagt ebenfalls nichts dazu. Der Verfall laeuft damit
     * ueber ChefZ_ItemDecay und die Preservation-Daten, nicht ueber Vanillas
     * Fleisch- oder Obstuhr.
     *
     * ChefZ_PortionedDish_Base bekommt bewusst NICHTS davon: aus dem Bulk im
     * Kochgefaess wird portioniert, nicht gegessen (15 §2). Dafuer haengt
     * ChefZ_ActionTakePortion an ChefZ_PortionedFood_Base.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatBig);
    }
}
