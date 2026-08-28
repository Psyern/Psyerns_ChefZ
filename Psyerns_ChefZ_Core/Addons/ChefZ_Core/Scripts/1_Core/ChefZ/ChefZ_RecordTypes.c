//==============================================================================
// ChefZ_*Def - die Record-Arten als Huellen
//
// Entwurf: 02 §5.1 (Feldliste), 02 §4 (die Felder, die als Klassenbaum aus
// Rang 1 kommen), 19 S2 ("alle Record-Arten als leere Huellen").
//
// ABSICHTLICH DUENN. Jede Art traegt genau die Felder, die 02 nennt - nicht
// mehr. Die fachlichen Felder gehoeren den spaeteren Schritten:
//
//   ChefZ_StateDef        06 / S9 (eigene Datei)  ChefZ_QualityTierDef 12 / S12
//   ChefZ_IngredientDef   05 / S4        ChefZ_NutritionDef    13 / S12 (eigene Datei)
//   ChefZ_ProcessDef      11 / S14 (eigene Datei)  ChefZ_PreservationDef 14 / S11 (eigene Datei)
//   ChefZ_StationDef      11 / S14 (eigene Datei)  ChefZ_ContainerDef    16 / S17 (eigene Datei)
//   ChefZ_TransformDef    11 / S14 (eigene Datei)  ChefZ_RecipeDef       08 / S6
//   ChefZ_ToolGroupDef    11 / S14 (eigene Datei)
//
// Hier Felder vorwegzunehmen, die S6 oder S12 anders schneiden werden, waere
// Arbeit, die zweimal gemacht wird - und im schlimmsten Fall ein Feldname im
// JSON, den ein Content-Autor benutzt, bevor er stabil ist.
//
// Was JEDE Huelle schon jetzt vollstaendig koennen muss:
//   Normalize / Validate / Compile / PatchFrom / CaptureExplicitBools /
//   ResolveDefaults
// Denn genau das ist der Vertrag, den der Config Manager ab S2 benutzt. Wer
// spaeter ein Feld ergaenzt, ergaenzt es an diesen sechs Stellen - der
// Manager bleibt unberuehrt.
//
// KEIN CONTENT: in dieser Datei steht keine Kategorie, keine Zutat, kein
// Gericht, keine Station. Nur Arten.
//
// Layer: 1_Core.
//==============================================================================

//------------------------------------------------------------------------------
// Kategorie (04)
//------------------------------------------------------------------------------
class ChefZ_CategoryDef extends ChefZ_Record
{
    string parent;              // leer = Wurzel
    string displayName;

    void ChefZ_CategoryDef()
    {
        parent      = ChefZ_Undefined.TEXT;
        displayName = ChefZ_Undefined.TEXT;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.CATEGORY;
    }

    override void Normalize()
    {
        super.Normalize();
        parent.TrimInPlace();
        displayName.TrimInPlace();
    }

    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        // Eine Kategorie, die ihr eigenes Elternteil ist, ist ein Zyklus der
        // Laenge 1. Die allgemeine Zyklenerkennung ueber den ganzen Baum baut
        // S3 (04); dieser Sonderfall ist hier ohne Baum entscheidbar.
        if (parent != "" && parent == id)
        {
            if (ctx)
                ctx.Error(this, "Kategorie ist ihr eigenes \"parent\" - Zyklus der Laenge 1.");
            return false;
        }
        return true;
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_CategoryDef s = ChefZ_CategoryDef.Cast(src);
        if (!s)
            return;
        parent      = PatchText(parent, s.parent, s, "parent");
        displayName = PatchText(displayName, s.displayName, s, "displayName");
    }
}

//------------------------------------------------------------------------------
// Tag (04)
//------------------------------------------------------------------------------
class ChefZ_TagDef extends ChefZ_Record
{
    string displayName;

    void ChefZ_TagDef()
    {
        displayName = ChefZ_Undefined.TEXT;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.TAG;
    }

    override void Normalize()
    {
        super.Normalize();
        displayName.TrimInPlace();
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_TagDef s = ChefZ_TagDef.Cast(src);
        if (!s)
            return;
        displayName = PatchText(displayName, s.displayName, s, "displayName");
    }
}

//------------------------------------------------------------------------------
// Zustand (06) - SYNC-RELEVANT (03 §4)
//
// Seit S9 ausgebaut und deshalb NICHT mehr hier, sondern in ChefZ_StateDef.c.
// Er traegt inzwischen Projektion, implizierte Tags, Verderbfaktor,
// Frischelebensdauer und drei Schalter - das sprengt eine Huelle. Der Eintrag
// bleibt als Wegweiser stehen, damit niemand die Art fuer vergessen haelt.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Qualitaetsstufe (12) - SYNC-RELEVANT (03 §4)
//
// Seit S10 ausgebaut und deshalb NICHT mehr hier, sondern in
// ChefZ_QualityTierDef.c. Sie traegt inzwischen Stufensatz, Rang, Schwelle,
// Ausbeute, Portionsbonus, Verderbfaktor sowie Effekt- und Taglisten - das
// sprengt eine Huelle. Der Eintrag bleibt als Wegweiser stehen, damit niemand
// die Art fuer vergessen haelt.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Werkzeuggruppe (11)
//
// Seit S14 ausgebaut und deshalb NICHT mehr hier, sondern in
// ChefZ_ToolGroupDef.c. Sie traegt inzwischen beide Schreibweisen aus 02 §4 /
// 02 §5.1 mit ausgeschriebener Normalisierung, Validierung und Vererbungs-
// schalter - das sprengt eine Huelle. Der Eintrag bleibt als Wegweiser
// stehen, damit niemand die Art fuer vergessen haelt.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Kochgeraet (10)
//------------------------------------------------------------------------------
class ChefZ_DeviceDef extends ChefZ_Record
{
    ref array<string> deviceCategories;
    int   portionCapacity;
    float qualityModifier;

    void ChefZ_DeviceDef()
    {
        deviceCategories = null;
        portionCapacity  = ChefZ_Undefined.INT;
        qualityModifier  = ChefZ_Undefined.FLOAT;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.DEVICE;
    }

    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!ChefZ_Undefined.IsIntUndefined(portionCapacity) && portionCapacity < 0)
        {
            if (ctx)
                ctx.Error(this, "portionCapacity ist negativ (" + portionCapacity.ToString() + ").");
            return false;
        }
        return true;
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_DeviceDef s = ChefZ_DeviceDef.Cast(src);
        if (!s)
            return;
        deviceCategories = PatchStringArray(deviceCategories, s.deviceCategories);
        portionCapacity  = PatchInt(portionCapacity, s.portionCapacity, s, "portionCapacity");
        qualityModifier  = PatchFloat(qualityModifier, s.qualityModifier, s, "qualityModifier");
    }

    override void ResolveDefaults()
    {
        super.ResolveDefaults();
        // 1.0 = neutral. Ein Geraet ohne Angabe soll die Qualitaet weder heben
        // noch senken; 0.0 waere ein stiller Totalausfall.
        qualityModifier = DefaultFloat("qualityModifier", qualityModifier, 1.0);
        portionCapacity = DefaultInt("portionCapacity", portionCapacity, 0);
    }
}

//------------------------------------------------------------------------------
// Behaelter (16)
//
// Seit S17 ausgebaut und deshalb NICHT mehr hier, sondern in
// ChefZ_ContainerDef.c. Er traegt inzwischen Kategorien, Leerklasse,
// Wiederverwendung, Verbrauch beim Servieren, Verderbfaktor, das
// Suchbitfeld und den Anzeigenamen - das sprengt eine Huelle. Der Eintrag
// bleibt als Wegweiser stehen, damit niemand die Art fuer vergessen haelt.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Zutat (05)
//
// Feldliste woertlich aus 05 §3.1. Zwei Eigenheiten, die zusammengehoeren und
// die man nur zusammen verstehen kann:
//
// 1. ResolveDefaults() fuellt quantityUnit und unitsPerWholeItem BEWUSST NICHT.
//    Diese beiden Felder duerfen "nicht gesetzt" bleiben, bis der
//    ChefZ_IngredientManager die Vererbung entlang der CfgVehicles-Elternkette
//    aufgeloest hat (05 E2). Wuerde ResolveDefaults sie hier auf PIECE/1
//    setzen, waere danach nicht mehr unterscheidbar, ob eine abgeleitete
//    Wurstsorte "PIECE" geerbt hat oder ob niemand etwas gesagt hat - und der
//    geerbte Wert eines Basisrecords ("GRAM", 100) wuerde still von einem
//    Default ueberschrieben.
//
// 2. Aus demselben Grund prueft Validate() NICHT, ob unitsPerWholeItem > 0 ist.
//    Zum Zeitpunkt von VALIDATE ist der Wert womoeglich noch nicht geerbt. Die
//    Pruefung aus 05 §7 (abweisen bzw. auf 1 klemmen) sitzt im Manager, wo der
//    aufgeloeste Wert vorliegt.
//------------------------------------------------------------------------------
class ChefZ_IngredientDef extends ChefZ_Record
{
    /**
     * Mengeneinheit, wenn keine deklariert und keine geerbt ist.
     *
     * Als Konstante und nicht als Einstellung: das ist kein Regler, sondern die
     * Bedeutung von "ein Item". Wer sie serverweit umstellen koennte, wuerde
     * jedes Rezept jedes Content-Moduls gleichzeitig umdeuten.
     *
     * Und es ist KEIN Content (Invariante I3), aus demselben Grund, aus dem
     * CoreSettings.defaultExcludedStates "BURNT"/"ROTTEN" nennen darf: der Core
     * legt damit keine Zutat, keine Kategorie und kein Gericht an. "PIECE" ist
     * eine Zaehleinheit - die Aussage lautet "ein volles Item ist eins", nicht
     * "es gibt das Lebensmittel X".
     */
    static const string DEFAULT_QUANTITY_UNIT = "PIECE";

    ref array<string> categories;
    ref array<string> tags;
    string defaultState;        // "" => Zustand ergibt sich nicht aus der Klasse

    string quantityUnit;        // Symbol der Mengeneinheit (05 §6)
    float  unitsPerWholeItem;   // 1 volles Item entspricht wie vielen Einheiten
    bool   decays;              // speist CanDecay() der Traegerklasse (01 V9)

    string containerCategory;   // Behaelterbindung (16 §4)
    string returnContainer;     // Rueckgabeklasse, "AUTO" erlaubt (16 §4)

    void ChefZ_IngredientDef()
    {
        categories        = null;
        tags              = null;
        defaultState      = ChefZ_Undefined.TEXT;
        quantityUnit      = ChefZ_Undefined.TEXT;
        unitsPerWholeItem = ChefZ_Undefined.FLOAT;
        containerCategory = ChefZ_Undefined.TEXT;
        returnContainer   = ChefZ_Undefined.TEXT;

        // bool ohne Sentinel: die Bool-Sonde traegt "decays" in
        // explicitFields[] nach, wenn es im JSON stand (siehe ChefZ_Record).
        // Genau daran erkennt der Manager spaeter, ob der Wert geerbt werden
        // darf. 01 V9: Edible_Base.CanDecay() ist von Haus aus false - eine
        // Zutat verdirbt nur, wenn es jemand sagt.
        decays            = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.INGREDIENT;
    }

    override void Normalize()
    {
        super.Normalize();
        defaultState.TrimInPlace();
        quantityUnit.TrimInPlace();
        containerCategory.TrimInPlace();
        returnContainer.TrimInPlace();
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_IngredientDef s = ChefZ_IngredientDef.Cast(src);
        if (!s)
            return;
        categories        = PatchStringArray(categories, s.categories);
        tags              = PatchStringArray(tags, s.tags);
        defaultState      = PatchText(defaultState, s.defaultState, s, "defaultState");
        quantityUnit      = PatchText(quantityUnit, s.quantityUnit, s, "quantityUnit");
        unitsPerWholeItem = PatchFloat(unitsPerWholeItem, s.unitsPerWholeItem, s, "unitsPerWholeItem");
        containerCategory = PatchText(containerCategory, s.containerCategory, s, "containerCategory");
        returnContainer   = PatchText(returnContainer, s.returnContainer, s, "returnContainer");
        decays            = PatchBool(decays, s.decays, s, "decays");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_IngredientDef o = ChefZ_IngredientDef.Cast(other);
        if (!o)
            return;
        if (decays == o.decays)
            MarkExplicit("decays");
    }

    //! true, wenn dieser Record zur Mengeneinheit ueberhaupt etwas sagt.
    //! Der Manager benutzt das fuer die Vererbung (05 E2).
    bool HasQuantityUnit()
    {
        return !ChefZ_Undefined.IsTextUndefined(quantityUnit);
    }

    bool HasUnitsPerWholeItem()
    {
        return !ChefZ_Undefined.IsFloatUndefined(unitsPerWholeItem);
    }

    bool HasDecays()
    {
        return HasExplicit("decays");
    }
}

//------------------------------------------------------------------------------
// Huellen ohne eigene Felder in S2
//
// Sie existieren jetzt schon, weil der Sink, die Registries und der
// Ladebericht ihre Art kennen muessen - eine Art nachzuruesten waere sonst eine
// Aenderung am Manager statt an einem Record.
//------------------------------------------------------------------------------

//! Naehrwert (13): seit S12 ausgebaut und deshalb NICHT mehr hier, sondern in
//! ChefZ_NutritionDef.c. Er traegt inzwischen scope, sechs Naehrwertfelder und
//! den Einheitenschalter - das sprengt eine Huelle. Der Eintrag bleibt als
//! Wegweiser stehen, damit niemand die Art fuer vergessen haelt.

//! Konservierung (14): seit S11 ausgebaut und deshalb NICHT mehr hier,
//! sondern in ChefZ_PreservationDef.c. Sie traegt inzwischen scope,
//! Multiplikator, zwei Verfallsschalter, einen Temperaturbereich und den
//! Spielerfaktor - das sprengt eine Huelle. Der Eintrag bleibt als Wegweiser
//! stehen, damit niemand die Art fuer vergessen haelt.

//! Verarbeitungsprozess (11): seit S14 ausgebaut und deshalb NICHT mehr hier,
//! sondern in ChefZ_ProcessDef.c. Er traegt inzwischen die Ausfuehrungsform,
//! Werkzeuggruppen, Dauer, Temperaturfenster, Werkzeugschaden und die
//! Anzeigeangaben - das sprengt eine Huelle. Der Eintrag bleibt als Wegweiser
//! stehen, damit niemand die Art fuer vergessen haelt.

//! Station (11): seit S14 ausgebaut und deshalb NICHT mehr hier, sondern in
//! ChefZ_StationDef.c. Sie traegt inzwischen Stationskategorien, die Liste der
//! angebotenen Prozesse, Parallelslots, Geschwindigkeit und den
//! Brennstoffschalter.

//! Umwandlung (11): seit S14 ausgebaut und deshalb NICHT mehr hier, sondern in
//! ChefZ_TransformDef.c. Sie traegt inzwischen Eingangsslots, Ergebnisse und
//! Nebenprodukte im REZEPTFORMAT (11 E4) - und damit Klassentausch UND
//! setState gleichrangig (V-B §1, Folge 3).

//! Rezept (08): seit S6 ausgebaut und deshalb NICHT mehr hier, sondern in
//! ChefZ_RecipeDef.c. Es traegt inzwischen Kontexte, Slots, Policy, Outputs,
//! Qualitaetsregeln und Faehigkeitsanforderungen - das sprengt eine Huelle.
//! Der Eintrag bleibt als Wegweiser stehen, damit niemand die Art fuer
//! vergessen haelt.
