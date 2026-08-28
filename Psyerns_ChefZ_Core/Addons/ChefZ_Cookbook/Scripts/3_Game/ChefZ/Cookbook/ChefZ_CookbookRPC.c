//==============================================================================
// ChefZ_CookbookRPC - die Kennungen, und warum sie alle hier stehen
//
// Entwurf: ChefZ_Cookbook_Workflow §5, Regel 5.
//
// ---------------------------------------------------------------------------
// AB 10000, UND ZWAR ALLE AN EINER STELLE
// ---------------------------------------------------------------------------
// Unterhalb von 10000 liegt Vanillas Bereich. Wer dort hineingreift, bekommt
// keinen Fehler - er bekommt fremde Nachrichten und schickt seine an fremde
// Empfaenger. Die Enforce-Skill nennt 10000 als Untergrenze fuer Mods.
//
// Sie stehen in EINER Datei, damit chefz-conflict-scout eine Stelle hat, an der
// er den ganzen Bereich sieht. Eine ueber vier Dateien verstreute Nummernvergabe
// ist genau die Art Fehler, die niemand beim Lesen findet.
//
// Der Abstand von 100 zwischen den Bloecken ist Absicht: kommt in V2 eine
// dritte Nachricht dazu, muss keine bestehende Nummer wandern - und eine
// wandernde RPC-Nummer bricht die Verstaendigung mit jedem Client, der noch die
// alte Fassung hat.
//
// Layer: 3_Game. Reine Konstanten.
//==============================================================================

class ChefZ_CookbookRPC
{
    //! Untergrenze des Modbereichs. Steht hier, damit der Pruefer sie
    //! mitlesen kann statt sie zu kennen.
    static const int BASE = 10000;

    //! Server -> Client: der VOLLSTAENDIGE Wissensstand.
    //! Anlass: Spieler verbindet, oder er oeffnet das Buch das erste Mal.
    static const int FULL_STATE = 10000;

    //! Server -> Client: nur das Delta.
    //! Anlass: eine neue Zutat oder ein neu gemeistertes Rezept.
    static const int DELTA = 10001;

    //! Client -> Server: "schick mir den Stand".
    //! Anlass: das Buch wird geoeffnet und der Clientspiegel ist leer.
    static const int REQUEST_STATE = 10002;

    //! Erste freie Nummer. Wer eine neue Nachricht braucht, nimmt sie und
    //! schiebt diese Marke hoch.
    static const int NEXT_FREE = 10003;

    static bool IsOurs(int rpcType)
    {
        return rpcType >= FULL_STATE && rpcType < NEXT_FREE;
    }

    static string Name(int rpcType)
    {
        if (rpcType == FULL_STATE)      return "FULL_STATE";
        if (rpcType == DELTA)           return "DELTA";
        if (rpcType == REQUEST_STATE)   return "REQUEST_STATE";
        return "?";
    }
}
