//==============================================================================
// ChefZ_ProcessCompiler - Rohform -> kompilierte Prozesse, Stationen, Transforms
//
// Entwurf: 11 §2 (Felder), 11 §5 (BOOT-Datenfluss und die Konsistenzliste),
// 11 §7 (Fehlerverhalten, Zeile fuer Zeile), 11 E4 (derselbe Selektor und
// dasselbe OutputDef wie Rezepte), 11 E8, 09 §4.1 (Spezifitaet), 07 §5
// (Selektor- und Slotcompiler), V-B §4 (essbare Ergebnisklassen brauchen
// Nutrition/Food).
//
// ---------------------------------------------------------------------------
// Die Leitlinie: ein DATENSATZ wird ganz genommen oder gar nicht - mit EINER
// bewussten Ausnahme
// ---------------------------------------------------------------------------
// Fuer Prozesse und Transforms gilt dieselbe Regel wie fuer Rezepte (08 §8):
// jeder Fehler verwirft den ganzen Datensatz. Ein halb gueltiger Transform ist
// gefaehrlicher als keiner.
//
// Die AUSNAHME ist die Station, und 11 §7 nennt sie ausdruecklich: "Station
// nennt unbekannten Prozess -> Eintrag verworfen, ERROR. Die uebrigen Prozesse
// der Station funktionieren." Das ist richtig so - eine Station ist ein
// MOEBELSTUECK, das in der Welt steht und das ein Spieler gebaut hat. Sie
// wegen eines Tippfehlers in einem von drei Prozessen komplett stumm zu
// schalten, waere eine Strafe fuer den falschen.
//
// ---------------------------------------------------------------------------
// Warum das hier liegt und nicht in 1_Core
// ---------------------------------------------------------------------------
// Drei Pruefungen lesen CfgVehicles - Ergebnisklasse existiert, essbare
// Ergebnisklasse registriert am Magen, Stationsklasse existiert. g_Game gibt
// es erst ab 3_Game (00 §4).
//
// KEIN CONTENT: keine Klasse, kein Prozess, keine Station wird hier benannt.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ProcessCompiler
{
    private ChefZ_LoadReport      m_Report;    // ohne ref: gehoert dem Manager
    private ChefZ_CompileContext  m_Ctx;       // ohne ref: gehoert dem Manager

    /**
     * Pruefung gegen CfgVehicles. Aus, wenn kein Spiel laeuft (Selbsttest):
     * ohne g_Game gaebe es keine Config, und dann waere JEDE Ergebnisklasse
     * "nicht vorhanden" - der Selbsttest wuerde jeden Testtransform verwerfen
     * und dabei nichts pruefen. Dieselbe Ueberlegung wie im
     * ChefZ_RecipeCompiler.
     */
    private bool m_VerifyClasses;

    void ChefZ_ProcessCompiler()
    {
        m_VerifyClasses = true;
    }

    void Init(ChefZ_CompileContext ctx, ChefZ_LoadReport report)
    {
        m_Ctx    = ctx;
        m_Report = report;
    }

    void SetVerifyClasses(bool on)
    {
        m_VerifyClasses = on;
    }

    //==========================================================================
    // Prozess
    //==========================================================================

    /**
     * Einen Prozess kompilieren. null heisst "abgewiesen".
     *
     * @param tools  Werkzeugregistry. Sie MUSS gebaut sein - 11 §7: "Prozess
     *        nennt unbekannte Toolgruppe -> Prozess ABGEWIESEN, ERROR - ohne
     *        Werkzeugpruefung waere er zu leicht ausloesbar." Genau deshalb
     *        wird hier bei fehlender Registry nicht durchgewunken, sondern
     *        abgewiesen: ein Prozess, dessen Werkzeugbedingung stillschweigend
     *        entfaellt, ist ein Prozess ohne Bedingung.
     */
    ChefZ_CompiledProcess CompileProcess(ChefZ_ProcessDef def, ChefZ_ToolRegistry tools)
    {
        if (!def)
            return null;

        int exec = ChefZ_ProcessExec.FromName(def.exec);
        if (exec < 0)
        {
            // Kann nach Validate() nicht mehr vorkommen. Die zweite Sicherung
            // steht hier, weil der Compiler auch aus Selbsttests gerufen wird,
            // und dort ist die Validierungsstufe nicht garantiert durchlaufen.
            Fail(def, "unbekannte Ausfuehrungsform \"" + def.exec + "\" - Prozess abgewiesen. "
                + "Gueltig: " + ChefZ_ProcessExec.ValidNames() + ".");
            return null;
        }

        ChefZ_CompiledProcess proc = new ChefZ_CompiledProcess();
        proc.processSym = ChefZ_SymbolTable.Intern(def.id);
        proc.id         = def.id;
        proc.sourceRef  = def.sourceRef;
        proc.exec       = exec;

        if (!CompileToolGroups(def, proc, tools))
            return null;

        proc.baseDurationSec = ResolveDuration(def, proc);

        proc.hasMinTemperature = def.HasMinTemperature();
        proc.minTemperature    = def.minTemperature;
        proc.hasMaxTemperature = def.HasMaxTemperature();
        proc.maxTemperature    = def.maxTemperature;

        if (proc.hasMinTemperature && proc.hasMaxTemperature
            && proc.minTemperature > proc.maxTemperature)
        {
            // 07 §7: vertauschte Grenzen werden GETAUSCHT, nicht abgewiesen -
            // die Absicht des Autors ist eindeutig. Dieselbe Regel wie bei
            // jedem anderen Wertebereich im System.
            Warn(def, "minTemperature (" + proc.minTemperature.ToString() + ") ist groesser "
                + "als maxTemperature (" + proc.maxTemperature.ToString()
                + ") - die Grenzen werden getauscht.");

            float swap          = proc.minTemperature;
            proc.minTemperature = proc.maxTemperature;
            proc.maxTemperature = swap;
        }

        proc.requiresHeat    = def.requiresHeat;
        proc.toolDamage      = ClampToolDamage(def);
        proc.animationLength = def.animationLength;
        proc.specialty       = def.specialty;
        proc.displayName     = def.displayName;

        // Ereignisnamen sind fuer den Core UNDURCHSICHTIG (17 E1): getragen
        // und weitergereicht, nie gedeutet.
        proc.emitEvents.Clear();
        if (def.emitEvents)
        {
            for (int i = 0; i < def.emitEvents.Count(); i++)
                proc.emitEvents.Insert(def.emitEvents.Get(i));
        }

        WarnAboutMisplacedFields(def, proc);
        return proc;
    }

    private bool CompileToolGroups(notnull ChefZ_ProcessDef def,
                                   notnull ChefZ_CompiledProcess proc,
                                   ChefZ_ToolRegistry tools)
    {
        proc.toolGroups.Clear();

        int declared = ChefZ_TextList.Count(def.toolGroups);
        if (declared == 0)
            return true;                // kein Werkzeug noetig - zulaessig

        for (int i = 0; i < declared; i++)
        {
            string name = def.toolGroups.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym group = ChefZ_SymbolTable.Intern(name);

            if (!tools || !tools.HasGroup(group))
            {
                Fail(def, "toolGroups[" + i.ToString() + "] nennt die Werkzeuggruppe \""
                    + name + "\", die in keinem CfgChefZTools-Eintrag vorkommt - Prozess "
                    + "ABGEWIESEN (11 §7). Nicht ignoriert: ohne Werkzeugpruefung waere der "
                    + "Prozess ohne Werkzeug ausloesbar, und das faellt niemandem auf.");
                return false;
            }

            if (proc.toolGroups.Find(group) < 0)
                proc.toolGroups.Insert(group);
        }

        return true;
    }

    /**
     * Die Grunddauer, geklemmt.
     *
     * Ein Stationsprozess mit Dauer 0 waere sofort fertig - fuer
     * STATION_TIMED heisst das "ein Raeuchervorgang, der nie stattfindet",
     * fuer STATION_ACTION "eine Aktion ohne Fortschrittsbalken". Beides ist
     * fast immer ein vergessenes Feld und nie eine Absicht, also wird geklemmt
     * und gemeldet.
     *
     * HANDCRAFT braucht keine Dauer: dort bestimmt Vanillas Craftsystem die
     * Zeit ueber animationLength und die Softskills (11 E2).
     */
    private float ResolveDuration(notnull ChefZ_ProcessDef def,
                                  notnull ChefZ_CompiledProcess proc)
    {
        float d = def.baseDurationSec;

        if (!proc.IsStationProcess())
        {
            if (d < 0.0)
                return 0.0;
            return d;
        }

        if (d >= ChefZ_ProcessingLimits.MIN_DURATION_SEC)
            return d;

        Warn(def, "baseDurationSec = " + d.ToString() + " ist fuer einen "
            + ChefZ_ProcessExec.Name(proc.exec) + "-Prozess zu klein und wird auf "
            + ChefZ_ProcessingLimits.MIN_DURATION_SEC.ToString() + " geklemmt. Ein Stationsjob "
            + "ohne Dauer waere von \"sofort\" nicht zu unterscheiden, und der "
            + "Fortschrittsbalken waere nur Flackern.");

        return ChefZ_ProcessingLimits.MIN_DURATION_SEC;
    }

    private int ClampToolDamage(notnull ChefZ_ProcessDef def)
    {
        int d = def.toolDamage;
        if (d < 0)
        {
            Warn(def, "toolDamage = " + d.ToString() + " ist negativ und wird auf 0 geklemmt. "
                + "Ein Prozess, der Werkzeuge REPARIERT, ist keine Funktion dieses Systems.");
            return 0;
        }
        if (d > 100)
        {
            Warn(def, "toolDamage = " + d.ToString() + " liegt ueber 100 und wird geklemmt. "
                + "100 ist der volle Gesundheitsbereich eines Items.");
            return 100;
        }
        return d;
    }

    /**
     * Felder, die zur gewaehlten Ausfuehrungsform nicht passen.
     *
     * Kein Fehler, aber eine Meldung - sonst wundert sich ein Content-Autor,
     * warum seine sorgfaeltig gesetzte animationLength am Raeucherschrank
     * nichts bewirkt. Genau solche stillen Wirkungslosigkeiten sind die
     * teuersten Stunden im Content-Bau.
     */
    private void WarnAboutMisplacedFields(notnull ChefZ_ProcessDef def,
                                          notnull ChefZ_CompiledProcess proc)
    {
        if (proc.exec == ChefZ_ProcessExec.HANDCRAFT)
            return;

        if (proc.animationLength > 0.0 || proc.specialty > 0.0)
        {
            Warn(def, "animationLength und specialty wirken nur bei HANDCRAFT (11 §2) - "
                + "dieser Prozess ist " + ChefZ_ProcessExec.Name(proc.exec)
                + " und ignoriert sie. Die Dauer eines Stationsjobs steht in "
                + "baseDurationSec.");
        }
    }

    //==========================================================================
    // Station
    //==========================================================================

    /**
     * Eine Station kompilieren. Sie wird NIE wegen eines einzelnen Prozesses
     * abgewiesen (11 §7) - siehe Kopfkommentar.
     *
     * @param known  Prozesssymbol -> ist bekannt. Der Manager reicht die
     *        bereits kompilierte Prozessmenge herein.
     */
    ChefZ_CompiledStation CompileStation(ChefZ_StationDef def, notnull map<int, int> known)
    {
        if (!def)
            return null;

        ChefZ_CompiledStation station = new ChefZ_CompiledStation();
        station.stationSym      = ChefZ_SymbolTable.Intern(def.id);
        station.id              = def.id;
        station.sourceRef       = def.sourceRef;
        station.parallelSlots   = def.parallelSlots;
        station.speedMultiplier = def.speedMultiplier;
        station.needsFuel       = def.needsFuel;

        ChefZ_TextList.SymbolsOf(def.stationCategories, station.categories);

        int declared = ChefZ_TextList.Count(def.processes);
        for (int i = 0; i < declared; i++)
        {
            string name = def.processes.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);

            if (!known.Contains(sym))
            {
                Fail(def, "processes[" + i.ToString() + "] nennt den Prozess \"" + name
                    + "\", den es nicht gibt - der EINTRAG wird verworfen. Die uebrigen "
                    + "Prozesse dieser Station funktionieren weiter (11 §7). Fehlt das "
                    + "Content-Modul, oder ist der Name falsch geschrieben?");
                continue;
            }

            if (station.processes.Find(sym) >= 0)
            {
                // Kein Fehler, aber der Prozessordinal ist der INDEX in dieser
                // Liste (er wird synchronisiert), und ein doppelter Eintrag
                // verschoebe alle folgenden Ordinale ohne Not.
                Warn(def, "Prozess \"" + name + "\" steht mehrfach in processes[] - der "
                    + "zweite Eintrag wird ausgelassen.");
                continue;
            }

            station.processes.Insert(sym);
        }

        if (station.processes.Count() > ChefZ_ProcessingLimits.PROCESS_ORDINAL_MAX + 1)
        {
            Warn(def, "Die Station bietet " + station.processes.Count().ToString()
                + " Prozesse an; synchronisierbar sind "
                + (ChefZ_ProcessingLimits.PROCESS_ORDINAL_MAX + 1).ToString()
                + ". Die uebrigen sind serverseitig nutzbar, aber der Client kann den "
                + "aktiven Prozess dann nicht anzeigen.");
        }

        if (declared == 0)
        {
            Warn(def, "Die Station nennt keinen einzigen Prozess - sie ist inerte Deko. "
                + "Das ist zulaessig; falls es ein Versehen war, fehlt processes[].");
        }

        VerifyStationClass(def);
        return station;
    }

    /**
     * 11 §2: "id == Klassenname der Station".
     *
     * Fehlt die Klasse, ist der Eintrag wirkungslos - aber KEIN Fehler:
     * dieselbe Ueberlegung wie bei stationsAllowed (11 §7). Die Klasse kann
     * aus einem optionalen Modul kommen, das auf diesem Server nicht geladen
     * ist. Ein ERROR wuerde dann jeden Serverstart mit einem Fehler versehen,
     * der keiner ist - und Betreiber, die drei Wochen lang Fehler ignorieren,
     * ignorieren auch den vierten.
     */
    private void VerifyStationClass(notnull ChefZ_StationDef def)
    {
        if (!m_VerifyClasses || !g_Game)
            return;
        if (ChefZ_VanillaNutrition.ClassExists(def.id))
            return;

        Warn(def, "Zu diesem CfgChefZStations-Eintrag gibt es keine CfgVehicles-Klasse \""
            + def.id + "\". Der Eintrag bleibt geladen und ist wirkungslos, bis die Klasse "
            + "da ist (11 §2: die id IST der Klassenname).");
    }

    //==========================================================================
    // Transform
    //==========================================================================

    /**
     * Einen Transform kompilieren. null heisst "abgewiesen", der Grund steht
     * dann im Ladebericht - immer mit Transform-ID und Herkunft.
     *
     * @param processes  Prozesssymbol -> kompilierter Prozess. Gebraucht fuer
     *        zwei Pruefungen: existiert der Prozess ueberhaupt (11 §7), und
     *        ist er HANDCRAFT (dann gilt die 2-Eingaenge-Grenze aus 01 V12).
     * @param stations   Stationssymbol -> ist bekannt. Nur fuer die WARNUNG zu
     *        stationsAllowed; ein unbekannter Eintrag weist NICHT ab.
     */
    ChefZ_CompiledTransform CompileTransform(ChefZ_TransformDef def,
                                             notnull map<int, ref ChefZ_CompiledProcess> processes,
                                             notnull map<int, int> stations)
    {
        if (!def)
            return null;

        if (!m_Ctx)
        {
            Fail(def, "kein Selektorkontext vorhanden - der Config Manager hat die Registries "
                + "noch nicht gebaut. Der Transform wird uebersprungen.");
            return null;
        }

        m_Ctx.SetSubject(def.sourceRef, def.id);

        ChefZ_Sym processSym = ChefZ_SymbolTable.Intern(def.process);

        ChefZ_CompiledProcess proc;
        if (!processes.Find(processSym, proc) || !proc)
        {
            Fail(def, "nennt den Prozess \"" + def.process + "\", den es nicht gibt - "
                + "Transform abgewiesen (11 §7). Kein Prozess heisst: keine Station koennte "
                + "ihn anbieten und keine Aktion ihn ausloesen.");
            return null;
        }

        ChefZ_CompiledTransform tr = new ChefZ_CompiledTransform();
        tr.transformSym = ChefZ_SymbolTable.Intern(def.id);
        tr.id           = def.id;
        tr.sourceRef    = def.sourceRef;
        tr.processSym   = processSym;

        if (!CompileInputs(def, tr, proc))       return null;
        if (!CompileOutputs(def, tr))            return null;
        if (!CheckStateChangeConsistency(def, tr)) return null;

        CompileStations(def, tr, stations);
        CompileTuning(def, tr, proc);
        CompileRequirements(def, tr);
        ComputeIndexFacts(tr);

        return tr;
    }

    //--------------------------------------------------------------------------
    // Eingaenge (11 E4: DERSELBE Slotcompiler wie beim Rezept)
    //--------------------------------------------------------------------------

    private bool CompileInputs(notnull ChefZ_TransformDef def,
                               notnull ChefZ_CompiledTransform tr,
                               notnull ChefZ_CompiledProcess proc)
    {
        array<ref ChefZ_CompiledSlot> slots;
        string error;

        if (!ChefZ_SelectorCompiler.CompileSlotList(def.inputs, m_Ctx, slots, error))
        {
            Fail(def, "Transform abgewiesen, weil ein Eingang nicht uebersetzt werden konnte: "
                + error + ". Ein Transform mit weggefallenem Pflichteingang waere viel zu "
                + "leicht ausloesbar (07 §7).");
            return false;
        }

        tr.inputs.Clear();
        for (int i = 0; i < slots.Count(); i++)
            tr.inputs.Insert(slots.Get(i));

        if (tr.RequiredInputCount() == 0)
        {
            Fail(def, "Transform hat keinen einzigen Pflichteingang - alle "
                + tr.inputs.Count().ToString() + " Eingaenge sind optional oder verlangen 0 "
                + "Items. Er wuerde an jeder Station zuenden, auch an einer leeren.");
            return false;
        }

        /**
         * 01 V12 / 11 §3: Vanillas RecipeBase kennt genau ZWEI Zutaten.
         *
         * 11 §3 ist an dieser Stelle ausdruecklich: "Die 2-Eingaenge-Grenze ist
         * eine Validatorregel, keine Laufzeitfalle. Ein HANDCRAFT-Prozess mit
         * drei Eingaengen liesse sich stumm nicht registrieren; der Validator
         * weist ihn als FEHLER ab, mit Verweis auf STATION_*."
         *
         * Genau das passiert hier - und zwar bereits beim Build, nicht erst
         * bei der Registrierung in S15. Ein Transform, der nie als Craftrezept
         * erscheint und dafuer keine Meldung erzeugt, ist der Fehler, den
         * niemand findet.
         */
        if (proc.exec == ChefZ_ProcessExec.HANDCRAFT
            && tr.inputs.Count() > ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS)
        {
            Fail(def, "Der Prozess \"" + proc.id + "\" ist HANDCRAFT und laeuft damit ueber "
                + "Vanillas Craftsystem; das kennt hoechstens "
                + ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS.ToString() + " Zutaten (01 V12). "
                + "Dieser Transform hat " + tr.inputs.Count().ToString()
                + " - abgewiesen. Abhilfe: den Prozess auf STATION_ACTION oder STATION_TIMED "
                + "umstellen; Stationen haben diese Grenze nicht.");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Ergebnisse (11 E4: DASSELBE ChefZ_OutputDef wie beim Rezept)
    //--------------------------------------------------------------------------

    private bool CompileOutputs(notnull ChefZ_TransformDef def,
                                notnull ChefZ_CompiledTransform tr)
    {
        tr.outputs.Clear();
        tr.byproducts.Clear();

        if (!def.outputs || def.outputs.Count() == 0)
        {
            Fail(def, "Transform ohne \"outputs\" - abgewiesen. Er wuerde die Eingaenge "
                + "verbrauchen und nichts erzeugen.");
            return false;
        }

        int withClass    = 0;
        int withoutClass = 0;

        for (int i = 0; i < def.outputs.Count(); i++)
        {
            ChefZ_OutputDef o = def.outputs.Get(i);
            if (!o)
                continue;

            string where = "outputs[" + i.ToString() + "]";

            if (o.cls == "")
            {
                /**
                 * 11 §2: der Zustandswechsel OHNE Klassenwechsel.
                 *
                 * Beim Rezept waere das ein Fehler ("ein Ergebnis ohne Klasse
                 * ist kein Ergebnis"); beim Transform ist es ein vollwertiger
                 * Output - allerdings nur MIT setState. Ohne beides sagt der
                 * Eintrag gar nichts.
                 */
                if (o.setState == "")
                {
                    Fail(def, where + " nennt weder \"cls\" noch \"setState\" - abgewiesen. "
                        + "Ein Ergebnis muss entweder eine Klasse erzeugen oder einen Zustand "
                        + "setzen (11 §2).");
                    return false;
                }

                withoutClass++;
                tr.outputs.Insert(o);
                continue;
            }

            if (!CheckOutputClass(def, o.cls, where))
                return false;

            // Portionsfelder (15 §7). Ein Transform traegt dasselbe
            // ChefZ_OutputDef wie ein Rezept (11 E4) und darf deshalb auch ein
            // Portionsgericht erzeugen - ein Kessel am Raeucherplatz ist
            // dieselbe Sache wie ein Kessel am Feuer.
            if (!CheckPortionFields(def, o, where))
                return false;

            // Varianten je Qualitaetsstufe (12): dieselbe Pruefung. Eine
            // fehlende Variantenklasse ist genauso toedlich wie eine fehlende
            // Hauptklasse - nur faellt sie erst auf, wenn jemand die Stufe
            // trifft, und das kann Wochen dauern.
            if (o.variants)
            {
                for (int v = 0; v < o.variants.Count(); v++)
                {
                    ChefZ_OutputVariant variant = o.variants.Get(v);
                    if (!variant || variant.cls == "")
                        continue;
                    if (!CheckOutputClass(def, variant.cls,
                            where + ".variants[" + v.ToString() + "]"))
                        return false;
                }
            }

            withClass++;
            tr.outputs.Insert(o);
        }

        if (tr.outputs.Count() == 0)
        {
            Fail(def, "Transform hat nach dem Pruefen kein einziges brauchbares Ergebnis mehr "
                + "- abgewiesen.");
            return false;
        }

        /**
         * Gemischt ist ein WIDERSPRUCH, kein Sonderfall.
         *
         * Ein Output mit Klasse verbraucht die Eingaenge; einer ohne laesst
         * sie bestehen. Beides zugleich hiesse, dasselbe Item zu verbrauchen
         * und weiterzufuehren - und das Ergebnis haenge davon ab, in welcher
         * Reihenfolge der Runner die Liste abarbeitet. Ein Verhalten, das man
         * nur durch Ausprobieren herausfindet, ist kein Verhalten.
         */
        if (withClass > 0 && withoutClass > 0)
        {
            Fail(def, "outputs[] mischt Klassentausch (" + withClass.ToString()
                + " mit \"cls\") und reinen Zustandswechsel (" + withoutClass.ToString()
                + " ohne \"cls\") - abgewiesen. Das eine verbraucht die Eingaenge, das andere "
                + "laesst sie bestehen; beides zugleich ist nicht bestimmbar. Trenne es in "
                + "zwei Transforms.");
            return false;
        }

        tr.pureStateChange = (withClass == 0);

        // Nebenprodukte MUESSEN eine Klasse nennen: ein Nebenprodukt ohne
        // Klasse ist nichts, und "setState" auf einem Nebenprodukt hat kein
        // Ziel - Nebenprodukte entstehen neu, sie werden nicht umgewandelt.
        if (def.byproducts)
        {
            for (int b = 0; b < def.byproducts.Count(); b++)
            {
                ChefZ_OutputDef by = def.byproducts.Get(b);
                if (!by)
                    continue;

                string byWhere = "byproducts[" + b.ToString() + "]";
                if (by.cls == "")
                {
                    Fail(def, byWhere + " hat kein \"cls\" - abgewiesen. Ein Nebenprodukt "
                        + "entsteht NEU; ein reiner Zustandswechsel hat dort kein Ziel.");
                    return false;
                }

                if (!CheckOutputClass(def, by.cls, byWhere))
                    return false;

                if (!CheckPortionFields(def, by, byWhere))
                    return false;

                tr.byproducts.Insert(by);
            }
        }

        if (tr.pureStateChange && tr.byproducts.Count() > 0)
        {
            Fail(def, "Ein reiner Zustandswechsel kann keine Nebenprodukte erzeugen - "
                + "abgewiesen. Er verbraucht nichts und erzeugt nichts; er setzt nur den "
                + "Zustand der Eingaenge (11 §2).");
            return false;
        }

        return true;
    }

    /**
     * Die Portionsfelder eines Transformergebnisses (15 §7).
     *
     * Woertlich dieselbe Aufgabe wie ChefZ_RecipeCompiler.CheckPortionFields,
     * und deshalb dieselben REGELN: sie stehen genau einmal, in
     * ChefZ_PortionOutputAudit. Was sich unterscheidet, sind nur der
     * Recordtyp und die Meldewege.
     */
    private bool CheckPortionFields(notnull ChefZ_TransformDef def,
                                    notnull ChefZ_OutputDef o,
                                    string where)
    {
        array<string> warnings = new array<string>();
        string portionClass;
        string rejectReason;

        if (!ChefZ_PortionOutputAudit.Audit(o, where, warnings, portionClass, rejectReason))
        {
            Fail(def, rejectReason);
            return false;
        }

        for (int w = 0; w < warnings.Count(); w++)
            Warn(def, warnings.Get(w));

        if (portionClass == "")
            return true;

        return CheckOutputClass(def, portionClass, where + ".portionClass");
    }

    /**
     * Die beiden Pruefungen, die CfgVehicles brauchen - identisch zum
     * Rezeptcompiler und aus demselben Grund (08 §8, 01 V7, V-B Auflage 4).
     *
     * Der Configzugriff selbst steht in ChefZ_VanillaNutrition, damit es fuer
     * dieselbe Engine-Bedingung nur EINE Nachbildung gibt (13 §3).
     */
    private bool CheckOutputClass(notnull ChefZ_TransformDef def, string cls, string where)
    {
        if (!m_VerifyClasses || !g_Game)
            return true;

        if (!ChefZ_VanillaNutrition.ClassExists(cls))
        {
            Fail(def, where + " nennt die Klasse \"" + cls + "\", die es in CfgVehicles nicht "
                + "gibt - Transform abgewiesen. Fehlt das Content-Modul, oder ist der Name "
                + "falsch geschrieben?");
            return false;
        }

        if (!ChefZ_VanillaNutrition.IsEdible(cls))
            return true;

        string reason;
        if (ChefZ_VanillaNutrition.WouldRegisterAtStomach(cls, reason))
            return true;

        Fail(def, where + ": die essbare Klasse \"" + cls + "\" " + reason
            + " - Transform abgewiesen. PlayerStomach.InitData registriert sie damit nicht, "
            + "und AddToStomach bricht beim Verzehr OHNE MELDUNG ab: das Ergebnis verschwaende, "
            + "ohne zu saettigen (01 V7).");
        return false;
    }

    /**
     * Ein reiner Zustandswechsel darf NICHTS verbrauchen.
     *
     * Das ist die Bedingung, die die Rohform nicht pruefen kann: sie sieht
     * erst nach ResolveDefaults der Slots, welcher Verbrauchsmodus gilt - und
     * der Default ist "whole".
     *
     * Ohne diese Pruefung waere der haeufigste Anfaengerfehler zugleich der
     * teuerste: ein Transform, der den Zustand setzen soll, loescht
     * stattdessen die Zutat und erzeugt nichts. Genau die
     * Zutatenvernichtungsmaschine aus 11 §7 - nur eine Ecke weiter.
     */
    private bool CheckStateChangeConsistency(notnull ChefZ_TransformDef def,
                                             notnull ChefZ_CompiledTransform tr)
    {
        if (!tr.pureStateChange)
            return true;

        for (int i = 0; i < tr.inputs.Count(); i++)
        {
            ChefZ_CompiledSlot slot = tr.inputs.Get(i);
            if (!slot)
                continue;
            if (slot.consumeMode == ChefZ_ConsumeMode.NONE)
                continue;

            Fail(def, "Eingang \"" + slot.slotId + "\" hat consume \""
                + ChefZ_ConsumeMode.Name(slot.consumeMode) + "\", der Transform ist aber ein "
                + "reiner Zustandswechsel (kein Ergebnis nennt eine Klasse) - abgewiesen. Er "
                + "wuerde die Zutat verbrauchen und nichts erzeugen. Setze \"consume\": \""
                + ChefZ_ConsumeMode.NONE_NAME + "\" an allen Eingaengen.");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Stationen, Feinjustierung, Faehigkeiten
    //--------------------------------------------------------------------------

    /**
     * stationsAllowed (11 E5, 11 §7).
     *
     * Ein unbekannter Eintrag ist ausdruecklich KEIN Fehler: "Der Transform
     * bleibt geladen, WARN einmal. Bewusst kein Fehler: die Station koennte
     * aus einem optionalen Modul kommen."
     *
     * Der Eintrag bleibt trotzdem in der Liste. Waere er entfernt worden und
     * die Liste dadurch leer, kippte die Bedeutung von "nur an diesen
     * Stationen" nach "an jeder Station" - aus einer Exklusivitaet wuerde
     * lautlos das Gegenteil.
     */
    private void CompileStations(notnull ChefZ_TransformDef def,
                                 notnull ChefZ_CompiledTransform tr,
                                 notnull map<int, int> stations)
    {
        tr.stationsAllowed.Clear();

        int declared = ChefZ_TextList.Count(def.stationsAllowed);
        for (int i = 0; i < declared; i++)
        {
            string name = def.stationsAllowed.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
            if (tr.stationsAllowed.Find(sym) < 0)
                tr.stationsAllowed.Insert(sym);

            if (!stations.Contains(sym))
            {
                Warn(def, "stationsAllowed[" + i.ToString() + "] nennt die Station \"" + name
                    + "\", die dieser Server nicht kennt. Der Transform bleibt geladen und der "
                    + "Eintrag steht weiter in der Liste - die Station kann aus einem "
                    + "optionalen Modul kommen (11 §7).");
            }
        }
    }

    private void CompileTuning(notnull ChefZ_TransformDef def,
                               notnull ChefZ_CompiledTransform tr,
                               notnull ChefZ_CompiledProcess proc)
    {
        if (def.HasDurationOverride())
        {
            float d = def.durationOverrideSec;
            if (d < 0.0)
            {
                Warn(def, "durationOverrideSec ist negativ (" + d.ToString()
                    + ") und wird ignoriert - es gilt die Prozessdauer von "
                    + proc.baseDurationSec.ToString() + "s.");
            }
            else
            {
                tr.durationOverrideSec = d;
            }
        }

        tr.qualityRule    = def.qualityRule;
        tr.freshnessCarry = def.freshnessCarry;
        tr.qualityDelta   = def.qualityDelta;
        tr.priority       = ClampPriority(def);

        if (tr.freshnessCarry < 0.0 || tr.freshnessCarry > 1.0)
        {
            Warn(def, "freshnessCarry = " + tr.freshnessCarry.ToString()
                + " liegt ausserhalb von [0, 1] und wird geklemmt. Ein Wert ueber 1 hiesse, "
                + "dass Verarbeiten eine Zutat FRISCHER macht - das waere ein Waschgang fuer "
                + "altes Fleisch (12 §4.1).");
            tr.freshnessCarry = Math.Clamp(tr.freshnessCarry, 0.0, 1.0);
        }
    }

    //! 09 §7: priority ausserhalb [-1000, 1000] wird geklemmt und gemeldet.
    private int ClampPriority(notnull ChefZ_TransformDef def)
    {
        int p = def.priority;
        if (ChefZ_Undefined.IsIntUndefined(p))
            return 0;

        if (p > 1000)
        {
            Warn(def, "priority = " + p.ToString() + " liegt ausserhalb von [-1000, 1000] und "
                + "wird auf 1000 geklemmt. Eine so grosse Zahl haette die berechnete "
                + "Spezifitaet faktisch abgeschaltet (09 §7).");
            return 1000;
        }
        if (p < -1000)
        {
            Warn(def, "priority = " + p.ToString() + " liegt ausserhalb von [-1000, 1000] und "
                + "wird auf -1000 geklemmt (09 §7).");
            return -1000;
        }
        return p;
    }

    private void CompileRequirements(notnull ChefZ_TransformDef def,
                                     notnull ChefZ_CompiledTransform tr)
    {
        tr.requires.Clear();
        if (!def.requires)
            return;

        for (int i = 0; i < def.requires.Count(); i++)
        {
            ChefZ_CapabilityReq req = def.requires.Get(i);
            if (!req)
                continue;
            if (req.capability == "")
            {
                Warn(def, "requires[" + i.ToString() + "] nennt keine \"capability\" - der "
                    + "Eintrag wird ausgelassen.");
                continue;
            }
            tr.requires.Insert(req);
        }
    }

    //--------------------------------------------------------------------------
    // Rangzahlen (09 §4.1)
    //--------------------------------------------------------------------------

    /**
     * Spezifitaet und Mindestitemzahl.
     *
     * Die Slotrechnung ist WOERTLICH dieselbe wie in
     * ChefZ_RecipeRanker.ComputeSpecificity - sie steht hier trotzdem
     * ausgeschrieben und nicht als Aufruf, weil der Ranker ein
     * ChefZ_CompiledRecipe erwartet und ein Transform keines ist. Ihn dafuer
     * auf eine gemeinsame Basisklasse umzubauen waere eine Aenderung am
     * Rezeptpfad fuer null Gewinn im Rezeptpfad.
     *
     * Der Kontextanteil der Rezeptformel entfaellt: ein Transform hat keine
     * Geraetekategorien und keine Temperaturbedingung - das steht im PROZESS,
     * nicht im Transform. Statt dessen zaehlt stationsAllowed mit demselben
     * Gewicht wie eine exakt genannte Geraeteklasse: beides ist dieselbe
     * Aussage ("nur hier").
     */
    private void ComputeIndexFacts(notnull ChefZ_CompiledTransform tr)
    {
        ChefZ_PriorityWeights w = Weights();

        float total = 0.0;
        int   minItems = 0;
        int   i;

        for (i = 0; i < tr.inputs.Count(); i++)
        {
            ChefZ_CompiledSlot slot = tr.inputs.Get(i);
            if (!slot)
                continue;

            if (!slot.optional && slot.minCount > 0)
            {
                int amount = slot.minCount;
                if (amount > w.amountCap)
                    amount = w.amountCap;
                if (amount < 1)
                    amount = 1;
                total = total + slot.specificity * amount;

                // Ein Item bedient hoechstens einen Slot (07 §4), also ist die
                // Summe der minCount die Mindestzahl.
                minItems = minItems + slot.minCount;
            }
            else
            {
                total = total + slot.specificity * w.wOptionalSlot;
            }
        }

        total = total + w.wContextDeviceClass * tr.stationsAllowed.Count();
        total = total + w.wCapability * tr.requires.Count();

        tr.specificity  = total;
        tr.minItemCount = minItems;
    }

    private ChefZ_PriorityWeights Weights()
    {
        if (m_Ctx)
            return m_Ctx.Weights();
        return new ChefZ_PriorityWeights();
    }

    //==========================================================================

    private void Fail(notnull ChefZ_Record rec, string message)
    {
        if (m_Report)
            m_Report.AddError(rec.sourceRef, rec.id, message);
    }

    private void Warn(notnull ChefZ_Record rec, string message)
    {
        if (m_Report)
            m_Report.AddWarn(rec.sourceRef, rec.id, message);
    }
}
