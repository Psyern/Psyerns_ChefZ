//==============================================================================
// ChefZ_CraftIntent - eine IDENTITAET fuer das, was der Spieler craften wollte.
//
// Entwurf: 11 §4 (HANDCRAFT laeuft ueber Vanillas Craftsystem), 00 §5
// ("nichts Autoritatives auf dem Client"), 18 §4 (der Betreiber muss den Fall
// im RPT sehen koennen).
//
// ---------------------------------------------------------------------------
// WOZU
// ---------------------------------------------------------------------------
// Vanillas Craftaktion uebertraegt beim Start die REZEPT-ID, und die ist eine
// POSITION in der Rezeptliste des jeweiligen Prozesses - nicht die Identitaet
// des Rezepts. Solange Client und Server dieselbe Liste in derselben
// Reihenfolge aufbauen, ist das gleichbedeutend. Sobald IRGENDEIN Mod auf den
// beiden Seiten zu unterschiedlichen ZEITPUNKTEN registriert, ist es das
// nicht mehr, und dann meint der Client Rezept A, waehrend der Server Rezept B
// ausfuehrt.
//
// ChefZ_HandcraftBridge beseitigt die URSACHE (Positionsanker, siehe dort).
// Diese Klasse liefert das zweite Netz: eine kleine, positionsunabhaengige
// Kennung, die neben der Position mitlaeuft und die der Server gegen sein
// eigenes Rezept haelt. Stimmen sie nicht ueberein, wird die Aktion
// VERWEIGERT - statt das falsche Ergebnis zu erzeugen.
//
// ---------------------------------------------------------------------------
// WARUM DIE NULL "KEINE ANGABE" HEISST
// ---------------------------------------------------------------------------
// Ein nicht gesetzter int ist in Enforce 0. Genau dieser Fall tritt real auf:
// im Einzelspielerbetrieb wird die Aktion gar nicht serialisiert, also
// bekommt der Server nie eine Kennung. Waere 0 gleichbedeutend mit "kein
// ChefZ-Rezept", wuerde dort JEDES ChefZ-Handwerksrezept verweigert.
//
// Deshalb: 0 = UNKNOWN = "diese Seite hat nichts mitgeteilt" -> durchlassen.
// Die Verweigerung braucht eine AUSSAGE, nie ein Schweigen. Aus demselben
// Grund braucht keine der beiden Datenklassen einen Konstruktor.
//
// KEIN CONTENT, KEIN FREMDSYSTEM: reine Arithmetik ueber eine Zeichenkette.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_CraftIntent
{
    //! "Diese Seite hat keine Kennung mitgeteilt." Vorgabewert eines
    //! uninitialisierten int - siehe Dateikopf.
    static const int UNKNOWN = 0;

    //! "Die Position, die uebertragen wurde, ist KEIN fertiges ChefZ-Rezept."
    //! Das ist der Normalfall: jedes Vanilla- und jedes Fremdrezept meldet
    //! diesen Wert, und zwei solche Meldungen sind gleich - ChefZ mischt sich
    //! in fremdes Crafting nicht ein.
    static const int NOT_CHEFZ = 1;

    /**
     * Bit 30. Es setzt zwei Dinge zugleich durch:
     *
     *   - der Wert kann nie UNKNOWN (0) und nie NOT_CHEFZ (1) werden, egal
     *     was Hash() liefert;
     *   - er ist auf beiden Seiten identisch, weil er nur von der
     *     Transform-ID abhaengt und von nichts sonst.
     */
    static const int MARK = 0x40000000;

    /**
     * Die Kennung eines HANDCRAFT-Transforms.
     *
     * Sie ist bewusst KEIN Ausweis und kein Geheimnis: der Client kann sie
     * frei erfinden. Er gewinnt damit nichts - der Server benutzt sie
     * ausschliesslich, um zu VERWEIGERN, nie um zu erlauben. Eine falsche
     * Kennung fuehrt zu einer abgelehnten Aktion, eine richtige zu genau der
     * Pruefung, die ohnehin folgt (CanDo -> Matcher).
     */
    static int Of(string transformId)
    {
        if (transformId == "")
            return NOT_CHEFZ;

        return transformId.Hash() | MARK;
    }

    //! Traegt dieser Wert eine ChefZ-Rezeptidentitaet?
    static bool IsChefZ(int intent)
    {
        return intent != UNKNOWN && intent != NOT_CHEFZ;
    }

    /**
     * Darf der Server die Aktion ausfuehren?
     *
     * @param clientIntent  was die Gegenseite gemeint hat, oder UNKNOWN
     * @param serverIntent  was an DIESER Position tatsaechlich steht
     *
     * Drei Faelle, und nur der dritte ist eine Verweigerung:
     *
     *   UNKNOWN            keine Angabe -> durchlassen. Der Vanilla-Pfad
     *                      bleibt damit unter allen Umstaenden unveraendert.
     *   gleich             uebereinstimmend -> durchlassen.
     *   ungleich           Versatz. Der Spieler haette etwas anderes
     *                      bekommen, als er ausgewaehlt hat.
     */
    static bool Accepts(int clientIntent, int serverIntent)
    {
        if (clientIntent == UNKNOWN)
            return true;

        return clientIntent == serverIntent;
    }

    //! Kurzform fuer Logzeilen. Kein Content, kein Name - nur die Art.
    static string Describe(int intent)
    {
        if (intent == UNKNOWN)
            return "keine Angabe";
        if (intent == NOT_CHEFZ)
            return "kein ChefZ-Rezept";
        return "ChefZ-Rezept #" + intent.ToString();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        if (Of("") != NOT_CHEFZ)                        return false;

        int a = Of("A");
        int b = Of("B");
        if (a == UNKNOWN || a == NOT_CHEFZ)             return false;
        if (b == UNKNOWN || b == NOT_CHEFZ)             return false;
        if (a != Of("A"))                               return false;   // stabil
        if (!IsChefZ(a) || !IsChefZ(b))                 return false;
        if (IsChefZ(UNKNOWN) || IsChefZ(NOT_CHEFZ))     return false;

        // Schweigen laesst durch, Widerspruch nicht.
        if (!Accepts(UNKNOWN, a))                       return false;
        if (!Accepts(UNKNOWN, NOT_CHEFZ))               return false;
        if (!Accepts(a, a))                             return false;
        if (!Accepts(NOT_CHEFZ, NOT_CHEFZ))             return false;
        if (Accepts(a, NOT_CHEFZ))                      return false;
        if (Accepts(NOT_CHEFZ, a))                      return false;
        if (a != b && Accepts(a, b))                    return false;

        return true;
    }
}
