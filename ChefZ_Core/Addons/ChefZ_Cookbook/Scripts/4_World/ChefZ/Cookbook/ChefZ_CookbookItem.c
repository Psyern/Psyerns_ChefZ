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
     * Die Aktion "Kochbuch aufschlagen" an das Buch haengen.
     *
     * Die Registrierung in ChefZ_ActionRegistration.c legt die Aktion nur an.
     * Sichtbar wird sie erst, wenn ein Item sie in SetActions per AddAction
     * aufnimmt - so und nicht anders funktioniert das in Vanilla
     * (1.30 scripts/4_World/DayZ/Entities/ItemBase.c:246 SetActions und :255
     * AddAction(typename); in 1.29 gleich). Ohne diese Ueberschreibung war der
     * Eintrag auf jedem Server tot, und nur das Tastenkuerzel aus
     * ChefZ_CookbookInput blieb uebrig.
     *
     * super zuerst, sonst fehlen Aufnehmen, In-die-Haende-nehmen und Ablegen.
     */
    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionOpenCookbook);
    }

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
