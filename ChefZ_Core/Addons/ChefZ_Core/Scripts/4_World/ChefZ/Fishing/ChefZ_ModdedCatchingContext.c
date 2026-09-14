//==============================================================================
// modded class CatchingContextFishingRodAction
//
// Entwurf: 20 §4.4 (Wortlaut der Erweiterung), 20 E1 (warum es ohne diese
// Klasse nicht geht), 20 E2 (warum genau diese eine Methode), 20 §12 (die
// Abgrenzung nach aussen), 00 §5 (jede gemoddete Vanilla-Klasse wird benannt
// und begruendet).
//
// DIE EINZIGE modded class dieses Teilsystems. Zwei hinzugefuegte Getter und
// GENAU EIN Override.
//
// ---------------------------------------------------------------------------
// Warum die beiden Getter noetig sind (20 E1)
// ---------------------------------------------------------------------------
// Ein Ertrag bekommt in GetYieldWeight ausschliesslich den Kontext. Die
// gesamte oeffentliche Flaeche der Kontextklassen liefert keine einzige
// Entitaet: das Hauptobjekt ist protected und hat keinen Getter, der Koeder
// ebenfalls.
//
// Der naheliegende Umweg - den Koeder am Hauptobjekt als Anhang suchen - waere
// AUCH MIT Zugriff falsch. Der Koeder haengt nicht an der Rute, sondern am
// HAKEN, der seinerseits an der Rute haengt. Genau deshalb laeuft die Engine
// bei der Ermittlung ueber die ganze Hierarchie und nicht ueber eine Ebene.
// Ein Einzeiler haette still null geliefert - und "kein Treffer" sieht hier
// aus wie "kein Koeder", worauf das System korrekt-aber-falsch mit den
// Grundgewichten weiterliefe. Das ist der teuerste aller Fehler: einer, der
// nach Erfolg aussieht.
//
// Beide Getter sind ADDITIV. Sie ueberschreiben nichts, sie tragen keinen
// Zustand und sie werden von Vanilla nie gerufen.
//
// ---------------------------------------------------------------------------
// Warum das eine Override genau hier sitzt (20 E2)
// ---------------------------------------------------------------------------
// Die Methode unten ist das Nadeloehr, durch das JEDER Koeder- und
// Hakenverlust des Rutenpfades laeuft: beide Verlustwuerfe rufen sie, und die
// beiden Signalabschluesse rufen sie BEDINGUNGSLOS auf den Koeder. Ein
// Kunstkoeder waere nach dem ersten Zyklus weg.
//
// Die Alternativen wurden geprueft und verworfen:
//
//   (a) die beiden Signalmethoden ueberschreiben - braeuchte zwei Overrides,
//       die super NICHT rufen duerfen und dessen Rumpf duplizieren, inklusive
//       Schadensroutine und Datenauffrischung. Jede kuenftige Aenderung der
//       Engine liefe daran vorbei, und wir wuerden zugleich die Version einer
//       fremden Erweiterung derselben Klasse umgehen.
//   (b) die Verlustwahrscheinlichkeit auf 0 klemmen - wirkt nur auf den Wurf,
//       nicht auf die beiden bedingungslosen Aufrufe. Loest das Problem also
//       gar nicht.
//   (c) diese Methode - EIN Override auf dem EINEN Nadeloehr. Alles
//       Nicht-ChefZ geht unveraendert in super.
//
// ---------------------------------------------------------------------------
// Warum super hier NICHT zuerst laeuft - und warum I1 trotzdem gilt
// ---------------------------------------------------------------------------
// Die Regel "super zuerst, danach nur beobachten" gilt dem Kochhook, wo super
// den Spielzustand fortschreibt. Hier IST super das Loeschen. "Super zuerst"
// hiesse, den Koeder erst zu zerstoeren und ihn danach nicht mehr retten zu
// koennen.
//
// Der Rueckfall bleibt trotzdem vollstaendig Vanilla: die Bedingung ist ein
// einzelner Treffer in einer Hashtabelle, und JEDER Nicht-Treffer - jeder
// Haken, jeder Vanilla-Koeder, jeder Weichkoeder, jedes Item eines fremden
// Mods - geht unveraendert in super. Steht die Registry nicht, ist die
// Bedingung definitionsgemaess falsch, und dieser Code ist ein Bool-Test.
//
// ---------------------------------------------------------------------------
// Kollisionsflaeche
// ---------------------------------------------------------------------------
// Ueberschrieben wird ausschliesslich diese eine Methode. Sie trifft keine
// Fangentscheidung, keinen Chancenwert, keine Signalzeit und keinen
// Qualitaetsmodifikator - also nichts von dem, was erweiternde Mods an dieser
// Klasse ueblicherweise anfassen. Die Fangaktion selbst wird nicht angefasst.
// Die Arbeitsteilung ist sauber: ob ueberhaupt gefangen wird, entscheidet
// diese Datei nicht; was gefangen wird, entscheidet die Gewichtstabelle.
//
// Layer: 4_World.
// SCOUT-GEPRUEFT 2026-09-15, I4-BELEG: Terje ueberschreibt an derselben Klasse TryDamageItems,
// GetChanceCoef, RandomizeSignalDuration, RandomizeSignalStartTime und
// GetQualityModifier - ChefZ nur RemoveItemSafe. Keine Ueberschneidung
// (GATE_FISHING_REPORT.md S-06).
//==============================================================================

modded class CatchingContextFishingRodAction
{
    //! NEU, additiv. Die einzige Bruecke vom Kontext zum Ertragsobjekt.
    EntityAI ChefZ_GetBaitEntity()
    {
        return m_Bait;
    }

    //! NEU, additiv. Heute nur fuer die Diagnose; er steht hier, weil die
    //! Frage "was haengt an der Rute" ohne ihn ein zweites Mal ueber die
    //! Hierarchie laufen muesste.
    EntityAI ChefZ_GetHookEntity()
    {
        return m_Hook;
    }

    /**
     * EINZIGER Override dieses Teilsystems.
     *
     * Ein Kunstkoeder wird nicht geloescht, sondern abgenutzt - und zwar
     * ausschliesslich serverseitig, genau wie der Hakenschaden der Engine.
     * Der Client fasst Health nicht an; er bekommt den Wert ueber die
     * gewoehnliche Item-Synchronisation.
     *
     * Erreicht der Koeder dabei "ruined", uebergeht ihn die
     * Datenauffrischung der Engine beim naechsten Durchlauf, der Koeder gilt
     * als nicht vorhanden, und die Gewichte fallen von selbst auf Vanilla
     * zurueck. Ein eigener Sonderpfad dafuer waere doppelte Arbeit.
     */
    override protected void RemoveItemSafe(EntityAI item)
    {
        if (item)
        {
            string itemType = item.GetType();
            int typeHash = itemType.Hash();

            ChefZ_FishingRegistry registry = ChefZ_FishingRegistry.Get();
            if (registry.IsLure(typeHash))
            {
                if (!g_Game.IsMultiplayer() || g_Game.IsDedicatedServer())
                {
                    float wear = registry.GetLureWear(typeHash);
                    if (wear > 0.0)
                        item.AddHealth("", "Health", -wear);
                }

                // super wird bewusst NICHT gerufen: super IST das Loeschen.
                return;
            }
        }

        super.RemoveItemSafe(item);
    }
}
