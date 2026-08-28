//==============================================================================
// ChefZ_Apiary - die Skriptklassen der Imkerei (Slice "apiary").
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// Processing/ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_Beehive : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Beehive" }] }
//     Skript       class ChefZ_Beehive extends ChefZ_ProcessingStation_Base {}
//
// "Mehr ist nicht noetig. Kein Core-Code, keine eigene Action, keine eigene
// Persistenz." Genau deshalb steht hier nichts ausser den Bindungen.
//
// ---------------------------------------------------------------------------
// WAS HIER BEWUSST NICHT STEHT
// ---------------------------------------------------------------------------
// KEIN ChefZ_HasHeat. Der Trockenrahmen laesst die Basisantwort "nein"
// stehen, weil Trocknen keine Waerme braucht; hier gilt dasselbe aus demselben
// Grund. Kein Prozess dieses Slice setzt requiresHeat.
//
// KEINE eigene Action. Der Auftrag verlangt, dass die Imkerpfeife beim Oeffnen
// des Stocks Schaden abhaelt. Das braeuchte einen Punkt im Ablauf, an dem der
// handelnde SPIELER und das Item in seiner Hand gleichzeitig bekannt sind.
// An einer ChefZ-Station gibt es diesen Punkt nicht:
//
//   - ChefZ_ActionProcessAtStation.OnFinishProgressServer reicht an die
//     Station nur (ItemBase inHands, int actorId) weiter. actorId ist
//     PlayerIdentity.GetPlayerId() - eine Zahl ohne Rueckweg zum PlayerBase.
//     Und im Straffall, also mit leeren Haenden, ist inHands null; es bliebe
//     kein Zeiger auf den Spieler uebrig.
//   - Ein STATION_ACTION laeuft ohnehin nicht ueber ChefZ_BeginJob, sondern
//     ueber RunImmediate. Ein Ueberschreiben von ChefZ_BeginJob in dieser
//     Klasse wuerde bei PROCESS_HARVEST_HIVE nie zuenden.
//
// Vanillas eigene Vorlage - CAContinuousMineWood.DamagePlayersHands(),
// Handschuhe federn den Schaden ab, sonst Blutung - sitzt in einer EIGENEN
// Actionkomponente. Sie nachzubauen hiesse, eine zweite Action neben
// ChefZ_ActionProcessAtStation zu stellen; kein anderes Content-Modul dieses
// Projekts schreibt eine eigene Action, alle benutzen nur SetActions() zum
// Anhaengen vorhandener. Der Slice waehlt deshalb den Weg, den das Projekt
// schon hat: PROCESS_HARVEST_HIVE fuehrt die Werkzeuggruppe BEE_SMOKER, und
// ohne Pfeife erscheint die Aktion nicht. Strenger als der Auftrag, aber nie
// irrefuehrend.
//
// KEINE Essaktion. Keine Klasse dieses Slice ist Nahrung - das Ergebnis der
// Kette ist Vanillas Honey. Die Begruendung steht an ChefZ_HoneycombFrame_Base
// in der config.cpp.
//
// Der Stock fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

//! Der Bienenstock (Auftrag: "Bienenstock"). Was er anbietet, steht im
//! Stationsdatensatz (Config/Processing/Apiary_Stations.json), was woraus
//! wird im Transform (Config/Processing/Apiary_Hive.json) - nicht hier.
class ChefZ_Beehive extends ChefZ_ProcessingStation_Base {}

//! Der Bausatz (Auftrag: "Beehive_Kit"). Reines Traggut ohne ChefZ-Zustand;
//! er wird von TR_RaiseBeehive verbraucht und ist bis dahin nur schwer.
class ChefZ_BeehiveKit extends ItemBase {}

//! Gemeinsame Skriptbasis der vier Raehmchen. Sie traegt bewusst KEINEN
//! ChefZ-Zustand: der Unterschied zwischen leer, verdeckelt, voll und
//! entdeckelt ist die KLASSE, nicht eine Zustandsvariable auf einer Klasse.
//! Vier Klassen sind hier richtiger als eine mit vier Zustaenden, weil jeder
//! Schritt ein eigener Transform mit eigenem Ein- und Ausgang ist und die
//! Stufen verschiedene Gewichte und Beschreibungen tragen.
class ChefZ_HoneycombFrame_Base extends ItemBase {}

//! Auftrag: "Honigwabe_Leer" / "Honeycomb_Frame_Empty".
class ChefZ_HoneycombFrameEmpty extends ChefZ_HoneycombFrame_Base {}

//! Verdeckelt und im Stock. Kein Auftragsname - die Begruendung fuer diesen
//! vierten Zustand steht an der config.cpp.
class ChefZ_HoneycombFrameSealed extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full".
class ChefZ_HoneycombFrameFull extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Frame_Ready_To_Spin".
class ChefZ_HoneycombFrameUncapped extends ChefZ_HoneycombFrame_Base {}

//! Die Entdeckelungsgabel (Auftrag: "Uncapping_Fork"). Reines Werkzeug -
//! sie wird nie verbraucht, nur ueber die Werkzeuggruppe UNCAPPING_TOOL
//! gefunden und ueber toolDamage an PROCESS_UNCAP_COMB abgenutzt. Dieselbe
//! Bauart wie ChefZ_PastaMachine in ChefZ_Processing.
class ChefZ_UncappingFork extends ItemBase {}

//! Die Imkerpfeife (Auftrag: "Smoker"). Ebenfalls reines Werkzeug, gefunden
//! ueber die Werkzeuggruppe BEE_SMOKER.
//!
//! NICHT zu verwechseln mit ChefZ_Smoker aus ChefZ_Processing - das ist der
//! Raeucherschrank der Konservierungskette.
class ChefZ_BeeSmoker extends ItemBase {}
