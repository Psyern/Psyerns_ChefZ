//==============================================================================
// modded class Cooking - der EINZIGE Eingriff des Core in den Kochpfad
//
// Entwurf: 10 §3 (der Hook, woertlich), 10 E1 (Post-Hook statt Pre-Hook oder
// Ersatz), 10 E2 (Cooking statt FireplaceBase), 10 E3 (Methode erfragen statt
// nachbilden), 10 §8 (Zeile "Anderer Mod moddet Cooking ebenfalls"),
// 01 V1, 01 V2, 01 V11, 19 S7.
//
//==============================================================================
// BEGRUENDUNG DIESES OVERRIDES
//==============================================================================
// WARUM UEBERHAUPT: ChefZ muss erfahren, wann in einem Gefaess gekocht wird.
// Es gibt keinen Ereigniskanal dafuer.
//
// WARUM Cooking UND NICHT FireplaceBase (10 E2, 01 V1): Cooking.
// CookWithEquipment ist der gemeinsame Trichter ALLER Wege, auf denen in einem
// Gefaess gekocht wird - Feuerstelle, Fass, Gasherd, Kochstaender,
// Direct-Cooking-Slots. Ein Hook, alle Geraete. Ein Hook in FireplaceBase
// verpasste den Gasherd, und jedes neue Vanilla-Kochgeraet in einem
// DayZ-Update braeuchte eine Core-Aenderung.
//
// Zusaetzlich ist FireplaceBase der Ort, an dem sich ChefZ mit JEDEM anderen
// Feuerstellen-Mod ins Gehege kaeme. Ein Cooking-Override ist seltener und die
// Kollisionsflaeche entsprechend kleiner.
//
// WARUM NICHT CookOnStick UND SmokeItem (10 §3, 01 V14): beide arbeiten auf
// einem einzelnen Edible_Base OHNE Gefaess; ChefZ-Rezepte sind gefaessbasiert.
// Vanilla-Raeuchern im Fass bleibt exakt Vanilla-Raeuchern.
//
//==============================================================================
// DIE DREI EIGENSCHAFTEN, DIE INVARIANTE I1 ERGEBEN (10 §3)
//==============================================================================
// Sie sind nicht verhandelbar. Wer diese Datei anfasst, prueft sie einzeln.
//
//   1. super wird BEDINGUNGSLOS und als ERSTE ANWEISUNG aufgerufen. Es gibt
//      keine Bedingung davor und keinen Codepfad, der ihn ueberspringt.
//
//   2. Der Rueckgabewert bleibt der von Vanilla, unveraendert. Er ist
//      Vanilla-Interna (01 V2: der Kommentar im Original stimmt nicht, 0 kommt
//      nur bei null und IsRuined) und darf sich jederzeit aendern.
//
//   3. Der ChefZ-Teil hat keinen Rueckkanal. AfterVanillaCook gibt nichts
//      zurueck. Es gibt keine Referenz, mit der sich der bereits gelaufene
//      Tick rueckgaengig machen liesse.
//
// Daraus folgt die Aussage, um die es in Gate 1 geht, und sie ist STRUKTURELL
// wahr statt nur beabsichtigt: faellt der gesamte ChefZ-Teil aus - Config
// kaputt, Rezepte leer, Ausnahme mitten in der Auswertung -, ist der Tick fuer
// den Spieler ununterscheidbar von einem Server ohne ChefZ. Vanilla ist zu
// diesem Zeitpunkt bereits vollstaendig gelaufen.
//
//==============================================================================
// VERTRAEGLICHKEIT MIT ANDEREN MODS (10 §8)
//==============================================================================
// UpdateCookingState, ProcessItemToCook und GetItemTypeFromCargo (26.10.2026)
// greifen nur bei einem
// gebundenen ChefZ-Rezept und rufen sonst super unveraendert.
//
// modded class kettet. Unser super-Aufruf ist die erste Anweisung, und wir
// geben dessen Rueckgabewert zurueck - das vertraeglichste moegliche Verhalten.
// Ruft ein anderer Mod in seiner Kette super nicht, bricht er ohnehin Vanilla;
// dann greift unser Hook nicht mehr, was das sichere Ergebnis ist.
//
// Layer: 4_World.
//==============================================================================

// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// I4-BELEG: der Fremdmodname unten ist das Ergebnis der Kollisionspruefung -
// welche Methoden ein Nachbarmod auf derselben Klasse anfasst. Kein Aufruf,
// keine Abhaengigkeit; der Core kennt keine Klasse dieses Mods.
// super.CookWithEquipment unbedingt und als erste Anweisung, Rueckgabewert
// unveraendert weitergereicht - Regel 3 ist damit strukturell und nicht
// bloss beabsichtigt. Terje moddet UpdateCookingStateOnStick und
// ProcessItemToCook, keine Ueberschneidung.
modded class Cooking
{
    override int CookWithEquipment(ItemBase cooking_equipment, float cooking_time_coef = 1)
    {
        // ---- 1) VANILLA - immer, zuerst, ohne jede Vorbedingung ------------
        int vanillaResult = super.CookWithEquipment(cooking_equipment, cooking_time_coef);

        // ---- 2) ChefZ - rein beobachtend, ohne Rueckkanal ------------------
        //
        // ShouldObserve ist Stufe 0 (10 §5) und steht VOR der Frage nach der
        // Kochmethode. Grund: GetCookingMethodWithTimeOverride laeuft im
        // Zweifel durchs ganze Cargo (Cooking.c:445) und legt ein Param2 an.
        // 19 S7 verlangt "bei leerem Rezeptbestand kostet der Hook einen
        // Bool-Test" - mit dem Aufruf in der Argumentliste waere es ein
        // Cargo-Durchlauf je Tick je Feuerstelle auf der ganzen Karte.
        //
        // m_UpdateTime und GetCookingMethodWithTimeOverride sind protected und
        // genau hier - und nur hier - erreichbar. Beide werden ERFRAGT und
        // durchgereicht, nicht nachgebaut: 01 V11 zeigt, dass ein Nachbau
        // subtil falsch waere. Die Methode kippt von BOILING auf BAKING,
        // sobald das Wasser verdampft ist, und Vanilla refresht sie mitten im
        // eigenen Ablauf.
        if (ChefZ_CookingHook.ShouldObserve(cooking_equipment))
        {
            ChefZ_CookingHook.AfterVanillaCook(cooking_equipment, cooking_time_coef, m_UpdateTime, GetCookingMethodWithTimeOverride(cooking_equipment));
        }

        // ---- 3) Vanilla-Rueckgabewert unveraendert weiterreichen -----------
        return vanillaResult;
    }

    /**
     * Garhalten fuer gebundene ChefZ-Rezepte (26.10.2026).
     *
     * Die einzige Stelle, an der ChefZ Vanillas Garen beeinflusst, und sie ist
     * eng: nur ein Item, das in einem Gefaess mit GEBUNDENEM ON_STAGE-Rezept
     * schon in dessen Endstufe steht, gart nicht weiter (es bleibt warm). So
     * werden Zutaten mit verschiedenen Garzeiten gemeinsam fertig, statt dass
     * die schnellen verbrennen, waehrend die langsamen noch roh sind
     * (Begruendung an ChefZ_CookingDeviceAdapter.HoldsDoneStage). Ohne Treffer
     * - und damit ohne ChefZ-Daten oder bei einem Fehler im Matcher - laeuft
     * Vanilla unveraendert.
     */
    override protected int UpdateCookingState(Edible_Base item_to_cook, CookingMethodType cooking_method, ItemBase cooking_equipment, float cooking_time_coef)
    {
        if (cooking_equipment && cooking_equipment != item_to_cook && ChefZ_CookingDeviceAdapter.Get().HoldsDoneStage(cooking_equipment, item_to_cook))
        {
            AddTemperatureToItem(item_to_cook, cooking_equipment, 0);
            return 0;
        }

        return super.UpdateCookingState(item_to_cook, cooking_method, cooking_equipment, cooking_time_coef);
    }

    /**
     * Brandschutz fuer Zutaten ohne FoodStage in einem gebundenen Gefaess
     * (26.10.2026, Begruendung an ChefZ_CookingDeviceAdapter.ProtectsContents).
     *
     * Eng wie das Garhalten darueber: nur Items, die Vanilla NICHT garen kann,
     * nur ausserhalb des Gefaesses selbst und nur mit gebundenem Rezept. Sie
     * werden gewaermt wie bei Vanilla, verlieren aber keine Lebenspunkte und
     * kein Volumen. Alles andere laeuft unveraendert durch Vanilla.
     */
    override void ProcessItemToCook(notnull ItemBase pItem, ItemBase cookingEquip, Param2<CookingMethodType, float> pCookingMethod, out Param2<bool, bool> pStateFlags)
    {
        if (cookingEquip && pItem != cookingEquip)
        {
            Edible_Base edible = Edible_Base.Cast(pItem);
            bool cookable = false;
            if (edible && edible.CanBeCooked())
                cookable = true;
            if (!cookable && ChefZ_CookingDeviceAdapter.Get().ProtectsContents(cookingEquip))
            {
                AddTemperatureToItem(pItem, null, 0);
                return;
            }
        }

        super.ProcessItemToCook(pItem, cookingEquip, pCookingMethod, pStateFlags);
    }

    /**
     * Bratfett in einem gebundenen Gefaess (26.10.2026).
     *
     * Vanilla zieht dem Fett in der Pfanne je Stufenwechsel eines anderen
     * Items 25 Einheiten ab (UpdateCookingState, COOKING_LARD_DECREASE_COEF).
     * Die Suche vergleicht Type(), und Klassen ohne eigene Skriptklasse -
     * ChefZ_Butter - tragen die von Lard. Drei Pilze verbrauchten damit die
     * ganze Butter der Pilzpfanne, die Bindung fiel, das Gericht kam nie
     * (Autotest). Ist ein Rezept gebunden, gibt Vanilla das Fett deshalb nur
     * heraus, solange danach ein Rest bleibt; sonst traegt das gegarte Item
     * den Verlust selbst, genau wie Vanilla ohne Fett.
     */
    override protected ItemBase GetItemTypeFromCargo(typename item_type, ItemBase cooking_equipment)
    {
        ItemBase found = super.GetItemTypeFromCargo(item_type, cooking_equipment);
        if (!found || item_type != COOKING_INGREDIENT_LARD)
            return found;

        if (found.GetQuantity() > COOKING_LARD_DECREASE_COEF)
            return found;

        if (ChefZ_CookingDeviceAdapter.Get().ProtectsContents(cooking_equipment))
            return null;

        return found;
    }

    /**
     * S18: dieselbe Methodenauskunft fuer die DIAGNOSE.
     *
     * ADDITIV und ausdruecklich KEIN override. Die Kollisionsflaeche dieser
     * modded class bleibt damit CookWithEquipment, UpdateCookingState, ProcessItemToCook und GetItemTypeFromCargo
     * darueber - eine hinzugefuegte Methode kann keine fremde ueberschreiben.
     *
     * Warum sie noetig ist: GetCookingMethodWithTimeOverride ist protected und
     * NUR aus einer Cooking-Ableitung erreichbar. "chefz match" muss dieselbe
     * Methode sehen wie der Kochtick, sonst beantwortet die Diagnose eine
     * andere Frage als die gestellte - und ein Nachbau waere subtil falsch
     * (01 V11: die Methode kippt von BOILING auf BAKING, sobald das Wasser
     * verdampft ist).
     *
     * Rein lesend. Vanillas Funktion prueft Quantity, Fluessigkeitstyp und
     * Cargo und legt ein Param2 an; sie veraendert nichts (Cooking.c:430).
     * Damit bleibt die Zusage aus 18 E6 - "chefz match ist
     * nebenwirkungsfrei" - auch an dieser Stelle gewahrt.
     */
    int ChefZ_QueryMethod(ItemBase cooking_equipment)
    {
        if (!cooking_equipment)
            return CookingMethodType.NONE;

        Param2<CookingMethodType, float> method = GetCookingMethodWithTimeOverride(cooking_equipment);
        if (!method)
            return CookingMethodType.NONE;

        return method.param1;
    }
}
