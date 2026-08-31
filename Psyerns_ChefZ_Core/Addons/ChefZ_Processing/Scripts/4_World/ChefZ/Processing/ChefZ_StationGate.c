//==============================================================================
// ChefZ_StationGate - die eine Frage, die jeder Torwaechter dieses Moduls
//                     stellt: gehoert dieses Item in DIESE Station?
//
// ### 31.08.2026 ###
//
// WARUM ES TORWAECHTER UEBERHAUPT GIBT
// ------------------------------------
// Eine Verarbeitungsstation haette gern zwei Bereiche - einen fuer das, was
// hineingeht, und einen fuer das, was herauskommt. Die Engine gibt das nicht
// her: ein Item hat GENAU EINEN Cargo (GameInventory.GetCargo()), und
// ChefZ_ProcessingStation_Base sammelt seine Zutaten ueber
// ChefZ_FactCollector.CollectFromCargo aus eben diesem einen Bereich. Die
// Ergebnisse entstehen ebenfalls darin.
//
// Ein Fleischwolf, in den man auch Konserven, Munition und Schuhe legen kann,
// ist deshalb nicht nur unordentlich - er ist ein Lager mit Kurbel. Die
// Trennung "Eingang / Ausgang" findet in diesem Modul stattdessen AM EINGANG
// statt: EntityAI.CanReceiveItemIntoCargo (scripts - 1.29/3_Game/DayZ/
// Entities/EntityAI.c:1550-1559) entscheidet, was ueberhaupt hineindarf.
// Vorbild der Ueberschreibung ist Barrel_ColorBase.c:512; im Projekt macht es
// ChefZ_HoneyExtractor seit dem Apiary-Slice genauso.
//
// WARUM KATEGORIEN UND NICHT KLASSENNAMEN
// ---------------------------------------
// Ein Torwaechter aus Klassennamen ist beim Schreiben bequem und beim naechsten
// Slice falsch. Die Transforms der Fleischkette nennen ihre Eingaenge zu einem
// grossen Teil ueber KATEGORIEN - { "category": "MEAT" }, { "category":
// "SPICE" }, { "category": "CASING" } -, und wer morgen ein neues Gewuerz
// anlegt, traegt es in die Kategorie ein und nicht in eine Liste im Skript
// eines fremden Moduls. Ein Namenstorwaechter haette dieses Gewuerz stumm
// ausgesperrt und die Wurstrezepte damit unerfuellbar gemacht - genau die Art
// Fehler, die dieser Slice gerade aufraeumt.
//
// Die Frage geht deshalb an dieselbe Stelle, die auch der Matcher fragt:
// ChefZ_IngredientManager fuer die Stammdaten der Klasse, ChefZ_CategoryManager
// fuer den Vorfahrenvergleich. IsInCategory arbeitet auf der CLOSURE, also auf
// dem self-or-ancestor-Bitset - "MEAT" trifft damit auch MINCED_MEAT, SAUSAGE,
// WILD_MEAT, DOMESTIC_MEAT, POULTRY und PREDATOR_MEAT, ohne dass hier eine
// dieser sechs steht (Kategoriebaum: ChefZ_Registry/Config/Categories.json).
//
// KEIN NEUES CORE-SYSTEM: diese Datei liest zwei vorhandene Manager und haelt
// keinen eigenen Zustand. Sie liegt in ChefZ_Processing, weil nur die
// Stationen dieses Moduls sie brauchen.
//
// NICHT BEREIT HEISST NICHT GESPERRT
// ----------------------------------
// CanReceiveItemIntoCargo laeuft auch auf dem Client und auch, bevor
// ChefZ_Boot die Manager gebaut hat. Waere die Antwort dann "nein", liesse sich
// in genau diesem Fenster gar nichts einlagern - und der Spieler saehe eine
// Station, die grundlos nichts annimmt. Deshalb beantwortet ChefZ_RegistryReady
// die Vorfrage, und jeder Torwaechter laesst bei "noch nicht bereit" alles
// durch. Der Server entscheidet ohnehin autoritativ, und er ist der Letzte,
// der ohne Manager laeuft.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_StationGate
{
    /**
     * Stehen Zutaten- und Kategorieregister? Nur dann darf ein Torwaechter
     * ueberhaupt etwas ablehnen.
     */
    static bool ChefZ_RegistryReady()
    {
        ChefZ_IngredientManager ingredients = ChefZ_IngredientManager.Get();
        if (!ingredients)
            return false;
        if (!ingredients.IsReady())
            return false;

        ChefZ_CategoryManager categories = ChefZ_CategoryManager.Get();
        if (!categories)
            return false;
        return categories.IsReady();
    }

    /**
     * Liegt das Item in dieser Kategorie oder in einer ihrer Unterkategorien?
     *
     * @param item          das Item, das ins Cargo will.
     * @param categoryName  Kategorie-ID, wie sie in Categories.json steht.
     *
     * false ist die haeufigste Antwort und kein Fehler: 99 % aller
     * Vanilla-Klassen sind nicht als Zutat deklariert (05 §7), und eine
     * undeklarierte Klasse hat eine leere Closure.
     */
    static bool ChefZ_InCategory(EntityAI item, string categoryName)
    {
        if (!item)
            return false;
        if (!ChefZ_RegistryReady())
            return false;

        // Lookup und nicht Intern: eine Abfrage legt keine Symbole an. Steht
        // die Kategorie nicht in der Registry, ist das Symbol ungueltig, und
        // die Antwort lautet "nein" - der Torwaechter faellt dann auf seine
        // uebrigen Bedingungen zurueck, statt alles zu sperren.
        ChefZ_Sym category = ChefZ_SymbolTable.Lookup(categoryName);
        if (!ChefZ_SymbolTable.IsValid(category))
            return false;

        ChefZ_IngredientManager ingredients = ChefZ_IngredientManager.Get();
        ChefZ_IngredientInfo info = ingredients.ResolveByName(item.GetType());
        if (!info)
            return false;

        ChefZ_CategoryManager categories = ChefZ_CategoryManager.Get();
        return categories.IsInCategory(info.closure, category);
    }

    /**
     * Wie viele Items im Cargo dieser Station liegen in der Kategorie?
     *
     * Die Zaehlung ist der zweite Teil jedes Torwaechters: das Cargo-Gitter
     * gibt den PLATZ vor, die Kapazitaet gibt die Station vor. Dieselbe
     * Arbeitsteilung benutzt ChefZ_HoneyExtractor fuer Rahmen und Glaeser.
     */
    static int ChefZ_CountInCategory(EntityAI station, string categoryName)
    {
        int count = 0;
        if (!station)
            return count;

        GameInventory inventory = station.GetInventory();
        if (!inventory)
            return count;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return count;

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI entry = cargo.GetItem(i);
            if (ChefZ_InCategory(entry, categoryName))
                count = count + 1;
        }
        return count;
    }
}
