//==============================================================================
// ChefZ_CookContext - die Umgebung einer Auswertung
//
// Entwurf: 08 §3 (Feldliste woertlich), 10 §4 (wer ihn fuellt), 10 §5
// (wann er entsteht), 08 §7 ("Die Engine ist zustandslos gegenueber der Welt.
// Zweimal dieselbe Eingabe ergibt zweimal dasselbe Ergebnis").
//
// Der Kontext beantwortet die Frage "wo und womit wird gerade gekocht" -
// getrennt von der Frage "was liegt drin", die der ChefZ_FactSnapshot
// beantwortet. Beide zusammen sind die vollstaendige Eingabe der Engine, und
// beide sind reine Daten: kein ItemBase, kein EntityAI, kein Enum aus
// 4_World. Genau das macht die Auswertung ohne laufendes Spiel pruefbar.
//
// Gefuellt wird er in 4_World von ChefZ_CookingDeviceAdapter.BuildContext()
// (10 §4, S7). Bis dahin fuellt ihn nur der Selbsttest - und das ist der
// Punkt: der Rezeptpfad steht und rechnet, bevor die erste Feuerstelle ihn
// ruft.
//
// KEIN CONTENT: kein Geraetename, keine Kochmethode, keine Fluessigkeit wird
// hier benannt. Alles sind Symbole, und wer sie vergibt, ist Content.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_CookContext
{
    //! Die Klasse des Gefaesses, in dem gekocht wird.
    ChefZ_Sym deviceClass;

    //! Die Klasse darunter - Feuerstelle, Ofen, Gasherd (10 §4). Ein Rezept
    //! darf an beiden binden; typisch bindet es an keiner von beiden, sondern
    //! an einer Geraetekategorie (08 E4).
    ChefZ_Sym deviceRootClass;

    //! Kategorien des Geraets aus ChefZ_DeviceDef. Nie null.
    ref array<ChefZ_Sym> deviceCategories;

    //! Kochmethode dieses Ticks, aus CookingMethodType gemappt (10 §4).
    //! INVALID heisst "keine" und ist ein gueltiger Wert - Rezepte, die keine
    //! Methode nennen, laufen trotzdem.
    ChefZ_Sym method;

    float     deviceTemperature;

    ChefZ_Sym liquidType;
    float     liquidQuantity;

    int       portionCapacity;

    //! Geraetebonus aus ChefZ_DeviceDef.qualityModifier. 1.0 = neutral.
    float     qualityModifier;

    //! Werkzeuggruppen, die in Reichweite sind. Nie null.
    ref array<ChefZ_Sym> availableToolGroups;

    /**
     * Wer handelt gerade an diesem Gefaess? 0 = niemand beteiligt.
     *
     * Ein Kochgeraet fuehrt keinen Besitzer, und ChefZ darf ihm keinen geben
     * (00 §5). Der Wert ist deshalb keine Eigenschaft des Gefaesses, sondern
     * eine Beobachtung der Kochsitzung; wer sie anstellt und nach welcher
     * Regel, steht im Kopf von ChefZ_CookActor. Diese Klasse NIMMT ihn
     * entgegen und faellt keine Entscheidung darueber - 1_Core kennt keine
     * Spieler.
     *
     * Gebraucht wird er fuer Faehigkeiten (17 §3.3) und fuer die Nutzlast der
     * Kochereignisse (17 §3.1). Ueber ChefZ_CapabilityGate wirkt er damit
     * auch auf die Bindung: ein Rezept mit requires[] und onFail "block"
     * faellt fuer den einen Koch weg und fuer den anderen nicht. Das ist
     * gewollt (08 §7 Schritt 2c) und vertraegt sich mit der Zusage aus 08 §7
     * ("zweimal dieselbe Eingabe ergibt zweimal dasselbe Ergebnis"), weil der
     * Spieler TEIL der Eingabe ist und sich nur aendert, wenn sich zugleich
     * der Inhalt aendert - und ein Inhaltswechsel loest ohnehin eine
     * vollstaendige Neubewertung aus.
     *
     * Bei 0 verhaelt sich der gesamte Pfad wie ohne Zuschreibung: das Tor
     * sperrt nichts (17 §3.3), Faehigkeiten liefern ihren Default, und ein
     * Fortschrittsempfaenger sieht "niemand beteiligt". Ein Kochvorgang ohne
     * Spieler in der Naehe laeuft vollstaendig durch.
     */
    int       actorIdentityId;

    //! Verstrichene Zeit der Kochsitzung, nur fuer completion TIMED (10 §6).
    float     elapsedSec;

    void ChefZ_CookContext()
    {
        deviceCategories    = new array<ChefZ_Sym>();
        availableToolGroups = new array<ChefZ_Sym>();
        Reset();
    }

    /**
     * Auf "kein Geraet, nichts bekannt" zuruecksetzen.
     *
     * Die beiden Listen werden GELEERT, nicht neu angelegt: der Kontext wird
     * pro Gefaess wiederverwendet (08 §7, "aus einem Pool"), und eine
     * Neuallokation je Tick je Feuerstelle waere genau die Sorte Kosten, die
     * dieser Entwurf vermeiden will.
     */
    void Reset()
    {
        deviceClass       = ChefZ_SymbolTable.INVALID;
        deviceRootClass   = ChefZ_SymbolTable.INVALID;
        deviceCategories.Clear();
        method            = ChefZ_SymbolTable.INVALID;
        deviceTemperature = 0.0;
        liquidType        = ChefZ_SymbolTable.INVALID;
        liquidQuantity    = 0.0;
        portionCapacity   = 0;
        qualityModifier   = 1.0;
        availableToolGroups.Clear();
        actorIdentityId   = 0;
        elapsedSec        = 0.0;
    }

    void AddDeviceCategory(ChefZ_Sym category)
    {
        if (!ChefZ_SymbolTable.IsValid(category))
            return;
        if (deviceCategories.Find(category) >= 0)
            return;
        deviceCategories.Insert(category);
    }

    void AddToolGroup(ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(group))
            return;
        if (availableToolGroups.Find(group) >= 0)
            return;
        availableToolGroups.Insert(group);
    }

    bool HasDeviceCategory(ChefZ_Sym category)
    {
        return deviceCategories.Find(category) >= 0;
    }

    bool HasToolGroup(ChefZ_Sym group)
    {
        return availableToolGroups.Find(group) >= 0;
    }

    //! Ist ueberhaupt Fluessigkeit da? "Menge > 0" und nicht "Typ gesetzt":
    //! Vanilla fuehrt Wasser im Topf als Quantity, und ein Typ ohne Menge ist
    //! ein leerer Topf, der sich erinnert.
    bool HasLiquid()
    {
        return liquidQuantity > 0.0;
    }

    string ToDebugString()
    {
        string s = ChefZ_SymbolTable.NameOrMark(deviceClass);

        if (ChefZ_SymbolTable.IsValid(deviceRootClass))
            s = s + " auf " + ChefZ_SymbolTable.Name(deviceRootClass);

        s = s + " [" + ChefZ_TextList.JoinSymbols(deviceCategories, ",") + "]" + " methode=" + ChefZ_SymbolTable.NameOrMark(method) + " temp=" + deviceTemperature.ToString();

        if (HasLiquid())
        {
            s = s + " fluessig=" + ChefZ_SymbolTable.NameOrMark(liquidType) + "/" + liquidQuantity.ToString();
        }
        if (elapsedSec > 0.0)
            s = s + " seit=" + elapsedSec.ToString() + "s";

        // Nur wenn es einen gibt: "koch=0" waere in jeder zweiten Zeile eines
        // Traces zu lesen und saehe aus wie ein Fehler, wo keiner ist.
        if (actorIdentityId != 0)
            s = s + " koch=" + actorIdentityId.ToString();

        return s;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_CookContext ctx = new ChefZ_CookContext();
        if (ChefZ_SymbolTable.IsValid(ctx.deviceClass))      return false;
        if (ctx.qualityModifier != 1.0)                      return false;
        if (ctx.HasLiquid())                                 return false;

        ChefZ_Sym kat = ChefZ_SymbolTable.Intern("CHEFZ_CC_GERAETEKAT");
        ctx.AddDeviceCategory(kat);
        ctx.AddDeviceCategory(kat);                         // keine Dublette
        if (ctx.deviceCategories.Count() != 1)               return false;
        if (!ctx.HasDeviceCategory(kat))                     return false;
        ctx.AddDeviceCategory(ChefZ_SymbolTable.INVALID);
        if (ctx.deviceCategories.Count() != 1)               return false;

        ctx.liquidQuantity = 2.0;
        if (!ctx.HasLiquid())                                return false;

        ctx.Reset();
        if (ctx.deviceCategories.Count() != 0)               return false;
        if (ctx.qualityModifier != 1.0)                      return false;

        return true;
    }
}
