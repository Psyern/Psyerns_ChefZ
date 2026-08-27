//==============================================================================
// ChefZ_PlannedOutput - ein Ergebnis, entschieden aber noch nicht erzeugt
//
// Entwurf: 08 §6 (die Transaktion, Schritte 2 bis 5), 08 §2 (ChefZ_OutputDef),
// 10 §8 ("Ergebnis passt nicht ins Gefaess -> Abbruch VOR jeder Aenderung").
//
// ---------------------------------------------------------------------------
// Warum es diese Zwischenstufe gibt
// ---------------------------------------------------------------------------
// 08 §6 verlangt, dass der Platz fuer JEDES Ergebnis geprueft wird, BEVOR das
// erste erzeugt wird. Dafuer muss vorher feststehen, welche Klassen ueberhaupt
// entstehen sollen - und genau das ist bei Nebenprodukten eine Entscheidung
// mit Zufall (ChefZ_OutputDef.chance).
//
// Wuerde der Wurf zweimal geworfen - einmal fuer die Platzpruefung und einmal
// beim Erzeugen -, pruefte der Applicator den Platz fuer eine andere Menge
// als die, die er anschliessend erzeugt. Der Wurf faellt deshalb GENAU EINMAL,
// beim Planen, und das Ergebnis steht hier.
//
// Die Liste dieser Objekte ist zugleich die Antwort auf die Frage, was bei
// einem Fehlschlag zurueckgerollt werden muss: created zeigt nach Schritt 4
// auf die tatsaechlich entstandene Instanz oder ist null.
//
// ---------------------------------------------------------------------------
// def OHNE ref - dieselbe Begruendung wie bei ChefZ_MatchResult.recipe
// ---------------------------------------------------------------------------
// Eigentuemer der ChefZ_OutputDef ist das kompilierte Rezept, und das lebt
// laenger als jede Transaktion (08 §7: nach dem Build unveraenderlich). Ein
// ref waere hier ein Zyklus in Wartestellung, ohne einen einzigen Gewinn.
//
// KEIN CONTENT: hier steht kein Klassenname, keine Kategorie, kein Gericht.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_PlannedOutput
{
    //! Die Rohdefinition aus dem Rezept. Siehe Kopf: bewusst ohne ref.
    ChefZ_OutputDef def;

    //! Die AUFGELOESTE Ergebnisklasse - Grundklasse oder Qualitaetsvariante
    //! (12 §3). Sie steht hier, damit Platzpruefung und Erzeugung mit
    //! Sicherheit dieselbe Klasse meinen.
    string cls;

    //! true fuer Nebenprodukte. Sie werden gleich behandelt wie Ergebnisse
    //! (08 §6 nennt beide in Schritt 4), nur ihre Meldungen sind andere.
    bool byproduct;

    //! Position in outputs[] bzw. byproducts[] - ausschliesslich fuer
    //! Meldungen, damit ein Autor die Stelle in seiner Datei wiederfindet.
    int index;

    //! Nach Schritt 4 die erzeugte Instanz, davor null. Schwacher Zeiger: die
    //! starke Referenz haelt die Welt, und der Rollback loescht ueber genau
    //! diese Liste.
    ItemBase created;

    void ChefZ_PlannedOutput()
    {
        def       = null;
        cls       = "";
        byproduct = false;
        index     = -1;
        created   = null;
    }

    //! "outputs[2]" oder "byproducts[0]" - die Adresse in der Rezeptdatei.
    string Where()
    {
        if (byproduct)
            return "byproducts[" + index.ToString() + "]";
        return "outputs[" + index.ToString() + "]";
    }

    string ToDebugString()
    {
        string s = Where() + " " + cls;
        if (created)
            s = s + " (erzeugt)";
        return s;
    }
}
