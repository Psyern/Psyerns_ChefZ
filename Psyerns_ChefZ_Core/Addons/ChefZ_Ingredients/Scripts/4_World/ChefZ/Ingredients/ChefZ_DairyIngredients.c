//==============================================================================
// ChefZ_DairyIngredients - die Skriptseite der beiden GARBAREN Milchprodukte.
//
// Slice "dairy". Production Map §50 (Kaese), §51 (Ei).
//
// ---------------------------------------------------------------------------
// Warum hier nur ZWEI der fuenf Klassen des Slice stehen
// ---------------------------------------------------------------------------
// ChefZ_Milk, ChefZ_Cream und ChefZ_Butter stehen bewusst NICHT hier.
//
//   - Milch und Sahne haben keinen Food-Knoten. Sie liegen in keinem
//     Pflicht-Slot eines Kochgeraets, HasFoodStage() ist false, und
//     Cooking.ProcessItemToCook laesst sie unangetastet liegen (Cooking.c:47).
//     Eine Skriptklasse wuerde daran nichts aendern.
//   - ChefZ_Butter erbt in der config.cpp von Lard, und Lard.c ueberschreibt
//     CanBeCooked() mit "return true". Die Butter erreicht ihre Endstufe damit
//     bereits ueber die Vanilla-Skriptkette. Sie hier anzufassen hiesse, ihr
//     Lards Bratfett-Verhalten (IsMeat, ActionEatMeat, CanDecay) wegzunehmen
//     und danach von Hand nachzubauen - eine Aenderung ohne Gegenwert.
//
// ---------------------------------------------------------------------------
// Kaese und Ei: die CONFIG-Elternklasse bleibt, die SKRIPT-Kette wechselt
// ---------------------------------------------------------------------------
// Beide Configklassen erben von einer Vanilla-Klasse (BoxCerealCrunchin bzw.
// Marmalade), und das ist weiterhin richtig so: die Vanilla-Basis liefert das
// Proxy-Modell samt Inventarform, ohne dass irgendwo ein p3d-Pfad geraten
// werden muss. Ein falsch geratener Pfad faellt erst beim Packen auf, eine
// geerbte Klasse nie. An der config.cpp aendert sich deshalb NICHTS -
// Aussehen, Inventarform und Rotationsflaechen bleiben Byte fuer Byte gleich.
//
// Config-Vererbung und Skript-Vererbung sind in Enfusion zwei getrennte
// Achsen. Der Defekt lag ausschliesslich auf der Skriptachse: DayZ sucht zu
// einer Configklasse die GLEICHNAMIGE Skriptklasse, fand fuer ChefZ_Cheese und
// ChefZ_Egg keine und nahm die der Config-Elternklasse - BoxCerealCrunchin
// bzw. Marmalade. Beide erben von Edible_Base, und Edible_Base.CanBeCooked()
// liefert false (Edible_Base.c:129). Folge: Cooking.ProcessItemToCook ging an
// beiden vorbei (Cooking.c:47), die Garstufe blieb auf Raw stehen, und
// RCP_ChefZ_CheeseFlatbread - das ChefZ_Cheese in einem PFLICHT-Slot fuehrt -
// wurde nie fertig, weil ChefZ_RecipeEvaluator.CheckStages von jeder
// gebundenen Pflichtzutat eine erlaubte Endstufe verlangt. Die
// FoodStageTransitions in der config.cpp waren toter Text.
//
// ChefZ_Edible_Base.CanBeCooked() rechnet die Antwort aus den Daten der Klasse
// aus: FoodStage-Objekt vorhanden UND Food > FoodStageTransitions deklariert.
// Beides trifft auf Kaese und Ei zu, auf keine andere Klasse dieses Slice.
//
// ZWEI FOLGEN, offen benannt, weil sie im Spiel sichtbar sind:
//
//   1. Beide tragen ab jetzt einen ChefZ-Zustand. Der Abschnittskopf der
//      config.cpp sagt "dieser Slice vergibt bewusst keinen ChefZ-Zustand" -
//      dieser Satz gilt weiter fuer Milch, Sahne und Butter. Fuer die beiden
//      Klassen, die IM TOPF LIEGEN, ist er nicht haltbar: der Zustandsblock
//      ist der Preis der Kochbarkeit, und ohne Kochbarkeit gibt es kein
//      Kaesefladenbrot.
//   2. CanDecay() folgt ab jetzt dem Zutatendatensatz statt Vanillas Default.
//      Config/Ingredients/Dairy.json sagt fuer beide "decays": true; bisher
//      lief Edible_Base.CanDecay() == false (Edible_Base.c:730) und die Zeile
//      war wirkungslos. Kaese und Ei verderben ab jetzt tatsaechlich - das ist
//      das, was der Datensatz seit jeher sagt, und keine neue Meinung.
//
// Layer: 4_World.
//==============================================================================

//! §50: Kaese. GENAU EINE Sorte in V1.
//!
//! Config: ChefZ_Cheese : BoxCerealCrunchin (Proxy-Modell Schachtel).
//! Skript: ChefZ_Edible_Base, damit CanBeCooked() den Daten folgt.
class ChefZ_Cheese extends ChefZ_Edible_Base
{
    /**
     * Die Essaktion, die die vorherige Skriptkette mitgebracht hat.
     *
     * Sie steht hier, weil Vanilla Essaktionen NICHT auf Edible_Base
     * registriert, sondern auf jeder Nahrungsklasse einzeln (Potato.c,
     * Lard.c, BoxCerealCrunchin.c - alle mit demselben Zweizeiler). Solange
     * die Skriptklasse BoxCerealCrunchin war, kamen ActionForceFeed und
     * ActionEatCereal von dort. Mit dem Wechsel auf ChefZ_Edible_Base fielen
     * sie ersatzlos weg, und der Spieler haette ein Stueck Kaese in der Hand,
     * das er nicht essen kann.
     *
     * NICHT ActionEatCereal, obwohl BoxCerealCrunchin genau die mitbrachte:
     * ActionEatCereal.OnFinishProgressServer laesst mit einer Wahrscheinlichkeit
     * eine OrienteeringCompass auf den Boden fallen - Vanillas Muesli-Osterei
     * (ActionEatCereal.c). An einer Muesli-Schachtel ist das ein Gag, an einem
     * Laib Kaese waere es ein Bug: Kaese essen liesse Kompasse regnen.
     *
     * ActionEatBig ist der wertgleiche Ersatz ohne diesen Nebeneffekt und die
     * Variante, die Vanilla fuer grosse Portionen nimmt (Rice.c, Marmalade.c).
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatBig);
    }
}

//! §51: Ei. Wird als es selbst gebraten und gekocht.
//!
//! Config: ChefZ_Egg : Marmalade (Proxy-Modell kleines Glas).
//! Skript: ChefZ_Edible_Base, damit CanBeCooked() den Daten folgt.
class ChefZ_Egg extends ChefZ_Edible_Base
{
    //! Dieselben zwei Aktionen, die Marmalade.c registriert - siehe die
    //! Begruendung an ChefZ_Cheese.SetActions().
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatBig);
    }
}
