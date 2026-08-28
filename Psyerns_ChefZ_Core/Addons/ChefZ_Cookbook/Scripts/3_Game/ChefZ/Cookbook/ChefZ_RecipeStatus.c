//==============================================================================
// ChefZ_RecipeStatus - die vier Zustaende eines Rezepts im Kochbuch
//
// Entwurf: ChefZ_Cookbook_Workflow §4.2.
//
// Der Zustand wird ABGELEITET und nie gespeichert. Das ist die zentrale
// Entscheidung des Wissensmodells, und sie hat eine konkrete Folge: kommt ein
// Rezept dazu oder aendert sich seine Zutatenliste, rechnet der naechste Login
// den Zustand neu aus. Ein gespeicherter Zustand waere nach jeder
// Contentaenderung falsch - und zwar still.
//
// Als Konstanten und nicht als enum, weil die Werte ueber RPC gehen und dort
// als int ankommen. Ein enum haette denselben Wert, aber der Vergleich mit
// einem gelesenen int waere eine Typkonvertierung mehr, die niemand liest.
//
// Layer: 3_Game. Reine Daten, kein Engine-Typ.
//==============================================================================

class ChefZ_RecipeStatus
{
    //! Keine einzige Zutat bekannt. Das Rezept erscheint NICHT in der Liste -
    //! ein Kochbuch, das alles zeigt, ist ein Nachschlagewerk und kein Fund.
    static const int UNKNOWN  = 0;

    //! Mindestens eine, aber nicht alle Pflichtzutaten bekannt. Erscheint als
    //! Schemen: Name verdeckt, bekannte Zutaten lesbar, unbekannte nicht.
    static const int PARTIAL  = 1;

    //! Alle Pflichtzutaten bekannt. Voll lesbar.
    static const int KNOWN    = 2;

    //! Zusaetzlich schon einmal erfolgreich gekocht.
    static const int MASTERED = 3;

    static string Name(int status)
    {
        if (status == UNKNOWN)      return "UNKNOWN";
        if (status == PARTIAL)      return "PARTIAL";
        if (status == KNOWN)        return "KNOWN";
        if (status == MASTERED)     return "MASTERED";
        return "?";
    }

    static bool IsValid(int status)
    {
        return status >= UNKNOWN && status <= MASTERED;
    }

    //! Erscheint das Rezept ueberhaupt im Buch?
    static bool IsListed(int status)
    {
        return status > UNKNOWN;
    }

    //! Darf der Name gelesen werden? Bei PARTIAL bleibt er verdeckt - das ist
    //! der ganze Reiz des Zustands.
    static bool ShowsName(int status)
    {
        return status >= KNOWN;
    }
}
