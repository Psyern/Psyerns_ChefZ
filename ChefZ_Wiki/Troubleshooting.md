# Troubleshooting

Every case here has actually happened during development. Each one is written as
**symptom → cause → check → fix**, and each names **what to search for in the
RPT**. That is the difference between a useful troubleshooting page and a
decorative one.

---

## 0. Start here: read the boot block

Before chasing any individual symptom, find the ChefZ boot block in the server
RPT. These lines bypass the log level check and are always present.

```
findstr /C:"[ChefZ]" DayZServer_x64_*.RPT
```

You are looking for four lines:

```
[ChefZ][CONFIG] slices=9 files=27 records=812 ok=812 rejected=0 patched=14 health=OK in 214ms
[ChefZ][CORE] Config SERVER  health=OK  records=812  kategorien=61  zutaten=118  zustaende=9  stufen=6  rezepte=57  haltbarkeit=22  naehrwerte=118  prozesse=21  stationen=9  transforms=94  werkzeuggruppen=7  handwerksrezepte=19/19  portionsgerichte=32  behaelter=5  behaelterkategorien=3  aktiv=1
[ChefZ][CORE] Aussenkante SERVER  abonnenten=2  faehigkeitsanbieter=1  fortschrittsempfaenger=1  modus=asAuthored
[ChefZ][CORE] Handwerk  rezepte=19  abgewiesen=0  plaetze=19  ab Rezept-ID 214  kennsumme=884213  (alle vier muessen auf Client und Server gleich sein)
```

(The numbers are illustrative; yours will differ.)

Read them like this:

| Field | If it is wrong |
|---|---|
| `slices=0` | No addon registered through `CfgChefZ`. Nothing ChefZ is loaded. → Case 7 |
| `files=0` | No JSON was readable. → Case 7 |
| `rejected=<n>` above 0 | `n` records were thrown away. The reasons are above this line in the report. |
| `health=DEGRADED` | Errors below the safe-mode threshold. Some things work, some do not. |
| `health=SAFE_MODE` | **All registries emptied.** ChefZ is inert, vanilla cooking is unaffected. → Case 8 |
| `aktiv=0` | Core is off — either `enabled=false` or safe mode. |
| `rezepte=0` | No compiled recipes. Cooking will never produce a ChefZ dish. |
| `zutaten=0` | No ingredient bindings. Every selector fails. |
| `abonnenten=0`, `faehigkeitsanbieter=0`, `fortschrittsempfaenger=0` | No comp module registered. The cause is in the other mod, not in ChefZ. → Case 8 |
| `handwerksrezepte=0/0` or `abgewiesen>0` | → Case 4 and Case 5 |

If the whole block is absent, ChefZ never booted. Check that the PBOs are in
`-mod` and not `-serverMod`, and that the `$PREFIX$` survived packing — a wrong
prefix makes DayZ skip the script module **silently, with no RPT line at all**.

---

## 1. Raw key names in the UI instead of a name

**Symptom** — An item shows `STR_CHEFZ_ITEM_BREAD0` in the inventory, in the
crafting menu, or in an action prompt.

**Cause** — The stringtable entry is missing, or the CSV column set is
incomplete, or `stringtable.csv` did not end up where DayZ looks for it. DayZ
falls back to the raw key when a language column is missing. **The engine does
not report this.** It just stands there.

**Check**

1. There is no RPT line. Do not look for one.
2. Run the stringtable validator:
   ```
   node tools/chefz-validate/index.mjs --only=stringtable
   ```
   It checks that every `#STR_CHEFZ_*` referenced anywhere is defined, that
   there are no duplicates, and that the full column set is present.
3. Verify the file landed in the PBO at `<Prefix>/Language/stringtable.csv`.
4. Check the header row against the fifteen columns DayZ expects:
   ```
   "Language","original","english","czech","german","russian","polish","hungarian",
   "italian","spanish","french","chinese","japanese","portuguese","chinesesimp",
   ```

**Fix** — Add the missing key or column and repack. If the validator is green
but the key still shows raw in-game, the file is not being packed: check
`include.txt` contains `*.csv`. `Psyerns_ChefZ_COT_Comp` has no `include.txt` at
all — see [Installation](Installation).

---

## 2. A dish never finishes cooking

**Symptom** — Ingredients go into the pot, the pot heats, and nothing happens.
The items stay raw forever, or they go straight to burnt without ever becoming
the dish.

**Cause** — One of three, all of which look identical in-game:

- **(a)** An ingredient class declares `Food > FoodStages` but no
  `FoodStageTransitions`. The transitions are dead text; vanilla's
  `Cooking.ProcessItemToCook` walks past the item.
- **(b)** No class in the item's script chain switches cookability on.
  `CanBeCooked()` defaults to `false` in vanilla. This is the bug that put every
  ChefZ ingredient with a ChefZ script class permanently on `Raw` — and the
  validator output was byte-identical before and after the bug.
- **(c)** The script class says `CanBeCooked() == true` but the config class has
  no `Food > FoodStages`. Then no `FoodStage` object exists and the cooking tick
  hits nothing.

**Check**

- **The RPT is silent for all three.** This failure mode produces no error
  line — which is exactly why two static validators exist for it:
  ```
  node tools/chefz-validate/index.mjs --only=chefzstage,chefzcookable
  ```
  `chefzstage` catches (a). `chefzcookable` catches (a), (b) and (c) as rules
  A, B and C.
- If both are green, the recipe itself may not be matching. Raise the log:
  ```json
  { "id": "CORE", "logLevel": 5, "logChannels": ["MATCH", "COOK"] }
  ```
  and restart. Then search the RPT for `[ChefZ][MATCH]` and `[ChefZ][COOK]`.
- If the recipe uses `completion: "TIMED"` and the session never accrues time,
  check `sessionTtlSec` — a session older than the TTL without a cooking tick is
  dropped.

**Fix** — Add the missing `FoodStageTransitions`, or the missing `FoodStages`,
or the cookability override in the script chain. See [Food States](Food-States).

---

## 3. A food item cannot be eaten

**Symptom** — The item exists, has nutrition values, and offers no "Eat" action.

**Cause** — No class in the item's script chain registers an eat action. Vanilla
does **not** put this on `Edible_Base`; it puts it on each food class
individually (`Potato.c` adds `ActionEatFruit`). Without one, the item is simply
never offered for eating — again with no error picture.

A second, related cause: the class has neither `class Nutrition` nor
`class Food`, or its `scope` is `0`. Then `PlayerStomach` never registers it,
`GetIDFromClassname` returns `-1`, and `AddToStomach` returns silently. The bite
animation may even play — and do nothing.

**Check**

- No RPT line. Static validators only:
  ```
  node tools/chefz-validate/index.mjs --only=chefznut,chefzcookable
  ```
  `chefznut` enforces "every edible result and portion class has `Nutrition` or
  `Food` **and** `scope != 0`". `chefzcookable` rule C enforces "a food class
  registers an eat action".
- `ActionForceFeed` deliberately does **not** count — that is another player
  feeding you.
- At runtime, switch on the nutrition audit and read the start log:
  ```json
  { "id": "CORE", "enableNutritionAudit": true, "logLevel": 3, "logChannels": ["NUTRI"] }
  ```
  Search for `[ChefZ][NUTRI]`. The audit is the only place where a dish without
  a nutrition block ever becomes visible.

**Fix** — Add `AddAction(ActionEat…)` in the script class, or add the
`Nutrition`/`Food` block and a non-zero `scope`.

---

## 4. A crafting recipe does not appear

**Symptom** — Two items that should combine by hand offer no craft action.

**Cause** — The slice does not declare `handcraftRecipeSlots` in its `CfgChefZ`
node, or declares too few. ChefZ reserves its recipe positions in vanilla's list
from `RegisterRecipies()`, in the constructor, **before any data exists**. The
number of positions has to be known at that moment, and the only source that is
provably identical on client and server at that point is the engine config. So
the count is a **declaration**, not something derived from the data.

If a slice declares fewer slots than it has `HANDCRAFT` transforms, the surplus
transforms are **rejected loudly** — deliberately, because a missing recipe that
appears in the RPT costs one config line, while a recipe producing the wrong
item costs trust.

**Check** — Search the RPT for:

```
Kein Slice meldet handcraftRecipeSlots an
```
No slice declares any slots at all. Vanilla crafting is untouched, and ChefZ
offers no handcraft recipe.

```
ueberzaehligen werden NICHT angeboten
```
Some transforms were rejected because the declared count is too low. The
transform IDs are named in the same line.

```
Es sind keine Rezeptplaetze verankert - RegisterRecipies() hat den ChefZ-Teil der Kette nicht erreicht
```
The modded-class chain did not reach ChefZ. Another mod broke the chain, or
ChefZ's script module was not loaded (prefix problem).

```
PluginRecipesManager ist nicht erreichbar
```
The plugin could not be resolved. Handcraft recipes are not offered; vanilla
crafting is unaffected.

Also check the boot line:
```
[ChefZ][CORE] Handwerk  rezepte=<filled>  abgewiesen=<rejected>  plaetze=<slots>
```
`abgewiesen > 0` is case 4. `plaetze=0` is case 4 too.

**Fix** — In the addon's `config.cpp`, inside its `CfgChefZ` node:

```cpp
class CfgChefZ
{
    class ChefZ_Meat
    {
        chefzApiVersion = 1;
        loadOrder = 200;
        handcraftRecipeSlots = 1;   // exactly the number of HANDCRAFT transforms
        dataFiles[] = { ... };
    };
};
```

The number must equal the number of `HANDCRAFT` transforms that slice
contributes. The global cap is 256 — a larger sum is a typo and is clamped with
an error.

**One more possibility for the same symptom:** vanilla builds the craft action
**client-side**, from `PluginRecipesManager.GetValidRecipes()`. If the client
cannot read the transform JSON out of the PBO, it never parameterises the
recipe, and the action never appears — no matter what the server knows. Whether
PBO-internal JSON is client-readable is still an **open measurement**; the
`[ChefZ][V-A]` smoke test block in the RPT is what answers it. See
[Known Limitations](Known-Limitations).

---

## 5. Crafting produces the wrong item, or nothing

**Symptom** — A craft action runs and either yields something unrelated, or
runs and produces nothing at all. Sometimes another mod's crafting breaks too,
without the player having touched a ChefZ recipe.

**Cause** — **Recipe ID drift.** Vanilla assigns recipe IDs by *position* in a
single list. If a foreign mod registers its recipes at a different moment on the
client than on the server — for example from the constructor on one side and
after load on the other — the two lists are offset against each other. Then the
client's ChefZ recipes point at foreign recipes on the server, and the client's
foreign recipes point at *other* foreign recipes. The second half is the worse
one: ChefZ would have broken someone else's crafting.

ChefZ addresses this with two independent nets:

1. **Position anchor** — ChefZ registers empty placeholder recipes from
   `RegisterRecipies()`, at the same point in the modded-class chain on both
   sides. The count comes from `CfgChefZ handcraftRecipeSlots`, the only source
   proven identical on both sides at constructor time. The placeholders are
   parameterised later from `ChefZ_Boot`; no position ever moves.
2. **Identity check on the wire** — `ChefZ_CraftIntent`: the client sends a
   position-**independent** identifier along with the position. The server holds
   it against the recipe sitting at that position on its side and **refuses** on
   disagreement. The action then does nothing — it never produces the wrong
   result.

**Check** — Compare the boot line in the **client** RPT and the **server** RPT:

```
[ChefZ][CORE] Handwerk  rezepte=19  abgewiesen=0  plaetze=19  ab Rezept-ID 214  kennsumme=884213
```

**All four numbers must be identical.** If `ab Rezept-ID` or `kennsumme`
differs, you have drift.

For refusals, search for:

```
Eine Craftaktion wurde VERWEIGERT: der Client meinte
```

The line names what the client meant, the recipe ID, and what stands there on
the server.

**Fix**

- Make the mod sets identical on client and server. A client with an extra addon
  is the most common cause.
- A `HANDCRAFT` transform introduced **only by a rank-3 overlay**
  (`$profile:ChefZ\Overlay\*.json`) takes a slot server-side that the client
  fills differently. Do not introduce handcraft transforms through the overlay.
- If `abgewiesen > 0` on one side and `0` on the other, fix
  `handcraftRecipeSlots` first (Case 4) and re-check.

---

## 6. Vanilla cooking behaves unexpectedly

**Symptom** — A player puts ordinary vanilla ingredients in a pan and gets a
ChefZ dish, or gets nothing where vanilla would have cooked the items normally.

This breaks the mod's central design rule: **if no ChefZ recipe matches, DayZ
cooks on exactly as it always did.**

**Cause** — Two directions:

- **(a) A ChefZ recipe fires too early.** A recipe that can be satisfied
  *entirely with vanilla ingredients* takes over vanilla cooking. Put three
  vanilla mushrooms in a pan and you get a ChefZ dish — without ever having
  touched anything from ChefZ.
- **(b) `defaultExtraItems` is not `forbid`.** With `ignore` or `consume`, an
  unbound foreign item in the vessel no longer blocks a ChefZ match, so a recipe
  matches where the safe default would have handed control back to vanilla.

**Check**

- For (a), the static rule is mechanical:
  ```
  node tools/chefz-validate/index.mjs --only=chefzvanilla
  ```
  `chefzvanilla` enforces invariant I2 and flags any recipe that is
  vanilla-satisfiable. It knows no threshold — whether it happens often in play
  is a human judgement.
- For (b), check your effective `defaultExtraItems`. The default is `forbid`
  precisely because it falls back to vanilla.
- At runtime, raise `MATCH` to `DEBUG` and watch which recipe wins:
  ```json
  { "id": "CORE", "logLevel": 4, "logChannels": ["MATCH"] }
  ```
  The diagnostics module is built for exactly this question and its refusal
  line is a constant in the code:
  ```
  Kein Treffer -> Vanilla-Kochen laeuft unveraendert weiter.
  ```
  Seeing that line for a vanilla-only pot is the **correct** behaviour.

**Fix** — Give the recipe at least one ChefZ-only ingredient slot, or raise its
specificity so it cannot be satisfied by vanilla alone. Set `defaultExtraItems`
back to `forbid`.

---

## 7. A whole module is missing in-game

**Symptom** — Every item, recipe and station of one module is absent. Other
modules work fine.

**Cause** — The module's JSON was discarded as a whole. A single unknown field,
a missing `kind`, or any parse error throws away **the entire file** — nothing
is applied half-way. That is deliberate: a half-applied data file is worse than
none.

**Check** — Search the RPT for the file name, or for these strings:

```
JSON nicht lesbar - die gesamte Datei wird verworfen
```
Followed by `Parsermeldung: <text>` — the parser's own message names the
problem.

```
Feld "kind" fehlt im Dokumentkopf
```
The document header has no `kind`, so the record type is undetermined.

```
Unbekannte Art "<x>" - Datei verworfen. Gueltig: …
```
Typo in `kind`. Valid names are listed in the same line.

```
Datei existiert, liefert aber keinen Inhalt - uebersprungen.
```
File present but empty or unreadable inside the PBO.

Also check the summary line: `records=` versus `ok=` versus `rejected=`.

If the RPT truncates the report, switch it to a file:

```json
{ "id": "CORE", "logReportToFile": true }
```

and read `$profile:ChefZ\Logs\load_report.txt`.

**Fix** — Run the schema validator, which catches unknown fields, missing
mandatory fields, wrong types and duplicate recipe IDs before the server ever
sees them:

```
node tools/chefz-validate/index.mjs --only=schema
```

> **A warning about "unknown fields are ignored".** The failure doctrine assumes
> the engine's JSON deserialiser ignores fields it does not know. That
> assumption is **not proven anywhere in this project** — which is why the
> shipped overlay template deliberately contains no explanatory extra fields.
> Treat an unknown field as a file-killer until measured otherwise.

**If the module is missing entirely and there is no error at all:** the module
never registered. Check `slices=` in the summary. A missing `CfgChefZ` node, or
a script module whose `dir` does not match the PBO prefix, makes DayZ skip the
module **without any RPT entry**.

---

## 8. Cooking XP never arrives

**Symptom** — With `Psyerns_ChefZ_Terje_Skills_Comp` loaded, processing at
stations awards XP but cooking a dish awards nothing.

**Cause** — Attribution. The core does not award XP; it reports completions and
a comp module turns them into XP. A completion report carries an actor identity,
and the Terje sink discards any report with `identityId == 0`:

```c
if (args.identityId == 0)
    return;
```

For **cooking**, the identity is not a stored fact. A vanilla pot has no owner
and ChefZ is not allowed to give it one. The identity is an **observation**: who
stood at the device in the tick when its cargo grew. Three ways it ends up `0`:

- `cookActorRadius` is set to `0` — attribution is switched off entirely.
- Nobody was within `cookActorRadius` when the ingredients went in.
- **Two or more strangers** were in range at that moment. Then the dish belongs
  to nobody, by design — so that a claim cannot be taken by walking up. An
  existing claimant keeps his claim as long as he stays in range.

**Check**

1. Boot line:
   ```
   [ChefZ][CORE] Aussenkante SERVER  abonnenten=…  faehigkeitsanbieter=…  fortschrittsempfaenger=…  modus=…
   ```
   `fortschrittsempfaenger=0` means the Terje sink never registered — the cause
   is in the comp mod or its load order, not in the attribution. `abonnenten=0`
   means nothing at all hangs off ChefZ.
2. Confirm `cookActorRadius` is not `0` and was not clamped:
   ```
   cookActorRadius = <x> ist unbrauchbar - geklammert auf <y>.
   ```
3. Watch the actor in the cooking context with `COOK` at `DEBUG` — the context
   dump appends ` koch=<identityId>` only when the identity is non-zero. No
   `koch=` means identity `0`.
4. XP can also be zero for reasons that are not attribution: the XP matrix is
   config (`CfgChefZTerjeSkills`, overridable through `GameOverrides.xml`),
   there is a repeat damper (`repeatFreeCount 5`, `repeatWindowSec 900`, floor
   25 %), and a batch cap. A player who cooked the same dish six times in
   fifteen minutes gets less on purpose.

**Fix** — Raise `cookActorRadius`, or accept the multi-player rule. Details of
the XP matrix, the perk and the damper are on
[Terje Compatibility](Terje-Compatibility).

---

## 9. `health=SAFE_MODE` — ChefZ is inert

**Symptom** — Nothing ChefZ works at all. Vanilla cooking is completely normal.

**Cause** — Either `strictMode: true` with at least one load error, or more
errors than `safeModeErrorThreshold` (default 25). All registries are emptied.
This is the intended emergency exit: *rather all vanilla than half ChefZ*.

**Check** — Search for:

```
SAFE MODE.
```

The line names the reason:

```
SAFE MODE. strictMode ist eingeschaltet und es gab <n> Fehler.
SAFE MODE. <n> Fehler ueberschreiten die Schwelle <threshold>.
```

followed by

```
ChefZ ist im SAFE MODE inert. Vanilla-Kochen ist davon unberuehrt und laeuft vollstaendig.
```

The individual errors are **above** this line in the load report.

**Fix** — Fix the individual errors. Do not raise the threshold to hide them.
Run the full validator first — it finds most load errors statically:

```
node tools/chefz-validate/index.mjs
```

---

## 10. Settings changes have no effect

**Symptom** — You edited `$profile:ChefZ\Core.json` and nothing changed.

**Causes, in order of likelihood**

1. **You did not restart.** There is no runtime reload in V1. A second
   `LoadAll()` logs `LoadAll() wurde erneut aufgerufen und ignoriert.` and does
   nothing.
2. **The overlay is switched off.** Look for:
   ```
   Overlay ist per Einstellung abgeschaltet (allowProfileOverlay=false) - $profile:ChefZ wird nicht gelesen.
   ```
3. **The profile directory is not writable.** Look for:
   ```
   Unter $profile:ChefZ kann nicht geschrieben werden - das Overlay entfaellt.
   ```
   Usually a missing `-profiles=` on the server command line, or a read-only
   directory.
4. **Your `id` is not `CORE`.** The record with `id == "CORE"` is preferred; a
   different id is only used as a fallback when there is no `CORE` record.
5. **You are on a client.** Rank 3 is server-only. `$profile:` on a client is
   the player's own directory and is never read.
6. **The value was clamped.** Look for:
   ```
   <field> = <was> ist unbrauchbar - geklammert auf <now>.
   ```
7. **The value name was unknown.** For example:
   ```
   capabilityMode "asauthored" ist unbekannt - benutzt wird "asAuthored". Gueltig: asAuthored, neverBlock, ignore.
   defaultExtraItems "forbidden" ist unbekannt - benutzt wird "forbid". Gueltig: forbid, ignore, consume.
   Unbekannter Logkanal "COOKING" - ignoriert. Gueltig: …
   ```
   These are case-sensitive for `capabilityMode` and `defaultExtraItems`.
8. **You wrote a nested block partially and lost the rest.** `priorityWeights`
   and `qualityScoring` are replaced **whole** by an overlay; keys you omit fall
   back to the **code** defaults, not to the shipped `Core.json` values.

---

## 11. Quick reference: RPT search strings

| Search for | Means |
|---|---|
| `[ChefZ]` | everything |
| `[ChefZ][ERROR]` | every ChefZ error |
| `[ChefZ][CONFIG] slices=` | the load summary line |
| `health=SAFE_MODE` / `health=DEGRADED` | core state |
| `SAFE MODE.` | why safe mode was entered |
| `Core ist per Einstellung abgeschaltet (enabled=false)` | core deliberately off |
| `JSON nicht lesbar` | a data file was discarded |
| `Feld "kind" fehlt` / `Unbekannte Art` | header problem in a data file |
| `ist unbrauchbar - geklammert auf` | a setting was clamped |
| `Unbekannter Logkanal` | typo in `logChannels` |
| `Kein Slice meldet handcraftRecipeSlots an` | no handcraft slots declared |
| `ueberzaehligen werden NICHT angeboten` | too few handcraft slots declared |
| `Es sind keine Rezeptplaetze verankert` | modded-class chain did not reach ChefZ |
| `Eine Craftaktion wurde VERWEIGERT` | recipe ID drift caught on the wire |
| `[ChefZ][CORE] Handwerk ` | the anchor line to compare client vs server |
| `[ChefZ][CORE] Aussenkante ` | who is attached to ChefZ |
| `Unter $profile:ChefZ kann nicht geschrieben werden` | overlay unavailable |
| `Overlay ist per Einstellung abgeschaltet` | `allowProfileOverlay=false` |
| `LoadAll() wurde erneut aufgerufen` | second load attempt, ignored |
| `[ChefZ][V-A]` | the temporary PBO-JSON smoke test |
| `[ChefZ][NUTRI]` | nutrition audit findings |
| `Kein Treffer -> Vanilla-Kochen laeuft unveraendert weiter.` | no ChefZ recipe matched — usually correct |

---

## 12. Turning the log up

The single most useful setting when investigating anything:

```json
{
    "kind": "coreSettings",
    "schemaVersion": 1,
    "records": [
        {
            "id": "CORE",
            "logLevel": 5,
            "logChannels": ["MATCH", "COOK", "CONFIG"],
            "logToFile": true,
            "logReportToFile": true
        }
    ]
}
```

Restart. Then read `$profile:ChefZ\Logs\ChefZ_<date>.log` and
`$profile:ChefZ\Logs\load_report.txt` instead of scrolling the RPT.

Use the channel mask, not a global `DEBUG`. On a server with dozens of active
fireplaces a global debug level is unusable — the one interesting line drowns in
thousands. Channels and levels are listed in [Configuration](Configuration).

---

## Next

- [Configuration](Configuration) — every setting and what it does
- [Validation](Validation) — catch it before the server does
- [Known Limitations](Known-Limitations) — what is known broken or unmeasured
