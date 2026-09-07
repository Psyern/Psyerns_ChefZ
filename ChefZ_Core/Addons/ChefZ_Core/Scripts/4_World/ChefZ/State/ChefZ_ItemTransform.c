//==============================================================================
// ChefZ_ItemTransform - der Klassentausch, V1-Normalfall des Zustandswechsels
//
// Entwurf: 06 §4.4 (Signatur woertlich), 06 §2 ("Klassentausch ist der
// V1-Normalfall, die Zustandsvariable die Ausnahme"), 06 E2, V-B §1 Folge 2
// ("Swap als vollwertiger Pfad, nicht als Nebenweg; TransferFoodStage wird
// BENUTZT, nicht nachgebaut"), 08 §6 (Reihenfolge: erzeugen vor verbrauchen).
//
// ---------------------------------------------------------------------------
// Warum der Tausch der Normalfall ist
// ---------------------------------------------------------------------------
// Production Map §73 sieht fuer JEDEN V1-Konservierungszustand ohnehin eine
// eigene Klasse vor, und das aus einem harten Grund: Naehrwerte haengen in
// DayZ zwingend an der Klasse (01 V6). Geraeucherte Wurst MUSS eine eigene
// Klasse sein, wenn sie andere Naehrwerte haben soll - und das soll sie.
//
// ---------------------------------------------------------------------------
// Die Reihenfolge, und warum sie nicht verhandelbar ist
// ---------------------------------------------------------------------------
//   1. Zielklasse pruefen        -> Fehlschlag: Quelle unveraendert
//   2. Platz suchen              -> Fehlschlag: Quelle unveraendert
//   3. Ziel ERZEUGEN             -> Fehlschlag: Quelle unveraendert
//   4. Eigenschaften uebertragen -> Fehlschlag: Ziel wieder loeschen
//   5. Quelle loeschen           -> ZULETZT, ausnahmslos
//
// Dieselbe Regel wie im ChefZ_Applicator (08 §6): ein verlorenes Ergebnis ist
// ein Bugreport, ein verlorenes Ausgangsitem ist ein wuetender Spieler.
//
// ---------------------------------------------------------------------------
// Was Vanilla erledigt und hier NICHT nachgebaut wird
// ---------------------------------------------------------------------------
//   MiscGameplayFunctions.TransferItemProperties   Agenten, Item-Variablen
//                                                  (Temperatur, Nassheit,
//                                                  Sauberkeit, Menge), Health
//   Edible_Base.TransferFoodStage                  Garstufe, m_LastDecayStage,
//                                                  m_DecayTimer, m_DecayDelta
//
// Beide sind Vanilla-Routinen mit Vanilla-Eigenheiten. Sie nachzubauen hiesse,
// Vanilla-Verhalten zu erraten - und das Erratene faellt erst auf, wenn ein
// Update es aendert.
//
// Server. Ausschliesslich.
//
// KEIN CONTENT: kein Klassenname steht in dieser Datei. "newClass" kommt aus
// einem ChefZ_TransformDef, also aus Daten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ItemTransform
{
    //! Zaehler fuer "chefz stats" (18 §2).
    private static int s_CountSwapped;
    private static int s_CountFailed;

    //! Nur fuer Tests: unterdrueckt die Meldungen dieser Klasse. Dieselbe
    //! Loesung und derselbe Grund wie im ChefZ_Applicator - ein Test, der
    //! Fehlerfaelle durchspielt, darf ChefZ_Log.GetErrorCount() nicht fuellen
    //! und damit den Server Richtung SAFE_MODE druecken (18 §4).
    private static bool s_QuietForTest;

    //==========================================================================

    /**
     * Tauscht ein Item gegen eine andere Klasse - ganz oder gar nicht.
     *
     * Uebertragen werden: Zustand, Qualitaet, Frische, Portionen und
     * Behaelterbindung (ChefZ), Garstufe und Verfallsdaten (Vanilla,
     * TransferFoodStage), Temperatur, Nassheit, Sauberkeit, Agenten und Health
     * (Vanilla, TransferItemProperties) sowie der Mengen-ANTEIL.
     *
     * @param source          das Ausgangsitem. Bleibt bei jedem Fehlschlag
     *                        vollstaendig unveraendert.
     * @param newClass        die Zielklasse.
     * @param freshnessCarry  Faktor auf die uebernommene Frische. Negativ
     *                        heisst "unveraendert uebernehmen".
     * @param err             Klartextgrund bei null. Nie leer, wenn null.
     *
     * @return das neue Item, oder null. Bei null ist NICHTS geschehen.
     */
    static ItemBase Swap(notnull ItemBase source, string newClass, float freshnessCarry, out string err)
    {
        err = "";

        if (!g_Game || !g_Game.IsServer())
        {
            err = "Klassentausch ist serverseitig - clientseitig gibt es keinen Ersatzweg";
            Fail(err, newClass);
            return null;
        }

        // --- 1. Zielklasse -------------------------------------------------
        if (newClass == "")
        {
            err = "keine Zielklasse angegeben";
            Fail(err, newClass);
            return null;
        }

        if (!ClassExists(newClass))
        {
            // 06 §7: ERROR mit Zielklasse. Der Build hat den Transform bereits
            // abgewiesen - die Laufzeit erreicht diese Zeile normalerweise
            // nicht. Wenn doch, ist es keine Kulanz wert.
            err = "die Zielklasse \"" + newClass + "\" existiert in keiner geladenen Config";
            Fail(err, newClass);
            return null;
        }

        // --- 2. und 3. Platz suchen und erzeugen ---------------------------
        float quantityRatio = QuantityRatio(source);

        ItemBase created = Create(source, newClass, err);
        if (!created)
        {
            Fail(err, newClass);
            return null;
        }

        // --- 4. Eigenschaften ----------------------------------------------
        if (!Carry(source, created, freshnessCarry, quantityRatio, err))
        {
            // Das Ziel ist entstanden, aber unbrauchbar. Es wird wieder
            // entfernt; die Quelle ist noch da, es geht nichts verloren.
            created.Delete();
            Fail(err, newClass);
            return null;
        }

        // --- 5. Quelle loeschen - ZULETZT ----------------------------------
        source.Delete();
        s_CountSwapped++;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.STATE, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.STATE, "Klassentausch: " + source.GetType() + " -> " + newClass + "  mengenanteil=" + quantityRatio.ToString() + "  frischefaktor=" + freshnessCarry.ToString());

        return created;
    }

    //==========================================================================
    // Schritt 3 - Erzeugen
    //==========================================================================

    /**
     * Erzeugt das Ziel moeglichst dort, wo die Quelle liegt.
     *
     * Zwei Wege, in dieser Reihenfolge:
     *   a) die Quelle haengt in einem Inventar -> freier Cargoplatz DESSELBEN
     *      Elternteils
     *   b) sonst -> Boden an der Position der Quelle
     *
     * Warum nicht exakt dieselbe Zelle: sie ist besetzt - von der Quelle, die
     * es ja noch gibt und die es noch geben MUSS, bis das Ziel steht. Der
     * Umweg ueber "irgendein freier Platz im selben Behaelter" ist der Preis
     * dafuer, dass ein Fehlschlag nichts kostet. Vanillas eigener Weg
     * (ReplaceItemWithNewLambdaBase) loest das ueber eine Reservierung im
     * Inventar des SPIELERS - er braucht dafuer aber einen Spieler
     * (MiscGameplayFunctions.c:454), und ein Item im Kochtopf hat keinen.
     */
    private static ItemBase Create(notnull ItemBase source, string newClass, out string err)
    {
        err = "";

        EntityAI parent = source.GetHierarchyParent();
        if (parent && parent.GetInventory())
        {
            InventoryLocation loc = new InventoryLocation();
            if (parent.GetInventory().FindFirstFreeLocationForNewEntity( newClass, FindInventoryLocationType.CARGO, loc))
            {
                EntityAI spawned = parent.GetInventory().CreateEntityInCargoEx( newClass, loc.GetIdx(), loc.GetRow(), loc.GetCol(), loc.GetFlip());

                ItemBase item = ItemBase.Cast(spawned);
                if (item)
                    return item;

                if (spawned)
                {
                    // Entstanden, aber unbrauchbar. Sofort wieder weg - ein
                    // herrenloses Objekt im Behaelter waere schlimmer als
                    // keines.
                    spawned.Delete();
                    err = "die Zielklasse \"" + newClass + "\" ist kein ItemBase";
                    return null;
                }
            }

            err = "im Behaelter ist kein Platz fuer \"" + newClass + "\"";
            return null;
        }

        Object obj = g_Game.CreateObjectEx(newClass, source.GetPosition(), ECE_PLACE_ON_SURFACE);
        ItemBase onGround = ItemBase.Cast(obj);
        if (onGround)
            return onGround;

        if (obj)
            g_Game.ObjectDelete(obj);

        err = "die Engine hat \"" + newClass + "\" nicht erzeugt";
        return null;
    }

    //==========================================================================
    // Schritt 4 - Eigenschaften uebertragen
    //==========================================================================

    /**
     * @return false, wenn das Ziel dabei geloescht wurde oder verschwunden
     *         ist. Dann rollt der Aufrufer zurueck - die Quelle lebt noch.
     */
    private static bool Carry(notnull ItemBase source, notnull ItemBase target, float freshnessCarry, float quantityRatio, out string err)
    {
        err = "";

        // 1. Vanilla-Eigenschaften: Agenten, Item-Variablen, Health.
        //    excludeQuantity = true, weil die Menge gleich als ANTEIL gesetzt
        //    wird: die Quelle kann eine andere Hoechstmenge haben als das Ziel,
        //    und eine roh kopierte Zahl waere dann entweder eine Aufwertung
        //    oder ein Verlust.
        MiscGameplayFunctions.TransferItemProperties(source, target, true, true, true, true);

        // 2. Garstufe und Verfallsdaten. TransferFoodStage uebertraegt
        //    m_LastDecayStage, m_DecayTimer und m_DecayDelta und ruft
        //    ChangeFoodStage - genau das, was ein Tausch braucht
        //    (Edible_Base.c:619). Nachbauen waere Raten.
        Edible_Base srcEdible = Edible_Base.Cast(source);
        Edible_Base dstEdible = Edible_Base.Cast(target);
        if (srcEdible && dstEdible && srcEdible.GetFoodStage() && dstEdible.GetFoodStage())
            dstEdible.TransferFoodStage(srcEdible);

        // 3. Der ChefZ-Block: Zustand, Qualitaet, Frische, Portionen,
        //    Behaelterbindung. Traegt eine der beiden Seiten keinen Block
        //    (z.B. Tausch von einer Vanilla-Klasse in eine ChefZ-Klasse), ist
        //    das kein Fehler - dann gibt es eben nichts zu uebernehmen und der
        //    Zustand ergibt sich aus dem defaultState der Zielklasse
        //    (06 §3, Schritt 2; das ist der V1-Normalfall).
        ChefZ_ItemStateComponent.InheritFrom(target, source, freshnessCarry);

        // 4. Mengenanteil.
        if (target.IsSetForDeletion())
        {
            err = "das erzeugte Item wurde beim Uebertragen der Eigenschaften geloescht";
            return false;
        }

        if (quantityRatio >= 0.0 && target.HasQuantity())
        {
            // SetQuantityNormalized klemmt selbst und loescht das Item, wenn
            // der Wert das Minimum erreicht und die Klasse
            // varQuantityDestroyOnMin fuehrt (ItemBase.c:3431). Deshalb wird
            // danach geprueft, ob das Ziel ueberhaupt noch da ist.
            target.SetQuantityNormalized(quantityRatio);

            if (target.IsSetForDeletion())
            {
                err = "das erzeugte Item wurde beim Setzen der Menge geloescht - der " + "Mengenanteil der Quelle liegt unter dem Minimum der Zielklasse";
                return false;
            }
        }

        target.SetSynchDirty();
        return true;
    }

    //! Mengenanteil 0..1, oder -1 wenn die Quelle keine Menge fuehrt.
    private static float QuantityRatio(notnull ItemBase source)
    {
        if (!source.HasQuantity())
            return -1.0;

        float max = source.GetQuantityMax();
        if (max <= 0.0)
            return -1.0;

        return Math.Clamp(source.GetQuantity() / max, 0.0, 1.0);
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    //! Dieselbe Pruefung wie im ChefZ_IngredientManager: eine Klasse kann in
    //! CfgVehicles, CfgWeapons oder CfgMagazines stehen.
    private static bool ClassExists(string cls)
    {
        if (!g_Game)
            return false;
        if (g_Game.ConfigIsExisting("CfgVehicles "  + cls))  return true;
        if (g_Game.ConfigIsExisting("CfgWeapons "   + cls))  return true;
        if (g_Game.ConfigIsExisting("CfgMagazines " + cls))  return true;
        return false;
    }

    private static void Fail(string reason, string newClass)
    {
        s_CountFailed++;
        if (s_QuietForTest)
            return;

        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.STATE, "transform.swap." + newClass + "." + reason, "Klassentausch nach \"" + newClass + "\" abgebrochen: " + reason + ". Das Ausgangsitem bleibt unveraendert - es geht nichts verloren.");
    }

    //--------------------------------------------------------------------------

    static int GetSwappedCount() { return s_CountSwapped; }
    static int GetFailedCount()  { return s_CountFailed; }

    static void ResetCounters()
    {
        s_CountSwapped = 0;
        s_CountFailed  = 0;
    }

    static void SetQuietForTest(bool quiet)
    {
        s_QuietForTest = quiet;
    }
}
