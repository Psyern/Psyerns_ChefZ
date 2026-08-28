//==============================================================================
// ChefZ_CookbookItem - das Kochbuch als Gegenstand
//
// Entwurf: ChefZ_Cookbook_Workflow §6.3 ("Das Buch muss im Inventar liegen").
//
// Die Klasse ist absichtlich duenn. Ein Buch ist ein Gegenstand, den man
// besitzt - es rechnet nichts, es speichert nichts, es entscheidet nichts. Das
// Wissen haengt am Charakter (ChefZ_PlayerKnowledge), nicht am Papier; wer sein
// Buch verliert, vergisst nicht, was er gelernt hat.
//
// Die einzige Aufgabe hier: die Frage "hat dieser Spieler ein Kochbuch dabei"
// an EINER Stelle beantworten, damit Tastenkuerzel und Item-Aktion sich nicht
// widersprechen koennen.
//
// Layer: 4_World. Keine Dabs-Referenz (Regel 3) - das Buch existiert auch auf
// einem Server ohne Dabs, es laesst sich dort nur nicht aufschlagen.
//==============================================================================

class ChefZ_CookbookItem : Inventory_Base
{
    /**
     * Traegt der Spieler ein Kochbuch bei sich?
     *
     * Durchsucht die gesamte Hierarchie, nicht nur die Haende: ein Buch im
     * Rucksack ist ein Buch. Vanillas GetInventory().EnumerateInventory
     * liefert genau diese Sicht.
     */
    static bool CarriedBy(PlayerBase spieler)
    {
        if (!spieler)
            return false;

        GameInventory inv = spieler.GetInventory();
        if (!inv)
            return false;

        array<EntityAI> alles = new array<EntityAI>();
        inv.EnumerateInventory(InventoryTraversalType.PREORDER, alles);

        for (int i = 0; i < alles.Count(); i++)
        {
            ChefZ_CookbookItem buch = ChefZ_CookbookItem.Cast(alles.Get(i));
            if (buch)
                return true;
        }
        return false;
    }
}
