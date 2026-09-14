//==============================================================================
// ### SLICE fish-sea-b ###   Skriptklassen der elf Arten.
//
// Je Configklasse eine Skriptklasse, leer. Die Bindung ist der Zweck: ohne sie
// traegt das Item keinen ChefZ-Zustand (ChefZ_Edible_Base.c, Kopf), und ein
// Cast auf die Art ginge ins Leere.
//
// NICHT HIER: die Filets. ChefZ_BluefinTunaFillet und die zehn anderen gehoeren
// dem Slice "fillets" (ChefZ_fillets_Items.c). Dieser Slice gibt sie nur als
// Ergebnis seiner Transforms aus.
//
// Was ein Fisch IST - Kategorie, Tag, Zustand, Naehrwert, Filetausbeute - steht
// in Config/Ingredients/Fish_Sea_B.json und Config/Processing/Fillet_Sea_B.json,
// nicht hier. Jede Zeile Verhalten waere Content, der sich nicht mehr ueber eine
// Datei aendern liesse.
//
// ---------------------------------------------------------------------------
// GANZER FISCH: nicht kochbar, aber essbar
// ---------------------------------------------------------------------------
// ChefZ_Edible_Base.CanBeCooked() antwortet datengetrieben: es liest, ob die
// Configklasse Food > FoodStageTransitions deklariert. ChefZ_SeaFishB_Base
// deklariert KEINEN Food-Knoten - der ganze Fisch ist damit nicht kochbar, und
// es steht dafuer keine Zeile Code hier. Genau so haelt es Vanilla mit seiner
// Makrele (Mackerel.c: CanBeCooked() false, IsCorpse() true), und genau so
// haelt es der Nachbarslice fish-fresh.
//
// ROH GEGESSEN werden kann er trotzdem, und auch das ist Absicht: agents = 4
// (eAgents.SALMONELLA) an jeder der elf Configklassen macht daraus ein
// Salmonellenrisiko. Der Weg ohne Risiko ist PROCESS_FILLET_FISH und danach
// die Pfanne. Ohne die Essaktion gaebe es keine Wahl - nur ein Item, das das
// Spiel stumm nicht zum Essen anbietet.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der elf Arten (Configklasse gleichen Namens, scope 0).
class ChefZ_SeaFishB_Base extends ChefZ_Edible_Base
{
    /**
     * Vanillas Makrele sagt dasselbe (Mackerel.c). Object.IsCorpse() steuert
     * ueber Object.IsAttractingInfected() (Object.c:721), ob ein Gegenstand
     * Infizierte anzieht - ein ganzer Fisch im Rucksack tut das. Am Filet steht
     * die Zusage nicht: das ist Fleisch, kein Kadaver.
     */
    override bool IsCorpse()
    {
        return true;
    }

    /**
     * Vanillas Fischfilets sagen dasselbe (MackerelFilletMeat.c, CarpFilletMeat.c),
     * und der Nachbarslice fish-fresh sagt es an seinen ganzen Fischen. Ohne die
     * Zusage nimmt Edible_Base.ProcessDecay den Zweig fuer geoeffnete Dosen statt
     * den fuer rohes Fleisch, und ActionEatMeat.ApplyModifiers laesst die
     * blutigen Haende aus.
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Die Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (Lard.c:36-42, Potato.c). Ohne diese Zeilen bietet
     * das Spiel den Fisch nicht zum Essen an - ohne Fehlerbild und ohne
     * Logzeile.
     *
     * ActionEatMeat und nicht ActionEatBig: die Fleischvariante bringt
     * ApplyModifiers mit (blutige Haende bei rohem Fleisch) und verbraucht
     * UAQuantityConsumed.EAT_NORMAL. Dieselbe Wahl trifft fish-fresh.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}

class ChefZ_BluefinTuna extends ChefZ_SeaFishB_Base
{
}

class ChefZ_YellowfinTuna extends ChefZ_SeaFishB_Base
{
}

class ChefZ_RedSnapper extends ChefZ_SeaFishB_Base
{
}

class ChefZ_GiantGrouper extends ChefZ_SeaFishB_Base
{
}

class ChefZ_MahiMahi extends ChefZ_SeaFishB_Base
{
}

class ChefZ_GiantTrevally extends ChefZ_SeaFishB_Base
{
}

class ChefZ_BlueMarlin extends ChefZ_SeaFishB_Base
{
}

class ChefZ_Sailfish extends ChefZ_SeaFishB_Base
{
}

class ChefZ_Bonito extends ChefZ_SeaFishB_Base
{
}

class ChefZ_Yellowtail extends ChefZ_SeaFishB_Base
{
}

class ChefZ_Tarpon extends ChefZ_SeaFishB_Base
{
}
