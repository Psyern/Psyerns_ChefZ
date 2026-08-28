//==============================================================================
// ChefZ_ProduceFarming - die Skriptklassen der Gemuesekette.
//
// Slice "produce". Production Map §17 (Zwiebel), §18 (Knoblauch),
// §19 (Karotte), §20 (Kohl).
//
// Kartoffel, Tomate und Paprika kommen hier NICHT vor: Plant_Potato,
// Plant_Tomato, Plant_Pepper, PotatoSeed, TomatoSeeds, PepperSeeds und
// PepperSeedsPack sind Vanilla und werden erweitert, nicht nachgebaut
// (Workflow §10.5).
//
// Die Paprikakette ist seit dem Vanilla-Audit (§2) VOLLSTAENDIG Vanilla:
// PepperSeedsPack -> PepperSeeds -> Plant_Pepper -> GreenBellPepper ->
// CutOutPepperSeeds -> PepperSeeds. ChefZ haengt sich an EINER Stelle ein, an
// der Frucht: der Zutaten-Datensatz von GreenBellPepper in
// ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json. Von dort laufen
// TR_ChopBellPepper (-> ChefZ_ChoppedPaprika) und TR_PaprikaToDried
// (-> ChefZ_DriedPaprika -> ChefZ_PaprikaPowder). Es fehlt KEIN Glied: das
// Zurueckgewinnen von Saatgut erledigt Vanillas CutOutPepperSeeds, deshalb
// braucht ProduceSeeds.json keinen fuenften Samen-Transform.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse, die SKRIPTklasse von der ChefZ-Basis.
//
// Pflanzen und Samen erben dagegen von den VANILLA-Basen: sie tragen keinen
// ChefZ-Zustand, sie sind Gartenobjekte. PlantBase bringt Wachstum, Krankheit
// und Ernte mit, SeedBase bringt ActionPlantSeed und ActionAttachSeeds - eine
// ChefZ-Ableitung waere dort Gewicht ohne Gegenwert.
//
// Layer: 4_World.
//==============================================================================

//--- Pflanzen im Beet. Die Zahlen stehen im Horticulture-Knoten der config.cpp.
class ChefZ_OnionPlant extends PlantBase {}
class ChefZ_GarlicPlant extends PlantBase {}
class ChefZ_CarrotPlant extends PlantBase {}
class ChefZ_CabbagePlant extends PlantBase {}

//--- Saatgut. PlantType steht im Horticulture-Knoten der config.cpp.
class ChefZ_OnionSeeds extends SeedBase {}
class ChefZ_GarlicSeeds extends SeedBase {}
class ChefZ_CarrotSeeds extends SeedBase {}
class ChefZ_CabbageSeeds extends SeedBase {}

//--- Die gemeinsame Skriptbasis der Ernte, gleichnamig zur CONFIGbasis
//--- ChefZ_VegetableFood_Base (scope = 0). Dieselbe Bauform wie
//--- ChefZ_GrainFoodBase eine Datei weiter und wie ChefZ_MeatItemBase.
//---
//--- Der Name ist der Punkt: die Configbasis traegt Food > FoodStages UND
//--- Food > FoodStageTransitions fuer alle vier Gemuese. Ohne eine
//--- Skriptklasse an genau diesem Namen endete die Skriptkette der Familie bei
//--- Vanillas Edible_Base, dessen CanBeCooked() false liefert
//--- (Edible_Base.c:129). Cooking.ProcessItemToCook (Cooking.c:47) ginge an
//--- dem Item vorbei, die Garstufe bliebe auf Raw stehen, und
//--- RCP_ChefZ_FarmersBreakfast (Pflicht-Slot ChefZ_Onion) sowie die
//--- Fischeintoepfe (Pflicht-Slot ChefZ_Carrot) wuerden nie fertig.
//---
//--- ChefZ_Edible_Base.CanBeCooked() rechnet die Antwort aus den Daten aus.
//--- Fuer alle vier Erbinnen lautet sie "ja", und das ist richtig: Zwiebel,
//--- Knoblauch, Karotte und Kohl gehen alle vier gegart in Gerichte ein. Keine
//--- von ihnen ist eine reine Rohzutat, und eine Erbin OHNE Uebergaenge gibt
//--- es nicht - der Knoten steht auf der gemeinsamen Configbasis und laesst
//--- sich in einer Configklasse nicht wieder entfernen.
class ChefZ_VegetableFood_Base extends ChefZ_Edible_Base
{
    /**
     * Zwiebel, Knoblauch, Karotte und Kohl sind fuer Vanilla "Fruit". Potato.c
     * und Tomato.c sagen dasselbe ueber Kartoffel und Tomate; die Schublade
     * heisst so, meint aber Obst UND Gemuese (Edible_Base.c:755, Kommentar
     * "fruit, vegetables and mushrooms").
     *
     * Die Zusage ist hier PFLICHT und nicht Zierde: ActionEatFruit.
     * ActionCondition prueft "food_item.IsFruit()" und liefert sonst false -
     * die Aktion waere registriert und erschiene trotzdem nie. Genau die Sorte
     * lautloser Fehler, um die es hier geht.
     *
     * Zweitwirkung, ebenfalls gewollt: Edible_Base.ProcessDecay nimmt damit
     * den Obstzweig mit DECAY_FOOD_RAW_FRVG / BOILED_FRVG / BAKED_FRVG und dem
     * Ausgang "trocknet statt zu verrotten" - statt des Zweigs fuer geoeffnete
     * Konserven, in dem jedes ChefZ-Gemuese bisher lag.
     */
    override bool IsFruit()
    {
        return true;
    }

    /**
     * Die Essaktion der vier Ernteklassen.
     *
     * ActionEatFruit ist Vanillas Variante fuer die ganze Knolle: Potato.c
     * :26-31 registriert ActionForceFeed + ActionEatFruit, Tomato.c und
     * Zucchini.c ebenso. Sie verbraucht EAT_NORMAL und setzt zusaetzlich
     * CMD_ACTIONMOD_EAT / CMD_ACTIONFB_EAT ausdruecklich - dieselbe
     * Essanimation, aber ueber die Fruchtkondition abgesichert.
     *
     * Anders als bei den frischen Kraeutern ist hier die FRUCHTvariante
     * richtig: eine ganze Zwiebel ist ein Gegenstand in der Hand, kein
     * Buendel.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatFruit);
    }
}

//--- Die Ernte. Sie traegt einen ChefZ-Zustand, weil sie Eingang der
//--- Schnittstufe ist und der Transform Frische und Zustand fortschreibt.
class ChefZ_Onion extends ChefZ_VegetableFood_Base {}
class ChefZ_Garlic extends ChefZ_VegetableFood_Base {}
class ChefZ_Carrot extends ChefZ_VegetableFood_Base {}
class ChefZ_Cabbage extends ChefZ_VegetableFood_Base {}
