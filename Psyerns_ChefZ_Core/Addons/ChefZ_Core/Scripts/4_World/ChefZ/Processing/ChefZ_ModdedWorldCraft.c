//==============================================================================
// modded class ActionWorldCraft (+ die beiden zugehoerigen Datenhalter) -
// das ZWEITE NETZ gegen Rezept-ID-Versatz.
//
// Entwurf: 00 §5 ("Rezeptaufloesung und Item-Erzeugung sind serverseitig;
// nichts Autoritatives auf dem Client"), 11 §5, 18 §4 (der Betreiber muss den
// Fall im RPT sehen koennen), 19 S15.
//
//==============================================================================
// BEGRUENDUNG DIESER DREI OVERRIDES
//==============================================================================
// WARUM UEBERHAUPT: Vanillas Craftaktion uebertraegt beim Start genau eine
// Zahl - die POSITION des Rezepts in der Rezeptliste
// (ActionWorldCraft.WriteToContext: ctx.Write(action_data_wc.m_RecipeID)).
// Eine Position ist keine Identitaet. Bauen Client und Server ihre Liste
// unterschiedlich auf, meint dieselbe Zahl auf den beiden Seiten
// verschiedene Rezepte - und der Spieler bekommt schweigend etwas anderes,
// als er ausgewaehlt hat.
//
// Die URSACHE beseitigt ChefZ_HandcraftBridge mit dem Positionsanker. Was
// dieser Anker nicht abdeckt, steht dort unter "WAS DER ANKER NICHT LEISTET":
// eine Modliste des Clients, die nicht die des Servers ist, und
// $profile-Overlays, die nur der Server sieht (Rang 3, 02 §6). Fuer genau
// diesen Rest gibt es diese Datei.
//
// WAS SIE TUT: sie legt EINE zusaetzliche Zahl neben die Position - eine
// positionsunabhaengige Kennung des gemeinten Rezepts (ChefZ_CraftIntent).
// Der Server haelt sie gegen das Rezept, das bei IHM an dieser Position
// steht. Widersprechen sich die beiden, wird die Ausfuehrung ausgelassen.
//
// WARUM DIE PRUEFUNG NICHT IN ChefZ_GenericCraftRecipe.CanDo() REICHT: CanDo
// laeuft auf dem Rezept, das der SERVER an dieser Position hat. Zeigt die
// verschobene Position auf ein FREMDES Rezept, wird CanDo dieses fremden
// Rezepts gefragt - und das weiss von ChefZ nichts. Genau dieser Fall ist der
// gefaehrliche: er erzeugt ein falsches Item statt gar keins. Er ist nur an
// der Aktion abfangbar, nicht am Rezept.
//
//==============================================================================
// WARUM DAS VANILLA-CRAFTING DAVON NICHTS MERKT
//==============================================================================
// Vier Eigenschaften, einzeln pruefbar:
//
//   1. Geschrieben wird IMMER und IMMER NACH super. Der Vanilla-Teil des
//      Datenstroms ist damit Byte fuer Byte unveraendert; die ChefZ-Zahl
//      haengt hinten an. Vanilla selbst schreibt danach nur noch die
//      Quittungsnummer (ActionManagerClient: ctx.Write(m_PendingAck...)),
//      und die liest ActionManagerServer ebenfalls erst nach
//      ReadFromContext - die Reihenfolge bleibt also auf beiden Seiten
//      dieselbe.
//
//   2. Gelesen wird IMMER und IMMER NACH super, und ein FEHLSCHLAG DES
//      LESENS GIBT NIEMALS false ZURUECK. Er wird als "keine Angabe"
//      gewertet. Ein false an dieser Stelle wuerde die Aktion abweisen - und
//      das waere der eine Weg, auf dem diese Datei Vanilla-Crafting
//      beschaedigen koennte.
//
//   3. "Keine Angabe" laesst durch. Das ist kein Zugestaendnis, sondern der
//      Normalfall im Einzelspielerbetrieb: dort wird die Aktion gar nicht
//      serialisiert (ActionManagerClient ruft WriteToContext nur bei
//      g_Game.IsMultiplayer()), und beide Seiten sind ohnehin derselbe
//      Prozess.
//
//   4. Verweigert wird ausschliesslich bei WIDERSPRUCH. Ohne ChefZ-Rezepte
//      melden beide Seiten fuer jede Position denselben Wert
//      (ChefZ_CraftIntent.NOT_CHEFZ), und der Vergleich geht immer auf.
//
// VERTRAEGLICHKEIT MIT ANDEREN MODS: modded class kettet, und alle drei
// Overrides rufen super als erste Anweisung. Ein Fremdmod, der ebenfalls
// WriteToContext/ReadFromContext erweitert, haengt seine Daten vor oder nach
// unseren an - auf beiden Seiten an derselben Stelle der Kette, weil die
// Ladereihenfolge der Mods auf Client und Server dieselbe ist. Ein Fremdmod,
// der super NICHT ruft, bricht bereits Vanillas eigene Uebertragung.
//
// KEIN CONTENT: kein Rezept, kein Prozess, kein Item wird hier benannt.
//
// Layer: 4_World.
//==============================================================================

/**
 * Der Datenhalter der laufenden Aktion.
 *
 * BEWUSST OHNE KONSTRUKTOR: ChefZ_CraftIntent.UNKNOWN ist 0, und ein nicht
 * gesetzter int ist in Enforce 0. Der Vorgabewert ist damit genau der, den
 * der Einzelspielerpfad braucht - und diese Erweiterung besteht aus einer
 * einzigen Felddeklaration.
 */
modded class WorldCraftActionData
{
    int m_ChefZ_Intent;
}

//! Dasselbe Feld auf der Empfangsseite. Vanillas eigenes Muster: jedes Feld,
//! das ueber das Netz kommt, existiert einmal hier und einmal oben, und
//! HandleReciveData kopiert es hinueber.
modded class WorldCraftActionReciveData
{
    int m_ChefZ_Intent;
}

modded class ActionWorldCraft
{
    /**
     * CLIENT: die Kennung des gemeinten Rezepts anhaengen.
     *
     * Sie wird HIER berechnet und nicht in SetupAction, weil sie zum
     * Zeitpunkt des Sendens gelten muss: m_RecipeID steht dann fest, und die
     * Rezeptliste dieser Seite ebenfalls.
     */
    override void WriteToContext(ParamsWriteContext ctx, ActionData action_data)
    {
        super.WriteToContext(ctx, action_data);

        int intent = ChefZ_CraftIntent.UNKNOWN;

        WorldCraftActionData data = WorldCraftActionData.Cast(action_data);
        if (data)
            intent = ChefZ_HandcraftBridge.IntentOfRecipeId(data.m_RecipeID);

        ctx.Write(intent);
    }

    /**
     * SERVER: die Kennung mitlesen.
     *
     * Der Rueckgabewert von super wird unveraendert durchgereicht, und ein
     * fehlgeschlagenes Read fuehrt NIE zu false - siehe Dateikopf,
     * Eigenschaft 2.
     */
    override bool ReadFromContext(ParamsReadContext ctx, out ActionReciveData action_recive_data)
    {
        if (!super.ReadFromContext(ctx, action_recive_data))
            return false;

        int intent;
        if (!ctx.Read(intent))
            intent = ChefZ_CraftIntent.UNKNOWN;

        WorldCraftActionReciveData recv = WorldCraftActionReciveData.Cast(action_recive_data);
        if (recv)
            recv.m_ChefZ_Intent = intent;

        return true;
    }

    //! SERVER: das mitgelesene Feld in die Aktionsdaten uebernehmen - genau
    //! so, wie Vanilla es eine Zeile darueber mit m_RecipeID tut.
    override void HandleReciveData(ActionReciveData action_recive_data, ActionData action_data)
    {
        super.HandleReciveData(action_recive_data, action_data);

        WorldCraftActionReciveData recv = WorldCraftActionReciveData.Cast(action_recive_data);
        WorldCraftActionData       data = WorldCraftActionData.Cast(action_data);

        if (recv && data)
            data.m_ChefZ_Intent = recv.m_ChefZ_Intent;
    }

    /**
     * SERVER: der Torwaechter, unmittelbar vor der Ausfuehrung.
     *
     * Bei Widerspruch wird super AUSGELASSEN. Das ist bewusst und es ist
     * sicher: Vanillas eigene Fassung dieser Methode ruft ihrerseits kein
     * super auf und tut selbst schon nichts, wenn Item oder Ziel fehlen
     * (ActionWorldCraft.OnFinishProgressServer, erste Zeile im if). Der
     * ausgelassene Fall ist damit derselbe Zustand, den Vanilla ohnehin
     * kennt: die Aktion endet regulaer ueber OnEndServer, es wird nichts
     * erzeugt und nichts verbraucht.
     *
     * Die Alternative - super rufen und danach aufraeumen - gaebe es nicht:
     * PerformRecipeServer erzeugt die Ergebnisse, bevor irgendjemand
     * widersprechen koennte.
     */
    override void OnFinishProgressServer(ActionData action_data)
    {
        WorldCraftActionData data = WorldCraftActionData.Cast(action_data);

        if (data && !ChefZ_HandcraftBridge.AcceptCraftIntent(data.m_RecipeID, data.m_ChefZ_Intent))
            return;

        super.OnFinishProgressServer(action_data);
    }
}
