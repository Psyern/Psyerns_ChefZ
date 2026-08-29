//==============================================================================
// ChefZ_FarmingItems - die Skriptklassen der Weizenkette.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse, die SKRIPTklasse von der ChefZ-Basis. Der Core bringt
// fuer keine der beiden einen CfgVehicles-Eintrag mit (Invariante I3).
//
// Weizen ist eine FUNDPFLANZE wie Vanillas Pilze (Entscheidung vom
// 29.08.2026): keine Pflanzenklasse, kein Saatgut - nur das Korn.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der essbaren Getreidewaren. Sie traegt den
//! ChefZ-Zustand; die Nahrungsdaten stehen in der config.cpp und die Zahlen in
//! der ChefZ-Nutrition-Registry.
class ChefZ_GrainFoodBase extends ChefZ_Edible_Base
{
    /**
     * Die Essaktion aller elf Getreidewaren - Weizen, Mehl, Hefe, die drei
     * Teige, roher und getrockneter Nudelteig, Brot und Fladenbrot.
     *
     * Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (Rice.c:3-10). Ohne sie wird das Item im Spiel
     * nicht zum Essen angeboten: kein Fehlerbild, keine Logzeile, die Aktion
     * fehlt einfach. Die Engine findet diese Klasse fuer jede Erbin ueber die
     * Config-Elternkette - auch fuer ChefZ_Flour, das in ChefZ_Processing
     * liegt.
     *
     * ActionEatBig ist hier die Vanilla-Antwort und nicht nur die bequeme:
     * Rice.c und PowderedMilk.c sind Vanillas trockene Schuettware im Beutel
     * und registrieren genau ActionForceFeed + ActionEatBig. ChefZ_Flour
     * benutzt sogar PowderedMilk.p3d als Proxy. Brot und Teig sind dichte,
     * grosse Portionen und passen in dieselbe Variante; die gestapelten
     * Klassen (Weizen 1000 g, Mehl 1000 g, Nudeln 500 g) brauchen sie, weil
     * EAT_SMALL bei diesen Mengen zu hundert Bissen fuehrte.
     *
     * ZU HEFE, die als "sollte gar nicht essbar sein" in Frage stand: sie
     * bleibt essbar, und die Entscheidung liegt nicht bei dieser Datei. Die
     * zentrale Nutrition-Registry (ChefZ_Registry/Config/Nutrition.json)
     * fuehrt ChefZ_Yeast mit energy 90 / water 4 / stomach 5 als Nahrung. Die
     * saubere Loesung waere, ihr die Naehrwerte zu nehmen - dazu muesste
     * dieser Slice die Registry aendern, und das ist ihm verboten (Workflow
     * §5). Ein Item mit Naehrwerten und ohne Essaktion waere die schlechtere
     * Haelfte von beidem: es saettigte auf dem Papier und liesse sich nicht
     * anruehren.
     *
     * KEINE Gattungszusage (IsFruit/IsMeat). Vanillas gemahlene und
     * verpackte Waren - Rice, PowderedMilk, BoxCerealCrunchin - sagen
     * ebenfalls nichts dazu; ihr Verfall laeuft nicht ueber die Obst- oder
     * Fleischuhr. Fuer ChefZ uebernimmt das ChefZ_ItemDecay.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatBig);
    }
}

//! Das gefundene Korn und Eingang der Getreidemuehle. Weizen ist eine
//! Fundpflanze wie Vanillas Pilze - keine Pflanze, kein Saatgut.
class ChefZ_Wheat extends ChefZ_GrainFoodBase {}
