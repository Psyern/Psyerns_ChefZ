# Configuration

Complete reference of the ChefZ server settings.

Everything on this page is read from
`ChefZ_Core/Config/Core.json` and resolved by
`ChefZ_CoreSettingsDef` / `ChefZ_ConfigManager`.

**Count:** 45 top-level keys, of which 43 are plain settings and 2 are nested
blocks. The blocks contribute 19 (`priorityWeights`) and 5 (`qualityScoring`)
further keys. **67 individual settings in total.** Every one of them is described
below; recount from `ChefZ_Core/Config/Core.json` before quoting these numbers.

## 1. Where settings come from

Three ranks. A higher rank patches a lower one **field by field**.

| Rank | Source | Read by |
|---|---|---|
| 1 | `CfgChefZ*` class tree in each addon's `config.cpp` | server and client |
| 2 | JSON inside the PBO, listed in `CfgChefZ … dataFiles[]` | server and client |
| 3 | `$profile:ChefZ\Core.json` and `$profile:ChefZ\Overlay\*.json` | **server only** |

`coreSettings` records — that is, everything on this page — cannot come from
rank 1. There is no `CfgChefZ` node for them. They come from
`ChefZ_Core/Config/Core.json` (rank 2) and from the operator overlay (rank 3).

### The bootstrap exception

`ChefZ_Core/Config/Core.json` is read **before rank 1**, and it is the only
place where the rank order is broken. The reason is mechanical: `strictMode`
and `safeModeErrorThreshold` decide how the loader reacts to an error, so they
have to be in force before the first error can occur — and `logLevel` decides
whether you see the rest at all. After rank 3 has been read, the settings are
resolved **a second time**, because an overlay is allowed to patch them.

### Code defaults are the safety net

`coreSettings` is the only record kind that carries complete defaults in code.
If `Core.json` is missing entirely, that is a **warning, not an error** —
nothing is lost, every value falls back to the code default listed below.

## 2. Overriding without touching shipped files

Never edit `ChefZ_Core/Config/Core.json`. It lives inside a PBO and every
change costs a rebuild. Use rank 3 instead.

On the first server start ChefZ creates:

```
$profile:ChefZ\Core.json
$profile:ChefZ\README.txt
$profile:ChefZ\Overlay\
$profile:ChefZ\Logs\
```

`Core.json` is copied from `ChefZ_Core/Config/Templates/Core.overlay.json` and
contains the bare minimum:

```json
{
    "kind": "coreSettings",
    "schemaVersion": 1,
    "records": [
        {
            "id": "CORE"
        }
    ]
}
```

**ChefZ never overwrites a file that already exists.** It only creates missing
templates.

To change something, add only the fields you want to change:

```json
{
    "kind": "coreSettings",
    "schemaVersion": 1,
    "records": [
        {
            "id": "CORE",
            "logLevel": 4,
            "logChannels": ["MATCH", "COOK"],
            "cookActorRadius": 8.0,
            "globalSpoilageScale": 0.5
        }
    ]
}
```

### Rules that hurt if you do not know them

- **A partial file is the point.** What you do not name keeps its previous
  value. Do not copy the whole shipped `Core.json` into the overlay — you would
  freeze today's defaults forever.
- **Arrays replace, they do not merge.** Writing `"logChannels": ["MATCH"]`
  removes `ALL`. The same holds for `defaultExcludedStates`.
- **Nested blocks are replaced whole.** If your overlay writes `priorityWeights`
  at all, that block replaces the one from rank 2 entirely. *Inside* the block
  the field-wise patch still applies: a key you omit keeps its **code** default,
  not the value from the shipped `Core.json`.
- **States and quality tiers cannot be added here**, only their fields changed.
  They carry a network ordinal derived from rank 1 that must be identical on
  client and server.
- **Changes take effect on the next server start.** There is deliberately no
  runtime reload in V1 — reloading would reassign sync ordinals while items
  already carry state values. A second `LoadAll()` is ignored and says so:
  `LoadAll() wurde erneut aufgerufen und ignoriert.`
- **`id` must be `CORE`.** A document may contain several records; the one with
  `id == "CORE"` wins, otherwise the first one is taken.

### Booleans

`"enabled": false` works. You do not need `explicitFields`.

The engine's JSON reader cannot tell "absent" from "set to false", so ChefZ
parses each document **twice** — once with a `false` bias, once with `true` — and
records every field that came out the same as explicitly present. A hand-written
`explicitFields[]` array still works and behaves identically.

> The `README.txt` that ChefZ writes into `$profile:ChefZ` states that a boolean
> only takes effect if its name also appears in `explicitFields`. That text
> predates the probe mechanism and is **wrong**. The probe covers it.

## 3. What happens with a broken file

The failure doctrine is one sentence: *every error moves the system towards
less ChefZ, never towards wrong ChefZ.*

| Situation | Result | RPT |
|---|---|---|
| File missing | Code defaults apply | warning only |
| File exists but is empty / unreadable | File skipped | `Datei existiert, liefert aber keinen Inhalt - uebersprungen.` |
| `kind` field missing | **Whole file discarded** | `Feld "kind" fehlt im Dokumentkopf` |
| `kind` unknown | **Whole file discarded** | `Unbekannte Art "<x>" - Datei verworfen. Gueltig: …` |
| JSON does not parse | **Whole file discarded**, everything already loaded stays valid, the file is not touched | `JSON nicht lesbar - die gesamte Datei wird verworfen … Parsermeldung: <parser text>` |
| `schemaVersion` newer than the core | File **is** loaded, unknown fields ignored | warning |
| A value is out of range | Value **clamped**, never rejected | `<field> = <was> ist unbrauchbar - geklammert auf <now>.` |
| Unknown `capabilityMode` / `defaultExtraItems` | Falls back to the default, lists valid values | warning |
| Unknown log channel name | Ignored, does not block anything | `Unbekannter Logkanal "<x>" - ignoriert. Gueltig: …` |

A discarded file affects **only that file**. The lower rank stays in force.

At the end of the load, the health is decided:

| Health | Condition | Effect |
|---|---|---|
| `OK` | 0 errors | full operation |
| `DEGRADED` | errors ≤ `safeModeErrorThreshold` and `strictMode` off | the sound records run, the broken ones do not |
| `SAFE_MODE` | `strictMode` on with ≥ 1 error, **or** errors > threshold | **all ChefZ registries are emptied, the core is inert, vanilla cooking runs unchanged** |

## 4. Basic switches

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `enabled` | bool | `true` | `false` makes the core inert: nothing further is loaded, nothing frozen, one banner line in the RPT (`Core ist per Einstellung abgeschaltet (enabled=false) - Vanilla-Kochen laeuft unveraendert.`). Pure vanilla. |
| `strictMode` | bool | `false` | `true` sends the core into `SAFE_MODE` on the **first** error. The explicit emergency exit. |
| `safeModeErrorThreshold` | int | `25` | Number of load errors above which `SAFE_MODE` is entered. Clamped to a minimum of `1`. |

## 5. Logging

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `logLevel` | int | `2` (`WARN`) | `0` OFF · `1` ERROR · `2` WARN · `3` INFO · `4` DEBUG · `5` TRACE. Names are accepted too (`"INFO"`). A value outside `0..5` is clamped — **below the range it clamps to `ERROR`, not to `OFF`**, so a typo can never silence the log completely. |
| `logChannels` | string[] | `["ALL"]` | Channel bitmask. See the channel list below. Unknown names are ignored with a warning. |
| `logToFile` | bool | `false` | Additionally writes to `$profile:ChefZ\Logs\ChefZ_YYYY-MM-DD.log`. |
| `logServerOnly` | bool | `true` | Suppresses ChefZ log output on clients. |
| `logBufferLines` | int | `64` | Ring buffer size for the last lines. Minimum `1`. |
| `maxOnceKeys` | int | `512` | How many distinct "log this only once" keys are remembered. Minimum `1`. |
| `maxLogSizeMB` | int | `8` | Size cap of the log file. Minimum `1`. |
| `logReportToFile` | bool | `false` | Writes the full load report to `$profile:ChefZ\Logs\load_report.txt`. Server only. Turn this on when the RPT truncates the report — the RPT prints only the first entries and then says `… n weitere - vollstaendig in $profile:ChefZ\Logs\load_report.txt`. |

### Log channels

Thirteen channels. `ALL` covers all of them.

```
CORE  CONFIG  MATCH  COOK  PROCESS  STATE  QUALITY
NUTRI  PRESERV  PORTION  CONTAIN  EVENT  PERF
```

Two axes, not one: **level and channel mask**. A global `DEBUG` on a server
with dozens of simultaneous fireplaces is unusable — the interesting line drowns
in thousands. Say "only `MATCH` and `COOK`, at `DEBUG`" instead:

```json
{
    "kind": "coreSettings",
    "schemaVersion": 1,
    "records": [
        { "id": "CORE", "logLevel": 4, "logChannels": ["MATCH", "COOK"] }
    ]
}
```

### Line format in the RPT

```
[ChefZ][CHANNEL] message                    INFO / DEBUG / TRACE
[ChefZ][WARN][CHANNEL] message              WARN
[ChefZ][ERROR][CHANNEL] message             ERROR
```

`ERROR` and `WARN` carry their own tag; `INFO`, `DEBUG` and `TRACE` do not, so
block output stays readable. To find everything:

```
findstr /C:"[ChefZ]" DayZServer_x64_*.RPT
```

A handful of lines bypass the level check entirely, because an operator who has
to read them will not have a debug level switched on:

- the load summary `[ChefZ][CONFIG] slices=… health=… in …ms`
- `[ChefZ][CORE] Config SERVER  health=… rezepte=… aktiv=…`
- `[ChefZ][CORE] Aussenkante SERVER  abonnenten=… faehigkeitsanbieter=… fortschrittsempfaenger=… modus=…`
- `[ChefZ][CORE] Handwerk  rezepte=… plaetze=… ab Rezept-ID … kennsumme=…`
- the self-test summaries
- the `SAFE MODE` banner

### Runtime log changes

The log is the **only** part of the core whose configuration can be changed at
runtime — reproducing a bug should not cost a server restart. The parser exists
(`ChefZ_AdminCommands`):

```
chefz log status
chefz log level <0..5>
chefz log channel <name> on|off
chefz match <entityId>
chefz why <entityId> <recipeId>
chefz recipe <recipeId>
chefz registries | categories | symbols | ambiguities | audit | stats | report
```

> **However:** the core ships **no** entry point that reaches this parser.
> There is deliberately no chat hook, no RPC and no admin UI — vanilla does not
> route chat into script server-side, and the core has no RPC of its own by
> design. `ChefZ_AdminCommands.Execute()` is a plain function that any existing
> admin tool can call, and today the only caller in the repository is the
> self-test. Until an admin tool wires it up, change the log level in
> `$profile:ChefZ\Core.json` and restart. Runtime changes are never persistent
> anyway — after a restart `Core.json` applies again.

## 6. Matcher and selectors

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `matcherNodeBudget` | int | `4096` | Upper bound on nodes the matcher may visit while backtracking. Minimum `1`. Also used as the budget for transform matching. |
| `matcherCooldownSec` | float | `1.0` | Clamped at `0.0`. **Currently has no consumer in the runtime code** — see the note below. |
| `matchThrottleTicks` | int | `2` | How many cooking ticks must pass before an idle device is re-matched. Protects against a player cycling items in and out every second. A freshly filled pot is always evaluated immediately. `0` means no throttling. |
| `maxSelectorDepth` | int | `8` | Maximum nesting depth of a selector expression. Guards against cyclic copy-paste. Minimum `1`. |
| `maxCategories` | int | `256` | Cap on the category tree. Minimum `1`; a value `<= 0` falls back to the internal default. |

> `matcherCooldownSec` is declared, defaulted, patched and clamped, but no code
> outside `ChefZ_CoreSettingsDef` reads it. The design documents describe it as
> a time-based re-evaluation cooldown next to `matchThrottleTicks`; only the
> tick-based throttle was implemented. Setting it has no effect today.

## 7. Cooking sessions

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `sessionTtlSec` | float | `300.0` | Lifetime of a cooking session without a cooking tick. Without it the session map would grow unbounded over server uptime. **`0` disables ageing** — explicitly allowed for diagnosis and short test runs, explicitly not for production. Clamped at `0.0`. |
| `cookActorRadius` | float | `6.0` | Radius in metres in which a cooking device finds an acting player. **`0` disables attribution completely.** Hard cap `64.0`; a larger value is clamped and reported. |

### Why `cookActorRadius` exists

A cooking pot has no owner in vanilla, and ChefZ is not allowed to give it one
— `Pot`, `FryingPan`, `Cauldron`, `ItemBase`, `Edible_Base` and `FireplaceBase`
are on a closed list of classes ChefZ does not mod. So "who cooked this" is not
a stored fact but an **observation**: who was standing at the device when its
cargo grew. Vanilla's cooking loop never *adds* anything to a vessel, so cargo
growth means a player put something in.

Consequences you should know before changing the number:

- If two strangers stand at the device when cargo grows, the dish belongs to
  **nobody**. An existing claimant keeps his claim as long as he is in range.
  Nobody can take a claim by walking up.
- `cookActorRadius = 0` means every cooked dish has actor identity `0`, and
  every downstream consumer that needs an identity gets nothing. With the
  Terje Skills comp mod that means **cooking awards no XP at all**, because the
  sink discards progress reports with `identityId == 0`. See
  [Terje Compatibility](Terje-Compatibility).
- 6 m is far enough to stand next to a fireplace and move, tight enough that a
  passer-by does not count as the cook.

## 8. Spoilage and freshness

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `globalSpoilageScale` | float | `1.0` | Server-wide multiplier on decay. `<1` slower, `>1` faster. `0` is allowed and stops decay. Clamped at `0.0`. |
| `minDecayScale` | float | `0.01` | Lower clamp for any per-item decay scale. A value `<= 0` is clamped to `0.01`. |
| `maxDecayScale` | float | `10.0` | Upper clamp for any per-item decay scale. If it is below `minDecayScale` it is raised to it. |
| `defaultFreshnessLifetimeSec` | float | `21600.0` | Freshness lifetime for every state that does not name its own — and therefore for **every item without a state record**. `21600` is six hours, taken from vanilla's `DECAY_FOOD_RAW_MEAT`, its shortest decay. A value `<= 0` is legal and means "freeze freshness server-wide"; it is **not** clamped, but the preservation manager reports it at boot so a typo does not pass as a design decision. |

See [Food States](Food-States).

## 9. Recipes and quality

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `priorityScale` | float | `0.01` | Damping applied to a recipe's hand-written `priority` before it enters the score. A value `> 1.0` means the hand-written number overrules computed specificity — the core warns about it, because it disables the project's central rule ("the most specific valid recipe wins"). |
| `maxExternalQualityBonus` | float | `2.0` | Cap on the quality bonus a foreign mod may contribute through the event bus. `<= 0` means "no external bonus" and is a valid setting. Clamped at `0.0`. |
| `capabilityMode` | string | `"asAuthored"` | `asAuthored` — a `requires[]` behaves as written. `neverBlock` — a `block` failure is downgraded to `degrade`, so nothing is ever locked. `ignore` — **all** `requires[]` count as satisfied. Unknown values fall back to `asAuthored` with a warning. |
| `defaultExtraItems` | string | `"forbid"` | Policy for items in a vessel that no recipe slot binds. `forbid` — no ChefZ match, vanilla cooks on. `ignore` — the extra item is left alone. `consume` — it is consumed. Unknown values fall back to `forbid` with a warning. |
| `defaultExcludedStates` | string[] | `["BURNT", "ROTTEN"]` | States excluded from every slot that does not say otherwise. Not validated against the state registry — if no content defines them, they simply do nothing. |
| `allowTimedRecipes` | bool | `true` | `false` downgrades every `completion: "TIMED"` recipe to `ON_STAGE` with a warning instead of rejecting it. The operator's brake against recipes with their own clock. |
| `allowProfileOverlay` | bool | `true` | `false` stops rank 3 from being read. The directories and templates are still created (the log lives there too), and the RPT says `Overlay ist per Einstellung abgeschaltet (allowProfileOverlay=false)`. |

`defaultExtraItems: "forbid"` is the safe default because it falls back to
vanilla: a foreign item in the pot means "no ChefZ recipe", and vanilla cooks on
exactly as a player without ChefZ would expect. See [Recipes](Recipes).

## 10. `priorityWeights` — specificity scoring

The weights that turn a recipe's selectors into a specificity score. The block
lives in configuration, not in code, so an operator can retune ranking without
touching content.

`null`, i.e. block absent, means the code defaults apply completely. A partially
filled block acts partially.

| Key | Type | Default | Counts for |
|---|---|---|---|
| `wClass` | float | `3.00` | a slot naming an exact class |
| `wState` | float | `2.00` | a slot naming a state |
| `wTag` | float | `1.50` | a slot naming a tag |
| `wVanillaStage` | float | `1.50` | a slot naming a vanilla food stage |
| `wCategoryBase` | float | `1.00` | a slot naming a category |
| `wCategoryPerDepth` | float | `0.50` | per depth level — `WILD_MEAT` beats `MEAT` |
| `wNot` | float | `0.50` | a negated condition |
| `wRangePerBound` | float | `0.25` | per bound value range |
| `wMinQuality` | float | `0.75` | a `minQuality` condition |
| `wOptionalSlot` | float | `0.25` | optional slots count damped |
| `wContextDeviceClass` | float | `0.50` | per explicitly named device class |
| `wContextBound` | float | `0.25` | per temperature / liquid condition |
| `wPolicyForbid` | float | `0.50` | when `extraItems == "forbid"` |
| `wPolicyPerState` | float | `0.25` | per entry in `forbiddenStates` |
| `wCapability` | float | `0.25` | per entry in `requires[]` |
| `wToolGroup` | float | `0.25` | per entry in `requiredToolGroups` |
| `amountCap` | int | `3` | cap for `min(minCount, cap)` |
| `coverageBonus` | float | `0.50` | bonus for covering the vessel's contents |
| `priorityScale` | float | `0.01` | damping of the hand-written `priority` |

`priorityScale` appears **twice**: flat in section 9 and again inside this
block. Both are intentional. The flat value is the default; a value inside the
block overrides it — the more specific one wins.

Two states get a warning because they effectively switch off the ranking rule
and are almost never intended:

- `priorityScale > 1.0`
- all weights `0` (only the tiebreak decides: item count, slot count,
  `priority`, ID)

Negative weights are clamped — they would invert the ordering.

## 11. `qualityScoring` — quality calculation

Server-wide dials for how a dish's quality rank is computed. Quality *tiers*
are content and come from rank 1; the *weights* of the calculation are balancing
and must be adjustable without touching content.

`null` means the code defaults apply completely.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `defaultTierSet` | string | `"DISH_DEFAULT"` | Tier set used by a dish that names none |
| `freshnessWeight` | float | `1.0` | How strongly ingredient freshness moves the rank |
| `ingredientQualityWeight` | float | `0.5` | How strongly the ingredients' own quality moves the rank |
| `baseRank` | float | `1.0` | Starting rank before weights and penalties |
| `statePenalties` | array | shipped as `BURNT -3.0`, `ROTTEN -99.0` | Rank points per ingredient state. Code default is an **empty** list — the two entries come from the shipped `Core.json`. |

Warning case: if `freshnessWeight` and `ingredientQualityWeight` are both `0`
and there are no state penalties, ingredient condition no longer influences
quality at all — old meat gives the same dish as fresh. The core says so.

> **Known inconsistency:** the shipped `Core.json` sets
> `"defaultTierSet": "DISH_DEFAULT"`, but the core alone carries no quality
> tiers and cannot satisfy that default. It only becomes meaningful once a
> content module defines a tier set of that name.

See [Quality and Nutrition](Quality-and-Nutrition).

## 12. Nutrition audit

Four dials for a subsystem that does **nothing at runtime**. They control only
what appears in the log at server start — which is exactly why they belong in
the configuration: the audience is the operator, not the content author.

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `enableNutritionAudit` | bool | `true` | `false` drops the audit completely, for production servers that want a short start log. Leave it on: it is the only place where a dish without a `Nutrition` block ever becomes visible. |
| `nutritionTolerancePct` | float | `25.0` | Percentage deviation between target and actual above which the audit writes a `WARN`. **`0` means "report every deviation"** and is an explicitly valid setting; only a negative value is clamped. No correction is ever applied. |
| `nutritionAuditMaxFindings` | int | `64` | How many findings are logged individually before the audit summarises. Not a saving measure but a readability limit — the total still appears in the closing line, nothing is lost. Minimum `1`. |
| `nutritionExpectedCap` | float | `100000.0` | Probe limit of the target calculation. **Not a balancing cap** — the target value is never applied. It only catches an obviously derailed order of magnitude so the log carries one number instead of a column. A hit is an `INFO`, not an error. A value `<= 0` is clamped back to `100000.0`. |

## 13. Capabilities and events

Six dials for a layer that does nothing on a server without comp modules.

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `defaultCapabilityValue` | float | `0.0` | What a capability is worth while no provider answers. `0.0` is the honest default: without a skill mod nobody has a skill. Together with `onFail: "degrade"` that means "the dish is made, one grade worse". Clamped into `[capabilityMin, capabilityMax]`. **Deliberately not effective for quality rules with `when: "capability"`** — those count as unsatisfied without a provider, otherwise every player on every server without a skill mod would get the same bonus. |
| `capabilityMin` | float | `0.0` | Lower bound a provider's answer is clamped to. |
| `capabilityMax` | float | `10.0` | Upper bound. If it is below `capabilityMin` it is raised to it. The range is deliberately coarse — the core does not know which scale a foreign skill system uses. |
| `eventMaxDepth` | int | `3` | How deeply events may nest when a subscriber raises an event from inside a callback. Prevents an endless loop between two mods that throw events at each other. Minimum `1` — `0` would mean no delivery at all, and a comp module would silently never receive an event. |
| `eventTiming` | bool | `false` | Whether the bus measures duration per subscriber and reports outliers as `WARN`. Off by default: it costs two time queries per subscriber and event. Switch it on when hunting a hanging mod. |
| `eventSlowSubscriberMs` | int | `5` | Threshold in milliseconds above which a subscriber counts as an outlier. Only effective with `eventTiming = true`. Minimum `1`. |

## 14. Portions and containers

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `defaultTakePortionSec` | float | `2.0` | Default duration of the take-a-portion action, for any portioned dish that does not specify its own `takeDurationSec`. `0` is explicitly allowed ("almost invisible"); negative is clamped to `0.0`. |
| `containerSearchRadius` | float | `3.0` | Radius of the **environment** stage of the container search. Affects only the third search stage (`NEARBY_CARGO`), and that stage is **off by default** — a container is only looked for in nearby crates if its declaration carries `searchScope 4`. On a server without such containers this number does nothing. `0` disables the stage without touching a single declaration. Negative is clamped to `0.0`. |
| `maxContainerCandidates` | int | `32` | Cap on found containers per search. A pure protective cap: only the first entry is ever used, and the search stages run in fixed order, so the best choice is in front of the cap and not behind it. Minimum `1` — below that every container condition would fail without a log line. It matters because the search runs on every crosshair target change. |

See [Portions and Containers](Portions-and-Containers).

## 15. Full reference file

The shipped `ChefZ_Core/Config/Core.json`, for copy-and-adapt into
`$profile:ChefZ\Core.json`. **Do not paste this whole file into your overlay** —
take only the lines you actually want to change.

```json
{
    "kind": "coreSettings",
    "schemaVersion": 1,
    "records": [
        {
            "id": "CORE",

            "enabled": true,
            "strictMode": false,
            "safeModeErrorThreshold": 25,

            "logLevel": 2,
            "logChannels": ["ALL"],
            "logToFile": false,
            "logServerOnly": true,
            "logBufferLines": 64,
            "maxOnceKeys": 512,
            "maxLogSizeMB": 8,
            "logReportToFile": false,

            "matcherNodeBudget": 4096,
            "matcherCooldownSec": 1.0,
            "matchThrottleTicks": 2,
            "maxSelectorDepth": 8,
            "maxCategories": 256,
            "sessionTtlSec": 300.0,
            "cookActorRadius": 6.0,

            "globalSpoilageScale": 1.0,
            "minDecayScale": 0.01,
            "maxDecayScale": 10.0,
            "defaultFreshnessLifetimeSec": 21600.0,

            "priorityScale": 0.01,
            "maxExternalQualityBonus": 2.0,
            "capabilityMode": "asAuthored",

            "defaultTakePortionSec": 2.0,

            "containerSearchRadius": 3.0,
            "maxContainerCandidates": 32,

            "defaultCapabilityValue": 0.0,
            "capabilityMin": 0.0,
            "capabilityMax": 10.0,

            "eventMaxDepth": 3,
            "eventTiming": false,
            "eventSlowSubscriberMs": 5,

            "priorityWeights": {
                "wClass": 3.00,
                "wState": 2.00,
                "wTag": 1.50,
                "wVanillaStage": 1.50,
                "wCategoryBase": 1.00,
                "wCategoryPerDepth": 0.50,
                "wNot": 0.50,
                "wRangePerBound": 0.25,
                "wMinQuality": 0.75,
                "wOptionalSlot": 0.25,
                "wContextDeviceClass": 0.50,
                "wContextBound": 0.25,
                "wPolicyForbid": 0.50,
                "wPolicyPerState": 0.25,
                "wCapability": 0.25,
                "wToolGroup": 0.25,
                "amountCap": 3,
                "coverageBonus": 0.50,
                "priorityScale": 0.01
            },

            "qualityScoring": {
                "defaultTierSet": "DISH_DEFAULT",
                "freshnessWeight": 1.0,
                "ingredientQualityWeight": 0.5,
                "baseRank": 1.0,
                "statePenalties": [
                    { "state": "BURNT",  "points": -3.0 },
                    { "state": "ROTTEN", "points": -99.0 }
                ]
            },

            "enableNutritionAudit": true,
            "nutritionTolerancePct": 25.0,
            "nutritionAuditMaxFindings": 64,
            "nutritionExpectedCap": 100000.0,

            "defaultExtraItems": "forbid",
            "defaultExcludedStates": ["BURNT", "ROTTEN"],

            "allowProfileOverlay": true,
            "allowTimedRecipes": true
        }
    ]
}
```

## 16. Recipes for common goals

**Slow down all spoilage by half**

```json
{ "id": "CORE", "globalSpoilageScale": 0.5 }
```

**Debug why a recipe does not fire**

```json
{ "id": "CORE", "logLevel": 5, "logChannels": ["MATCH", "COOK"], "logToFile": true }
```

**Full load report to a file because the RPT truncates it**

```json
{ "id": "CORE", "logReportToFile": true, "logLevel": 3 }
```

**Run a skill-gated server without a skill mod, at full quality**

```json
{ "id": "CORE", "capabilityMode": "ignore" }
```

**Turn ChefZ off without unloading the mod**

```json
{ "id": "CORE", "enabled": false }
```

**Halt on the first data error instead of running degraded**

```json
{ "id": "CORE", "strictMode": true }
```

## Next

- [Troubleshooting](Troubleshooting) — what to grep for when something is wrong
- [Validation](Validation) — catch data errors before the server sees them
- [Architecture](Architecture) — why the configuration is shaped this way
