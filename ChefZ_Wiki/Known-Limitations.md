# Known Limitations

This page is the honest inventory. Everything here is either not finished, not
verified, or verified only as far as static analysis reaches. It is kept current
deliberately: a wiki that only describes what works costs the reader more time than
it saves.

If you are deciding whether to run ChefZ on a live server, read this page first.

## The short version

ChefZ compiles and the mod boots. It does not yet keep a server running.

As of 28.08.2026 all five script modules compile with zero errors and zero
warnings, the server binds its port, and all twelve addons of the time register.
The config load then reads 551 records, 550 of them good. After that the process
still dies, and the core comes up inert.

**That measurement is older than the code.** `ChefZ_Cookbook` (Milestone 5.1), the
self-test trace, Beekeeping V2 and the two asset addons all landed on 29.08.2026,
after this run. The mod is fifteen sources now, not twelve addons, and none of the
changes since have been through a compiler or a server start. The static suite is green; that is a different claim.

What that gap costs became concrete on 31.08.2026. `ChefZ_WearsGasMask`, part of the
Beekeeping V2 code that landed on 29.08., called `IsGasMask()` on an `ItemBase`. The
function does not exist there — vanilla declares it on `Clothing_Base`
(`InventoryItem.c:995`) and overrides it in `MaskBase.c:6`. The call cannot compile,
so `ChefZ_Farming` could not have built, and the beehive's entire sting branch sat
behind it. It stood in the tree for two days and no validator saw it: the static
suite parses ChefZ's own rules, not the vanilla class hierarchy. It is fixed — the
cast is `Clothing` now, vanilla's own pattern in `PlayerBase.c:1479/1499` — but it
was found by reading, which is not a method that scales.

Two properties of the engine's JSON layer caused most of this. One is fixed, one
is not. Both are described below, because neither is visible from the code and
neither produces an error message.

## Four asset addons — packer rule fixed, verified 31.08.2026

Two deliveries landed on 29. and 30.08.2026, together **50 `.p3d` and 52 `.paa` files**
in four addons: `ChefZ_Devices` (hive and the stations), `ChefZ_Items` (tools and
containers), `ChefZ_Plants` (crops and herbs) and `ChefZ_Food` (prepared food).
**56 of the 129 spawnable classes** now stand on their own geometry instead of a
vanilla proxy; 45 of them were rebound in the second delivery alone. All four
addons are assets only — no class, no script, no record.

All four carry two-level prefixes of the form `ChefZ\<name>`, inherited from the
delivery layout they came from. That prefix is **not a mistake**: the model paths
written into `ChefZ_Farming` point at exactly those roots
(`ChefZ\ChefZ_Items\models\carrot.p3d`), so config and prefix agree. An earlier
`pack.mjs` rule rejected any prefix that was not the folder name and skipped all
four; the rule now accepts exactly the two forms that are correct
(`pack.mjs:85-91`): the folder name itself, or `ChefZ\` plus the folder name.

Verified 31.08.2026: `node tools/chefz-pack/pack.mjs <target>` packs **all 17
sources** (14 core addons + 3 comp mods), exit code 0, including a 2.9 MB
`ChefZ_Core.pbo` and the four asset addons with their two-level prefixes.

What remains true: `chefz-validate` does not read `$PREFIX$` files, so a future
prefix/path divergence would again be invisible to the static suite.

**Incident, same day:** the five PBOs `ChefZ_Core`, `ChefZ_Devices`, `ChefZ_Food`,
`ChefZ_Items`, `ChefZ_Plants` on the test server were hand-packed from the
delivery folder `ChefZ/` instead of `Psyerns_ChefZ_Core/Addons/` — recognizable
by their raw filesystem-path prefixes (`Users\Administrator\...`). That replaced
the real 2.9 MB core with the delivery's 17 KB stub, and the Game script module
failed with `Unknown type 'ChefZ_Sym'` for every core type. The delivery folder
must never be packed (see next section); the repair is one full `pack.mjs` run.

## The delivery folder is back in the tree

`ChefZ/` was removed on 29.08.2026 and restored the same evening ("exactly as
uploaded"). It is the asset delivery in its original shape, and it keeps growing: a
second batch on 30.08. brought the plant models. It now holds **129 files** — 50
models, 52 textures, 17 scripts — in five folders: `ChefZ_Core`, `ChefZ_Devices`,
`ChefZ_Food`, `ChefZ_Items`, `ChefZ_Plants`.

Nothing consumes it directly. Eight of its models were copied into `ChefZ_Devices` and
`ChefZ_Items` under `Addons/`, and those copies are what the content addons point at.
The rest — including every plant model from the 30.08. batch — is delivered but not
bound to any class yet. The folder is the record of what arrived, not a second source.

**It must not be packed as it stands.** Three of its five `CfgPatches` names —
`ChefZ_Core`, `ChefZ_Devices`, `ChefZ_Items` — are also the names of real addons under
`Addons/`. When it was first uploaded only `ChefZ_Core` collided; the asset integration
added the other two. `ChefZ_Food` and `ChefZ_Plants` are unique so far, but that holds
only until their contents are integrated the same way. Two addons of one name cannot
both load.

Three checks keep it harmless today, and all three are checkable:

| Check | Result |
|---|---|
| Packed by `pack.mjs`? | **No** — it matches neither `Psyerns_ChefZ_Core/Addons/*` nor `Psyerns_ChefZ_*_Comp` |
| Carries a `$PREFIX$`? | **No** |
| Read by `chefz-validate`? | **No** — `ADDONS_DIR` is `Psyerns_ChefZ_Core/Addons` and nothing else |

## Not yet done

### The server does not stay up

After the config load the process ends with an access violation inside the
mission's `OnInit` chain. The same server, started without `@ChefZ` in the mod
list, runs stably — measured over two minutes — so the deployment itself is
sound.

The most likely link: `ChefZ_HandcraftBridge` anchors its recipe slots in the
mission constructor and fills them only after loading. While the core is in safe
mode they stay empty.

**Outdated as of 31.08.2026:** the server stayed up for roughly 80 minutes with a
client connected that day (`script_2026-08-31_14-44-26.log`), long enough for items
to decay and for 1925 runtime null accesses to accumulate. Whether the access
violation above is gone or only unhit is not written down anywhere — this entry
needs the operator's verdict.

### The core comes up in safe mode

551 records read, 550 good, none rejected — and every registry empty. Two causes,
both listed under *Engine limits* below: the overlay clamps
`safeModeErrorThreshold` to 1, and a single failing self-test then trips it,
because self-test errors count towards the same counter that guards safe mode.

`ChefZ_Log.ResetCounters()` after `RunSelfTest()` would separate the two.

### Eight self-test groups fail

S1, S9, S10, S11, S13, S14, S16 and S17 report failures. Some of them are likely
downstream of the constructor limit below, since the tests assume the sentinel
machinery works.

### No signatures, no binarisation

`tools/chefz-pack/pack.mjs` packs all seventeen sources (verified 31.08.2026,
see above), unsigned and unbinarised, and `tools/chefz-pack/testrun.ps1` starts
the test server and reads its verdict. Neither signing nor binarising has been
done. See [Installation](Installation).

### A config error is invisible in the logs

Worth knowing before debugging anything: DayZ reports a config error in a **modal
window**, not in the RPT. On a server nobody clicks it away, so the process sits
there with an eight-line RPT, no error and no exit. `testrun.ps1` reads that
window first and the logs second.

### No gate has been run in game

All four milestone gates stand at **NOT READY**. Each gate report carries a
numbered in-game checklist — together roughly 150 steps with concrete ingredients,
quantities, durations and expected RPT lines — and not one step has been executed.
A fifth list joined them on 31.08.2026:
`Psyerns_ChefZ_Docs/GATE_WILDWUCHS_CHECKLISTE.md`, 22 steps for the wild plants and
the CE fragment, including a server-side step nobody has had to do before — installing
central-economy files into the mission.

Gate 4 in particular requires two server configurations: one **without** Terje and
one with. The run without Terje is the more important of the two, because it tests
the project's central promise.

**The mod itself has run in game since 31.08.2026** — three sessions that day,
client and server (`script_2026-08-31_14-44-26.log` and two repeats at 16:11 and
16:14, some 80 minutes in total). They were bug hunts, not gate runs: they produced
the 1925 null accesses in `Edible_Base.GetFoodStageType`
(`ChefZ_Edible_Base.c:469`), the seven per-boot warnings for ingredient templates
(`ChefZ_ConfigCppSource.c:431`) and the Tactical Bacon slot that could never be
filled. Not one numbered checklist step has been ticked off.

### No 3D assets

Every item uses a vanilla proxy model. Several unrelated items therefore share a
mesh and are distinguishable only by size and weight, which makes an in-game test
harder than it needs to be.

One decision blocks asset production and should be made before anyone models
anything: **no config declares `hiddenSelections`**, so none of the planned texture
variants can be applied to a shared mesh. The selection name has to be agreed first.
Applied consistently, the shared-mesh strategy cuts the V1 mesh count from 161 to
about 45.

## Engine limits

Both were found by running the server, not by reading documentation, and both are
silent — no error, no warning, in the first case not even a call stack.

### A self-referential class crashes the JSON deserializer — fixed

`ChefZ_Selector` used to contain `anyOf`, `allOf` and `not`, all of its own type.
The engine builds its type descriptor by walking members, and a class that
contains itself never lets that walk finish. Reading the first document whose
records carry a selector ended the process with an access violation.

Proved rather than guessed: only the record kinds whose graph reaches a selector
crashed; `ChefZ_Range`, nested but not self-referential, reads fine; renaming the
`not` member changed nothing; and a throwaway self-referential class hung on
`ChefZ_PreservationDef` made `Preservation.json` crash too. Across the ~35 mods on
the test server there is not one self-referential class.

The cycle is now a chain — `ChefZ_Selector` → `ChefZ_SelectorL1` → … →
`ChefZ_SelectorL8`, the last without children. **The JSON is unchanged**: same
keys, same nesting, same files. The deepest nesting in all existing content is
two, and `maxSelectorDepth = 8` remains the binding limit.

> **Rule:** no class read from JSON may contain itself — not directly, not through
> a container, not through an intermediate class.

### The deserializer does not run the constructor — open

`ChefZ_Record` and its subclasses preset their fields with a sentinel in the
constructor: `ChefZ_Undefined.INT` is `int.MIN`. The whole ranked-configuration
scheme rests on it — `PatchInt` takes an incoming value only when it is *not* the
sentinel.

That constructor does not run during deserialization. A TRACE line placed inside
`ChefZ_CoreSettingsDef()` never appeared, in a run that wrote 162 other TRACE
lines and whose log shows the overlay being read two lines earlier.

So every **absent** field comes back as `0` / `""` / `false` / `null`. The rank-3
`$profile` overlay therefore overwrites every setting the operator did *not*
write — the opposite of what an overlay is for. The core's own overlay template
contains nothing but `{"id": "CORE"}`, and that is what clamps
`safeModeErrorThreshold` to 1 and puts the core into safe mode.

The bool probe is affected for the same reason: `ChefZ_RecordProbe.Bool()` is read
in the constructor, so "`allowPartial` not written" can no longer be told apart
from "`allowPartial: false`".

**Fix direction:** presence must come from the JSON text, not from the value.
Collect each record's written keys while reading and put them into
`explicitFields[]`. `PatchInt`, `PatchFloat`, `PatchText` and `PatchBool` already
consult `HasExplicit(field)`, so they need no change, and the sentinel becomes
unnecessary.

## Verified only as far as static analysis reaches

### Vanilla class collisions are checked only against a slice

`tools/chefz-validate/refindex/vanilla-classes.txt` is no longer empty: it holds
**184** vanilla item class names, taken from `ChefZ_Vanilla_Assets.md`, which was
itself built from a `types.xml` of 1,924 entries. Together with the ~16,000 names from
Terje, Expansion, COT, CF, Dabs and the vanilla *script* sources, that is what `naming`
and `classrefs` compare against — and it is why both report zero warnings today
(measured 31.08.2026).

What remains true: the vanilla *item* classes live in the game configs, which are not
in this repository, and 184 names are the ChefZ-relevant excerpt rather than the set.

Consequences, both still real:

- A ChefZ name colliding with a vanilla item **outside that excerpt** is not reported.
- Findings in the older gate reports are marked *plausible* rather than *confirmed*
  because they were written while the index was empty; they have not been revisited.

A class dump from a running server is still the better source. See
[Validation](Validation).

### The apiary rework is untested in-game

The 2026-08-29 honey chain (frames filling one after another inside the hive,
the extractor restarting itself per jar) rests on engine behaviour the static
checkers cannot confirm. Open until the in-game gate: whether the cargo grid
draws a frame's `quantityBar` (the vanilla UI code only proves it for the
vicinity view); whether `TurnItemIntoItemLambda` swaps a frame in its own cargo
cell (watch the log for `lambda cannot be executed, skipping!` and `Step D)
ABORT`; on failure the frame stays at 100 % as the first frame and every next
tick retries — nothing is lost, and the hive bypasses its own `CanReleaseCargo`
during the swap); whether the extractor's `CallLater` continuation and the
"cargo full → `RUN_FAILED` without consumption, chain stops until cranked
again" path behave as read from the source; and
whether a handcraft transform with two inputs of the same class
(`TR_ExtendBeehive`, two Beehive Kits) binds. The chain's assumptions A1–A5 are
listed on [Processing Stations](Processing-Stations#beehive-and-double-beehive).

### Wild plants (Wildwuchs) — 31.08.2026

The four wild plants, the harvest action, the companion roll and the CE fragment
(three events) were all written on 31.08.2026 and **not one of them has been seen
running**. The static suite passes on them, which says only that the records parse
and the names resolve. Four questions were left open on purpose, because guessing
them would have been cheaper than measuring them and worth less:

| | Question | What rides on it |
|---|---|---|
| **M1** | Does an **MLOD** model load at runtime, and does `corn_plant.p3d` then show *one* growth stage? | Every `.p3d` in the mod begins with the magic `MLOD` — unbinarised. The corn plant additionally carries fourteen `hide` selections on `PlantBaseSkeleton`, which in vanilla only `PlantBase.UpdatePlant()` ever switches. A wild plant is not a `PlantBase`. The countermeasures are a `class AnimationSources` with `initPhase` and a script pass from `EEInit`; **that a non-`PlantBase` may switch those selections is plausible from the sources and proven by no running example.** Failure looks like an invisible plant or six stages inside each other — a model/config question, not a script one |
| **M2** | Do script-spawned companions obey the `types.xml` lifetime (300 s) or the event lifetime (180 s)? | The whole corn population. A companion belongs to no event, so on paper only `types` applies — hence 300 rather than the 900 the other four carry. If that reading is wrong, the stock runs to roughly six times `nominal` instead of the intended band. The measurement is 15 minutes standing still, then 10 minutes away and back |
| **M3** | Does the world hold **160–220** corn plants and **~140** herbs after 30 minutes? | The equilibrium formula behind `nominal 60` is **calculated, not measured**. If the count comes out elsewhere, `nominal` in `ChefZTrajectoryCorn` is what gets adjusted — never the lifetime, which carries the companion logic |
| **M4** | Is `CALL_CATEGORY_SYSTEM` the right queue for the deferred companion spawn? | Vanilla's only comparable case (`Wreck_SantasSleigh`) uses `CALL_CATEGORY_GAMEPLAY`. This module runs all its server work in `SYSTEM` and a CE spawn is server work — a named deviation, not an oversight, but if the queue does not run, corn never grows in groups and there is no log line saying so |

Two more things nobody has watched yet: whether the three herb **proxy** meshes
(`plant_material.p3d`, `rosmary.p3d`, `parsley.p3d`) are hit by the action raycast at
all — the reason vanilla's prettier clutter models were *not* used is that they may
carry no geometry LOD — and whether a harvest that produces nothing correctly leaves
the plant standing.

The step-by-step measurement is `Psyerns_ChefZ_Docs/GATE_WILDWUCHS_CHECKLISTE.md`
in the repository — 22 steps, each with an expected value and a named failure mode.

### The test deployment holds two copies of the mod

`D:\Agent\deployments\DME-Test` contains **both** `@ChefZ\Addons` (17 PBOs, the
output of `pack.mjs`, unsigned) and `@3786176249\Addons` (the same PBOs, signed with
`DeadmansEchoCore.bikey`, `meta.cpp` name `DeadmansEcho-TOW-Test`). Both are listed
in the mod line of `tools/chefz-pack/start-DME-Test.cmd` and
`tools/chefz-pack/testrun.ps1`: `…;@3786176249;@ChefZ;`.

Two folders whose PBOs carry the same `CfgPatches` names are not a defined state, and
nothing in the repository says which one wins. Nobody has been bitten by it yet
because nothing has run — but every finding of the next test run is unattributable
until one of the two is removed from the list. Deciding it is step 1 of the wild-plant
checklist; whichever way it goes, both files have to be changed, or the next
`testrun.ps1` loads both again.

### What the checkers cannot see at all

Runtime behaviour, cooking logic under load, whether the balance feels right,
whether models and textures are correct, and whether the mod loads. That is what
the gate checklists are for.

Four defects found during development illustrate the point. Each passed every
static check, and each would have looked like something else in game:

| Defect | What it looked like |
|---|---|
| The core never enabled cookability | Recipes "don't work" |
| 111 food classes had no eat action | Items "are broken" |
| Cooking attributed no player | XP "isn't implemented" |
| Two stations derived from cookware | Vanilla cooking "behaves oddly" |

## Stations

All three defects in this section were found by reading, not by a gate review, and
none of them is reported by any checker. Two are fixed; one is open.

### Two stations had no cargo — fixed 2026-08-31

`ChefZ_GrainMill` and `ChefZ_MeatGrinder` declared no `class Cargo` block, so the
fact collector returned immediately and nothing could be put into them. The grain
chain stopped at its first step and the sausage chain had no station at all.

Both have one now: the mill 5×4, the grinder 5×3
(`ChefZ_Processing/config.cpp`). Every station in the mod now has cargo. The five
cells `TR_CornToFlour` needs for its `maxCount` of 5 are covered by the mill's
twenty.

### The smoker could never run — fixed 2026-08-31

`PROCESS_SMOKE` sets `requiresHeat` while the station base answers `false`, and the
station record set `needsFuel` against a class with no fuel slot, so the powered
check failed before the heat check was reached.

`ChefZ_Smoker` now overrides both `ChefZ_HasHeat()` and `ChefZ_IsPowered()`
(`ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c:202,216`) and
carries its own burn state, fed with bark from its own cargo — five minutes of full
burn for two pieces. Smoked Fish and Smoked Sausage work.

### `TR_SaltedMeatToSmoked` was rejected at boot — fixed 2026-08-31

Found 2026-08-31 while compiling [Recipe-Book](Recipe-Book).

The transform in `ChefZ_Preservation/Config/Processing/Smoking.json` declared **no
`process` field**, where its two neighbours in the same file both name
`PROCESS_SMOKE` and pin `stationsAllowed: ["ChefZ_Smoker"]`.
`ChefZ_TransformDef.ResolveDefaults` defaults `process` to the empty string
without complaint (`ChefZ_TransformDef.c:287`), and
`ChefZ_ProcessCompiler.c:355` then rejects the transform outright:

> *"nennt den Prozess "", den es nicht gibt — Transform abgewiesen (11 §7). Kein
> Prozess heisst: keine Station koennte ihn anbieten und keine Aktion ihn
> ausloesen."*

**What it cost:** `ChefZ_SmokedMeat` had no source — the class, its model, its
nutrition record and its stringtable entries were all shipped and unreachable, and
Salted Meat could only be dried.

**Why no checker saw it:** `chefzproc` does not test that a transform names a
process at all. The compiler's own error only appears in the RPT at server boot,
and the mod has not had a clean boot since 2026-08-28.

**The fix**, two lines in `Smoking.json` copied from `TR_RawSausageToSmoked`:
`"process": "PROCESS_SMOKE"` and `"stationsAllowed": ["ChefZ_Smoker"]`. All three
transforms in the file now carry the same shape, so Salted Meat reaches the smoker
in five minutes like its neighbours. Still unverified in a running game: the three
live tests of 31.08.2026 all predate this fix. The checker gap that hid it is
written up in [Validation](Validation).

## Built but inert

Three things exist in the code and do nothing today. None of them fails; they simply
never fire, which is why no checker reports them.

### The Terje Medicine module is dormant

It registers medicinal effects for `ChefZ_ThymeTea`, `ChefZ_WildGarlicTea` and
`ChefZ_HerbalTea`. **None of those three items exists in the main mod.** They appear
in no `config.cpp` and no ingredient record.

The teas fell through a gap in the plan: the Terje analysis listed them under the
compatibility module's responsibilities, but the *items* belong in the main mod, and
no milestone 3 slice had them in its brief. The module's own startup check reports
it — `3 of 3 entries without an item in the main mod` — so the diagnosis is already
in the log, but loading the PBO today buys nothing.

### Recipe locks are never asked

The capability gate is built and wired, with providers for survival level, the
herbalist perk and its yield bonus. **No recipe in the entire data set declares
`requires`**, so the gate is never consulted. This is blocked on an open design
decision about how hard the locks should be — refuse the recipe, degrade its
quality, slow it down, or reduce the yield.

### Two harvestables are invisible to the herbalist perk

The perk keys on the tag `CHEFZ_HERB`, and exactly four classes carry it. Two things
a player picks up as a seasoning do not: `ChefZ_PepperBerries` carries `CHEFZ_SPICE`
instead, and fresh paprika is vanilla `GreenBellPepper` (category `VEGETABLE`, tag
`CHEFZ_FRESH`) since 2026-08-29. Neither is highlighted in the world.

Since the harvest XP and the yield bonus were removed together with the herb plants,
highlighting is the only effect the perk still has — so this is the whole of it.

A comment in `ChefZ_TerjeHerbItem.c:25` still speaks of "die sieben frischen
ChefZ-Kraeuter". There are five. The behaviour may well be intended; the comment is
not.

## Balance consequences worth knowing

These are decisions the build made that the planning documents did not anticipate.
They are not defects, but they change how the mod plays.

**Cooking XP is scored by ingredient count, not by dish tier.** The plan spoke of
simple, complex and premium meals as if that were a property of the dish; nothing in
the data marks a dish as premium, so the build counts consumed ingredient entries
instead: two or fewer scores 3, three to five scores 8, more than five scores 15. A
cheap six-ingredient dish therefore scores premium while an expensive two-ingredient
one scores simple.

**Hunter Seasoning pays 2 XP.** It consumes five distinct ingredients at the mortar
and is scored as a generic spice grind. The plan asked for 5. Two other chain-
correcting overrides exist for exactly this kind of case, so this reads as an
oversight rather than a decision.

**Chain XP is per step, not per result.** Skipping a step pays only the steps you
actually performed — smoking meat without curing it first pays 3 rather than 5.

**The corn plot is a fourfold multiplier with no external input.** `CropsCount` went
from 2 to 4 on 31.08.2026 because wild corn had made the plot pointless: a plant used
to return one cob net after the seed went back in. The cob is its own seed
(`class Horticulture` on `ChefZ_Corn`), so one cob in now means four out — a loop, and
§22 of the production map forbids loops. It stands because vanilla runs exactly the
same loop for potato and tomato, and because at 2 it was already a loop, only a
quieter one. **Subject to gate review**; it is one number to turn back.

**Drying herbs is not needed for quality.** Fresh herbs reach up to 4 grade points
with no processing at all, because 19 recipes pay an extra point for freshness at or
above 0.8 and only an unprocessed ingredient reaches that. Since 31.08.2026 the CE
hands out fresh herbs by the plant, which makes the drying rack a shelf-life and XP
device rather than a quality one. Documented and deliberately not changed — it is a
recipe-design decision, not a build one.

## Open decisions

### Flour, yeast and spice powders are food

They carry nutrition records in the merged registry, so they are edible. Whether
they *should* be is a content decision that belongs in the registry, not in a
module — a module cannot un-declare nutrition it inherits. Salt shows what the
alternative looks like: no nutrition, no food node, no findings.

### The modded-class list has grown

The architecture note promised a closed list of modded vanilla classes and stated
that growth would mean the design is wrong. It has gone from 2 entries to 7 across
the four milestones:

```
ActionWorldCraft   Cooking   MissionGameplay   MissionServer
PluginRecipesManagerBase   WorldCraftActionData   WorldCraftActionReciveData
```

Every addition was individually justified. The list as a whole has not been
re-examined since, and it should be before the next one is added.

### Cook attribution after a restart

A dish is attributed to whoever stood alone at the device when its contents grew.
The first measurement of a session has no prior state and reads whatever is already
in the pot as growth. After a server restart, someone standing alone at a burning,
already-filled fireplace can therefore inherit the claim.

The alternative — ignoring the first measurement — was rejected because it breaks
the normal case: putting a full pot on a fire produces no later growth. See
[Terje Compatibility](Terje-Compatibility).

### Chernarus in non-Latin scripts

The place name is written in the target script for Russian, Chinese and Japanese
(`по-чернорусски`, `切爾諾盧斯`, `チェルナルス`) rather than kept in Latin. If the
project prefers the Latin form everywhere, it is a small, contained change.

## Housekeeping

### The registry merge is not reproducible from the repository

The delta merge has been performed by an ad-hoc script written fresh each time and
kept outside the project. It has been shown to reproduce the shipped state exactly,
but the script itself is not in the repository, so nobody else can run the merge.
If the merge is to stay machine-driven, it belongs in `tools/`.

### Redundant data in the deltas

Tag records carry an `appliesTo` list that the registry does not emit and no core
code reads. Tag membership actually comes from the `tags` field on the ingredient
records, which covers every entry those lists name. The data is redundant rather
than broken, but it reads as authoritative and is not.

### Standing warnings

Measured 31.08.2026, `node tools/chefz-validate/index.mjs`: **zero errors, two
warnings, exit code 0.** Both are `configcpp` naming the two `modded class` entries in
`ChefZ_TerjeSkillsEntry.c:102/138` (`MissionServer`, `MissionGameplay`) — they stay
listed on purpose, and the decision behind them belongs to the operator (see above).

The count used to be 90. What removed the other 88 was not suppression: 20 `modded
class` sites were reviewed and marked `SCOUT-GEPRUEFT`, the nutrition and `units[]`
rules were narrowed after 39 of 59 warnings turned out to be the checker's own noise,
and `refindex/vanilla-classes.txt` is no longer empty — it now carries 184 vanilla item
class names harvested from `ChefZ_Vanilla_Assets.md`. **That list is a slice, not a
dump:** it covers the classes the asset audit names, so a collision with a vanilla item
outside that slice would still go unreported. A class dump from a running server
remains the better source.

## What to do first

1. Compile. Nothing below this matters until the code builds.
2. Decide which of the two mod folders the test server loads — `@ChefZ` or
   `@3786176249`. Until then no test result can be attributed to a build.
3. Walk Gate 1's checklist on a server without Terje.
4. Decide the `hiddenSelections` name before commissioning any mesh.
5. Replace the 184-name vanilla excerpt with a class dump from a running server;
   that upgrades a whole class of older findings from *plausible* to decided.
