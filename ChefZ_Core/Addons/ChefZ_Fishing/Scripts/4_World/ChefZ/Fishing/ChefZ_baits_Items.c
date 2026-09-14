//==============================================================================
// ### SLICE baits ###   Skriptklassen der sieben Koeder.
//
// Je Configklasse eine Skriptklasse, und sonst nichts. Das ist kein Platzhalter,
// sondern die Bindung: ohne sie faellt die Klasse auf die Skriptklasse ihres
// Config-Vorfahren zurueck, und das ist hier ItemBase - richtig, aber nicht
// benannt. Benannt zu sein ist der Unterschied zwischen "ein Item" und "DIESES
// Item", sobald irgendwo ein Cast oder ein isinherited laeuft.
//
// ---------------------------------------------------------------------------
// WARUM HIER KEIN VERHALTEN STEHT
// ---------------------------------------------------------------------------
// Ein Koeder tut aus eigener Kraft nichts. Alles, was ihn wirksam macht, liegt
// woanders und gehoert dorthin:
//
//   - WELCHEN FISCH er hebt und um wieviel: Config/Fishing/Baits.json,
//     gelesen von ChefZ_FishingRegistry. Der Ertrag fragt den Kontext, nicht
//     der Koeder den Fisch.
//   - OB er den Zyklus ueberlebt: das eine Override in
//     ChefZ_Core/Scripts/4_World/ChefZ/Fishing/ChefZ_ModdedCatchingContext.c.
//   - WO er ans Geraet kommt: inventorySlot[] = {"Bait"} in der config.cpp,
//     abgefragt von CatchingContextFishingRodAction.InitItemValues.
//
// Ein override in dieser Datei waere eine vierte Stelle - und die erste, die
// bei der naechsten Engine-Version stillschweigend danebengreift.
//
// ---------------------------------------------------------------------------
// WARUM NICHT BaitBase UND WARUM NICHT Edible_Base
// ---------------------------------------------------------------------------
// BaitBase loescht sich beim ersten Ortswechsel selbst (FishingConsumables.c,
// "Obsolete item prison"); die Begruendung steht ausfuehrlich im Kopf der
// config.cpp. Edible_Base waere die Klasse eines Wurms - essbar, verderblich,
// mit Garstufen. Ein Gummifisch ist nichts davon.
//
// Bleibt ItemBase, die Skriptklasse hinter Inventory_Base.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der sieben Koeder. Entspricht der Configklasse
//! gleichen Namens (scope = 0, sie ist selbst kein Gegenstand).
class ChefZ_Bait_Base extends ItemBase
{
}

//! Basis der Kunstkoeder (baitKind "lure"). Sie bleiben am Haken.
class ChefZ_BaitLure_Base extends ChefZ_Bait_Base
{
}

//! Basis der Weichkoeder (baitKind "soft"). Sie werden verbraucht.
class ChefZ_BaitSoft_Base extends ChefZ_Bait_Base
{
}

class ChefZ_AntiqueSpoonFlasher extends ChefZ_BaitLure_Base
{
}

class ChefZ_MinnowLure extends ChefZ_BaitLure_Base
{
}

class ChefZ_RedTopperBait extends ChefZ_BaitLure_Base
{
}

class ChefZ_Spinnerbait extends ChefZ_BaitLure_Base
{
}

class ChefZ_BlueBait extends ChefZ_BaitSoft_Base
{
}

class ChefZ_PaddleTailBait extends ChefZ_BaitSoft_Base
{
}

class ChefZ_TroutBait extends ChefZ_BaitSoft_Base
{
}
