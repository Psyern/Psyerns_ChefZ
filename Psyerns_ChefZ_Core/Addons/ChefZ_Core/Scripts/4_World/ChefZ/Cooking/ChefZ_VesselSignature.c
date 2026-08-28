//==============================================================================
// ChefZ_VesselSignature - der billige Aenderungsdetektor
//
// Entwurf: 10 §4 (Feldliste woertlich), 10 §5 Stufe A, 10 E4 (warum
// Hash-Signatur statt Sequenzvergleich), 10 E5 (Fingerprint gegen
// Zeitdiebstahl), 10 §8 (der Preis einer Kollision), 01 V2 (Vanillas
// Rueckgabewert taugt NICHT als Aenderungssignal), 01 V13 (Pot und Cauldron
// sind selbst Edible_Base).
//
// ---------------------------------------------------------------------------
// Wozu
// ---------------------------------------------------------------------------
// Cooking.CookWithEquipment laeuft pro Tick fuer jede brennende Feuerstelle
// auf der Karte. Ein Vollmatch je Tick waere bei Dutzenden Feuerstellen mal
// ueber hundert Rezepten messbar. Die Signatur beantwortet die Frage
//
//     "hat sich im Topf seit dem letzten Mal ueberhaupt etwas geaendert?"
//
// in einem Cargo-Durchlauf und ohne einen einzigen Registry-Zugriff.
//
// ---------------------------------------------------------------------------
// Die Regel dieser Datei: KEIN Registry-Zugriff
// ---------------------------------------------------------------------------
// 10 §5 begruendet das ausdruecklich: die Drosselung darf nicht davon
// abhaengen, ob Kategorien, Zutaten oder Rezepte geladen sind. Klassenhash,
// Anzahl, FoodStage, Fluessigkeitstyp und Menge kommen direkt vom Item.
// Damit funktioniert die Drosselung auch bei degradierter Konfiguration -
// und genau dann ist sie am wichtigsten.
//
// Kein ChefZ_SymbolTable.Intern(), kein ChefZ_IngredientManager, kein
// ChefZ_CategoryManager. Wer hier einen davon einbaut, hebt die Eigenschaft
// auf, ohne dass ein Test es merkt.
//
// ---------------------------------------------------------------------------
// Warum Summe UND XOR
// ---------------------------------------------------------------------------
// Die Summe allein verwechselt {A,A,B,B} mit {A,B,A,B} nicht, wohl aber
// {A,C} mit {B,B}, wenn hash(A)+hash(C) == 2*hash(B). Das XOR faengt genau
// diese Faelle ab, weil es sich voellig anders verhaelt: gleiche Elemente
// loeschen sich aus. Zusammen mit Anzahl, FoodStage-Maske, Zustandsmaske,
// Fluessigkeitstyp, Mengenstufe und Kochmethode ist eine Kollision praktisch
// unwahrscheinlich.
//
// Und der Preis einer Kollision ist klein (10 §8): ein verpasster Rematch,
// der beim naechsten echten Wechsel nachgeholt wird. Kein Datenverlust, kein
// falsches Gericht. Diese Asymmetrie ist der Grund, warum ein Hash hier
// zulaessig ist und im Persistenzpfad nicht (03 E5).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_VesselSignature : Managed
{
    /**
     * Stufenbreite der Mengenerfassung.
     *
     * KEINE Einstellung, sondern eine Aufloesung: sie bestimmt nicht, WAS das
     * System tut, sondern nur, wie oft es sich die Frage neu stellt. Der Wert
     * ist an Vanilla angelehnt - Cooking.LIQUID_VAPOR_QUANTITY ist 2, also
     * verliert ein siedender Topf 2 Einheiten pro Tick. Mit Stufe 25 wechselt
     * die Signatur dadurch etwa alle zwoelf Ticks statt bei jedem einzelnen,
     * und ein Rematch nur wegen Verdunstung findet nicht statt.
     *
     * Zu klein: Vollmatch bei jedem Tick eines siedenden Topfes.
     * Zu gross:  eine Fluessigkeitsbedingung eines Rezepts greift verspaetet.
     */
    static const float QUANTITY_BUCKET = 25.0;

    //! Nur die sechs Vanilla-Stufen plus NONE passen in die Maske
    //! (01 V4: FoodStageType ist nicht erweiterbar, COUNT == 7).
    static const int FOODSTAGE_BITS = 7;

    int itemCount;
    int typeHashSum;        //! Summe der Klassenhashes
    int typeHashXor;        //! XOR, faengt Vertauschungen und Summenkollisionen ab
    int foodStageMask;      //! Bit je vorkommendem FoodStageType
    int chefzStateMask;     //! Bit je vorkommendem ChefZ-Sync-Ordinal (mod 32)
    int liquidType;
    int quantityBucket;     //! GetQuantity() des Gefaesses in groben Stufen
    int methodType;         //! CookingMethodType dieses Ticks

    void ChefZ_VesselSignature()
    {
        Reset();
    }

    /**
     * "Noch nie gemessen".
     *
     * itemCount = -1 und nicht 0: ein leeres Gefaess ist ein gueltiger,
     * messbarer Zustand mit itemCount 0. Waere der Startwert 0, saehe eine
     * frische Sitzung an einem leeren Topf so aus, als haette sie ihn bereits
     * gemessen - und der erste Inhalt einer Auswertung entginge der Stufe B.
     */
    void Reset()
    {
        itemCount      = -1;
        typeHashSum    = 0;
        typeHashXor    = 0;
        foodStageMask  = 0;
        chefzStateMask = 0;
        liquidType     = LIQUID_NONE;
        quantityBucket = 0;
        methodType     = CookingMethodType.NONE;
    }

    bool IsMeasured()
    {
        return itemCount >= 0;
    }

    bool Equals(notnull ChefZ_VesselSignature other)
    {
        // Reihenfolge nach Trennschaerfe: die Anzahl unterscheidet die meisten
        // Faelle und kostet einen Vergleich.
        if (itemCount      != other.itemCount)      return false;
        if (typeHashSum    != other.typeHashSum)    return false;
        if (typeHashXor    != other.typeHashXor)    return false;
        if (foodStageMask  != other.foodStageMask)  return false;
        if (chefzStateMask != other.chefzStateMask) return false;
        if (liquidType     != other.liquidType)     return false;
        if (quantityBucket != other.quantityBucket) return false;
        if (methodType     != other.methodType)     return false;
        return true;
    }

    void CopyFrom(notnull ChefZ_VesselSignature other)
    {
        itemCount      = other.itemCount;
        typeHashSum    = other.typeHashSum;
        typeHashXor    = other.typeHashXor;
        foodStageMask  = other.foodStageMask;
        chefzStateMask = other.chefzStateMask;
        liquidType     = other.liquidType;
        quantityBucket = other.quantityBucket;
        methodType     = other.methodType;
    }

    //==========================================================================
    // Aufbau
    //==========================================================================

    void BeginMeasure(int cookingMethod, int vesselLiquidType, float vesselQuantity)
    {
        itemCount      = 0;
        typeHashSum    = 0;
        typeHashXor    = 0;
        foodStageMask  = 0;
        chefzStateMask = 0;
        liquidType     = vesselLiquidType;
        quantityBucket = BucketOf(vesselQuantity);
        methodType     = cookingMethod;
    }

    /**
     * Ein Cargo-Item einrechnen.
     *
     * @param typeHash    className.Hash()
     * @param foodStage   FoodStageType als int, -1 wenn das Item keine
     *                    FoodStage hat (die grosse Mehrheit)
     * @param stateOrdinal ChefZ-Sync-Ordinal des Zustands, -1 wenn keiner.
     *                    Bleibt bis S9 (Food State System) durchgehend -1;
     *                    dann traegt ChefZ_Edible_Base den Wert und die Maske
     *                    faengt Zustandswechsel ohne FoodStage-Wechsel ab
     *                    (etwa SALTED -> DRIED an einer Station).
     */
    void AddItem(int typeHash, int foodStage, int stateOrdinal)
    {
        itemCount++;

        typeHashSum = typeHashSum + typeHash;
        typeHashXor = typeHashXor ^ typeHash;

        if (foodStage >= 0 && foodStage < FOODSTAGE_BITS)
            foodStageMask = foodStageMask | (1 << foodStage);

        // mod 32, weil eine int-Maske nicht mehr Bits hat. Zwei Zustaende, die
        // sich um genau 32 Ordinale unterscheiden, liegen damit auf demselben
        // Bit - eine Kollision mit dem bekannten, kleinen Preis (siehe Kopf).
        if (stateOrdinal >= 0)
            chefzStateMask = chefzStateMask | (1 << (stateOrdinal & 31));
    }

    static int BucketOf(float quantity)
    {
        if (quantity <= 0.0)
            return 0;
        return (int)(quantity / QUANTITY_BUCKET);
    }

    //==========================================================================
    // Fingerprint (10 E5)
    //==========================================================================

    /**
     * Stabiler Hash ueber den INHALT, ohne Kochmethode und ohne Mengenstufe.
     *
     * Er beantwortet eine engere Frage als Equals(): "sind es noch dieselben
     * Zutaten?" Bei completion TIMED laeuft eine eigene Uhr, und ohne diese
     * Pruefung koennte man kurz vor Ende die Zutaten tauschen und die
     * aufgelaufene Zeit erben (10 E5).
     *
     * Kochmethode und Mengenstufe sind bewusst NICHT enthalten: beide aendern
     * sich waehrend eines normalen Kochvorgangs von selbst (die Methode kippt
     * von BOILING auf BAKING, sobald das Wasser verdampft ist - 01 V11), und
     * dafuer soll niemand seine Kochzeit verlieren.
     */
    int ToFingerprint()
    {
        int h = 17;
        h = h * 31 + itemCount;
        h = h * 31 + typeHashSum;
        h = h * 31 + typeHashXor;
        h = h * 31 + foodStageMask;
        h = h * 31 + chefzStateMask;
        h = h * 31 + liquidType;
        return h;
    }

    //==========================================================================

    string ToDebugString()
    {
        if (!IsMeasured())
            return "(nie gemessen)";

        string chefzTxt1 = "items=" + itemCount.ToString() + " sum=" + typeHashSum.ToString() + " xor=";
        chefzTxt1 = chefzTxt1 + typeHashXor.ToString() + " stages=" + foodStageMask.ToString() + " states=" + chefzStateMask.ToString();
        chefzTxt1 = chefzTxt1 + " liquid=" + liquidType.ToString() + " qty~" + quantityBucket.ToString() + " methode=";
        chefzTxt1 = chefzTxt1 + methodType.ToString();
        return chefzTxt1;
    }

    //==========================================================================

    /**
     * Nur fuer den Selbsttest - laeuft ohne Item, ohne Gefaess, ohne Spiel.
     *
     * Die drei Faelle, die zaehlen: Vertauschung wird NICHT als Aenderung
     * gelesen (dieselbe Menge Zutaten ist derselbe Topf), ein Austausch sehr
     * wohl, und der Fingerprint ignoriert die Kochmethode.
     */
    static bool SelfCheck()
    {
        ChefZ_VesselSignature a = new ChefZ_VesselSignature();
        if (a.IsMeasured())                                     return false;

        ChefZ_VesselSignature b = new ChefZ_VesselSignature();
        if (a.Equals(b) != true)                                return false;   // beide ungemessen

        a.BeginMeasure(2, 1, 480.0);
        a.AddItem(1001, 1, -1);
        a.AddItem(2002, 3, -1);
        if (!a.IsMeasured())                                    return false;
        if (a.itemCount != 2)                                   return false;

        // 1. Andere Reihenfolge, gleicher Inhalt -> gleiche Signatur.
        b.BeginMeasure(2, 1, 480.0);
        b.AddItem(2002, 3, -1);
        b.AddItem(1001, 1, -1);
        if (!a.Equals(b))                                       return false;

        // 2. Eine Zutat ausgetauscht -> andere Signatur.
        ChefZ_VesselSignature c = new ChefZ_VesselSignature();
        c.BeginMeasure(2, 1, 480.0);
        c.AddItem(1001, 1, -1);
        c.AddItem(3003, 3, -1);
        if (a.Equals(c))                                        return false;

        // 3. Nur die FoodStage gewechselt -> andere Signatur.
        ChefZ_VesselSignature d = new ChefZ_VesselSignature();
        d.BeginMeasure(2, 1, 480.0);
        d.AddItem(1001, 3, -1);
        d.AddItem(2002, 3, -1);
        if (a.Equals(d))                                        return false;

        // 4. Nur die Kochmethode gewechselt -> andere Signatur, GLEICHER
        //    Fingerprint (10 E5: Methodenwechsel darf keine Kochzeit kosten).
        ChefZ_VesselSignature e = new ChefZ_VesselSignature();
        e.BeginMeasure(1, 1, 480.0);
        e.AddItem(1001, 1, -1);
        e.AddItem(2002, 3, -1);
        if (a.Equals(e))                                        return false;
        if (a.ToFingerprint() != e.ToFingerprint())             return false;

        // 5. Verdunstung innerhalb einer Stufe aendert nichts.
        ChefZ_VesselSignature f = new ChefZ_VesselSignature();
        f.BeginMeasure(2, 1, 478.0);
        f.AddItem(1001, 1, -1);
        f.AddItem(2002, 3, -1);
        if (!a.Equals(f))                                       return false;

        // 6. CopyFrom macht zwei gleiche, aber getrennte Signaturen.
        ChefZ_VesselSignature g = new ChefZ_VesselSignature();
        g.CopyFrom(c);
        if (!g.Equals(c))                                       return false;
        c.AddItem(4004, -1, -1);
        if (g.Equals(c))                                        return false;

        // 7. Zustandsmaske trennt, auch wenn sonst alles gleich ist (S9).
        ChefZ_VesselSignature h = new ChefZ_VesselSignature();
        h.BeginMeasure(2, 1, 480.0);
        h.AddItem(1001, 1, 3);
        h.AddItem(2002, 3, -1);
        if (a.Equals(h))                                        return false;

        // 8. Reset macht wieder "nie gemessen".
        h.Reset();
        if (h.IsMeasured())                                     return false;

        return true;
    }
}
