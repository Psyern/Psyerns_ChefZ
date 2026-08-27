//==============================================================================
// ChefZ_Container_Base - die optionale Basis eines Behaelters
//
// Entwurf: 16 §3.2 (Schnittstelle woertlich: "abstrakte Basis fuer Behaelter,
// scope = 0"), 16 §7 ("Vanilla-Item als Behaelterklasse - zulaessig und
// gelegentlich gewollt"), 16 E2 (Kategorien statt Klassenlisten).
//
// ---------------------------------------------------------------------------
// Sie ist OPTIONAL, und das ist der Punkt
// ---------------------------------------------------------------------------
// Ein Behaelter muss NICHT von hier erben. Die ChefZ_ContainerRegistry
// arbeitet ausschliesslich auf Klassennamen aus CfgChefZContainers; ein
// Vanilla-Topf kann dort stehen und funktioniert vollstaendig, ohne dass
// irgendjemand seine Skriptklasse anfasst (16 §7). Genau das ist die Zusage
// aus 16 E2 - ein neuer Behaelter kostet einen Eintrag, keinen Code.
//
// Was diese Basis bringt, ist zweierlei und sonst nichts:
//
//   1. ChefZ_GetContainerCategory()  die erste Kategorie, ohne Umweg ueber
//                                    die Registry - fuer Tooltips und fuer
//                                    Content, das seine eigene Anzeige baut.
//   2. ChefZ_IsEmpty()               eine Klasse, die "leer" anders definiert
//                                    als "kein Cargo und keine Menge", sagt
//                                    das HIER und nicht im Core.
//
// Der zweite Punkt ist der eigentliche Grund fuer die Klasse: der
// ChefZ_ContainerService darf nie einen GEFUELLTEN Behaelter verbrauchen -
// ein Topf mit Wasser ist keine freie Schuessel. Seine allgemeine Antwort
// (Cargo leer und Menge <= 0) trifft fast immer zu; wo sie es nicht tut, ist
// eine Ueberschreibung besser als eine Sonderregel im Core.
//
// ---------------------------------------------------------------------------
// Warum ItemBase und nicht ChefZ_Item_Base
// ---------------------------------------------------------------------------
// 16 §3.2 schreibt "class ChefZ_Container_Base : ItemBase", und das ist die
// sparsame Wahl: ChefZ_Item_Base traegt einen vollstaendigen
// ChefZ_ItemStateComponent mit Zustand, Qualitaet, Frische, Portionszaehler,
// Sync und einem eigenen Persistenzblock. Ein leerer Teller braucht davon
// nichts. Jedes dieser Felder waere Sync- und Spielstandslast auf einem Item,
// dessen ganzer Zweck es ist, leer zu sein.
//
// Wer einen Behaelter mit ChefZ-Zustand will - eine Schuessel, die schmutzig
// werden kann (16 E6) -, leitet seine Klasse von ChefZ_Item_Base ab und
// traegt sie ebenso in CfgChefZContainers ein. Beides geht; die Registry
// interessiert die Skriptbasis nicht.
//
// FUER CONTENT-AUTOREN:
//     config.cpp   class ChefZ_EmptyBowl : Container_Base { ... };   (Vanilla-Basis)
//     script       class ChefZ_EmptyBowl extends ChefZ_Container_Base { }
//     config.cpp   class CfgChefZContainers { class ChefZ_EmptyBowl { ... }; };
//
// Der Core bringt fuer diese Basis KEINEN CfgVehicles-Eintrag mit - er
// enthaelt keine Items, auch keine unsichtbaren (Invariante I3).
//
// KEIN CONTENT: kein Teller, keine Schuessel, keine Kategorie.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Container_Base extends ItemBase
{
    /**
     * Die erste Kategorie dieses Behaelters, oder INVALID.
     *
     * "Die erste" und nicht "alle": ein Behaelter darf mehreren Kategorien
     * angehoeren (16 §7), fuer eine Anzeige ist aber genau eine gemeint. Wer
     * alle braucht, fragt die Registry - GetClassesForCategory() ist die
     * Gegenrichtung, und ChefZ_ContainerRegistry.GetDef() liefert die volle
     * Liste.
     *
     * Der Wert wird NICHT zwischengespeichert: die Registry ist nach dem Boot
     * unveraenderlich, die Antwort damit stabil, und ein Feld auf jedem
     * Teller waere Speicher fuer einen Map-Zugriff.
     */
    ChefZ_Sym ChefZ_GetContainerCategory()
    {
        ChefZ_ContainerDef def;
        if (!ChefZ_ContainerRegistry.Get().GetDef(ChefZ_SymbolTable.Lookup(GetType()), def))
            return ChefZ_SymbolTable.INVALID;

        if (!def.containerCategories || def.containerCategories.Count() == 0)
            return ChefZ_SymbolTable.INVALID;

        return ChefZ_SymbolTable.Lookup(def.containerCategories.Get(0));
    }

    /**
     * Ist dieser Behaelter frei, also benutzbar?
     *
     * Die Vorgabe ist dieselbe Antwort, die der ChefZ_ContainerService auch
     * fuer ein Vanilla-Item gibt - siehe ChefZ_ContainerService.IsEmpty().
     * Sie steht dort und nicht hier, weil sie fuer BEIDE Aeste gelten muss:
     * fuer Klassen, die von hier erben, und fuer die, die es nicht tun.
     *
     * Eine Ableitung darf sie ueberschreiben. Das ist der eigentliche Zweck
     * dieser Klasse (siehe Dateikopf).
     */
    bool ChefZ_IsEmpty()
    {
        return ChefZ_ContainerService.IsEmpty(this);
    }
}
