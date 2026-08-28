//==============================================================================
// ChefZ_KnowledgeHooks - wo Wissen entsteht
//
// Entwurf: ChefZ_Cookbook_Workflow §4.3, Zeile 1 der Tabelle.
//
// ---------------------------------------------------------------------------
// WARUM EEItemLocationChanged UND NICHT EEItemIntoHands
// ---------------------------------------------------------------------------
// §4.3 nennt beides. EEItemIntoHands sieht nur, was der Spieler in die Haende
// nimmt - eine Zwiebel, die er aus einer Kiste direkt in den Rucksack zieht,
// bliebe unbekannt. Das waere fuer den Spieler nicht nachvollziehbar: er hat
// sie ja gefunden.
//
// EEItemLocationChanged sieht jeden Ortswechsel und wird auf ItemBase
// ueberschrieben, nicht auf PlayerBase - so haengt der Haken am Gegenstand,
// der etwas ueber sich weiss, statt am Spieler, der jeden Gegenstand einzeln
// befragen muesste.
//
// ---------------------------------------------------------------------------
// DER FALLSTRICK AUS §4.3
// ---------------------------------------------------------------------------
// "Ein Spieler, der einen Rucksack mit 40 Gegenstaenden aufnimmt, loest 40
// Inventarevents in einem Frame aus." Genau das passiert hier auch - und
// deshalb schreibt dieser Haken NICHTS. Er merkt nur vor. Der eine Lauf am
// Frameende steht in ChefZ_PlayerKnowledge.ChefZ_FlushIngredients.
//
// Layer: 4_World. Keine Dabs-Referenz (Regel 3).
//==============================================================================

modded class ItemBase
{
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);
        ChefZ_NoteAsIngredient(newLoc);
    }

    /**
     * Der neue Ort gehoert einem Spieler? Dann kennt er die Zutat jetzt.
     *
     * Serverseitig, wie §4.3 es verlangt ("Alles serverseitig"). Auf dem
     * Client waere der Stand ohnehin nur eine Vermutung, und zwei Quellen der
     * Wahrheit sind eine zu viel.
     */
    private void ChefZ_NoteAsIngredient(InventoryLocation newLoc)
    {
        if (!g_Game || !g_Game.IsDedicatedServer())
            return;

        EntityAI eltern = newLoc.GetParent();
        if (!eltern)
            return;

        PlayerBase spieler = PlayerBase.Cast(eltern.GetHierarchyRootPlayer());
        if (!spieler)
            return;

        // Nur was der Core als Zutat kennt. Ein Vorschlaghammer im Rucksack
        // erzeugt keinen Kochbucheintrag.
        ChefZ_IngredientManager zutaten = ChefZ_IngredientManager.Get();
        if (!zutaten || !zutaten.IsReady())
            return;

        ChefZ_Sym klasse = ChefZ_SymbolTable.Lookup(GetType());
        if (!ChefZ_SymbolTable.IsValid(klasse))
            return;
        if (!zutaten.IsKnown(klasse))
            return;

        spieler.ChefZ_NoteIngredient(klasse);
    }
}
