# Known Limitations

This page is the honest inventory. Everything here is either not finished, not
verified, or verified only as far as static analysis reaches. It is kept current
deliberately: a wiki that only describes what works costs the reader more time than
it saves.

If you are deciding whether to run ChefZ on a live server, read this page first.

---

## The short version

ChefZ compiles and the mod boots. It does not yet keep a server running.

As of 28.08.2026 all five script modules compile with zero errors and zero
warnings, the server binds its port, and all twelve addons of the time register.
The config load then reads 551 records, 550 of them good. After that the process
still dies, and the core comes up inert.

**That measurement is older than the code.** `ChefZ_Cookbook` (Milestone 5.1), the
self-test trace and Beekeeping V2 all landed on 29.08.2026, after this run. There
are thirteen PBOs now, not twelve, and none of the changes since have been through
a compiler or a server start. The static suite is green; that is a different claim.

Two properties of the engine's JSON layer caused most of this. One is fixed, one
is not. Both are described below, because neither is visible from the code and
neither produces an error message.

---

## Not yet done

### The server does not stay up

After the config load the process ends with an access violation inside the
mission's `OnInit` chain. The same server, started without `@ChefZ` in the mod
list, runs stably — measured over two minutes — so the deployment itself is
sound.

The most likely link: `ChefZ_HandcraftBridge` anchors its recipe slots in the
mission constructor and fills them only after loading. While the core is in safe
mode they stay empty.

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

`tools/chefz-pack/pack.mjs` packs all thirteen PBOs, unsigned and unbinarised, and
`tools/chefz-pack/testrun.ps1` starts the test server and reads its verdict.
Neither signing nor binarising has been done. See [Installation](Installation).

### A config error is invisible in the logs

Worth knowing before debugging anything: DayZ reports a config error in a **modal
window**, not in the RPT. On a server nobody clicks it away, so the process sits
there with an eight-line RPT, no error and no exit. `testrun.ps1` reads that
window first and the logs second.

### No in-game test

All four milestone gates stand at **NOT READY**. Each gate report carries a
numbered in-game checklist — together roughly 150 steps with concrete ingredients,
quantities, durations and expected RPT lines — and not one step has been executed.

Gate 4 in particular requires two server configurations: one **without** Terje and
one with. The run without Terje is the more important of the two, because it tests
the project's central promise.

### No 3D assets

Every item uses a vanilla proxy model. Several unrelated items therefore share a
mesh and are distinguishable only by size and weight, which makes an in-game test
harder than it needs to be.

One decision blocks asset production and should be made before anyone models
anything: **no config declares `hiddenSelections`**, so none of the planned texture
variants can be applied to a shared mesh. The selection name has to be agreed first.
Applied consistently, the shared-mesh strategy cuts the V1 mesh count from 161 to
about 45.

---

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

---

## Verified only as far as static analysis reaches

### Vanilla class collisions are unchecked

`tools/chefz-validate/refindex/vanilla-classes.txt` is empty. The reference index
holds roughly 16,000 class names from Terje, Expansion, COT, CF, Dabs and the
vanilla *script* sources — but the vanilla *item* classes live in the game configs,
which are not in this repository.

Consequences, both real:

- A ChefZ class name that collides with a vanilla item name would not be reported.
- Several findings in the gate reports are marked *plausible* rather than
  *confirmed* purely because this index is missing.

Filling it is cheap — a class dump from a server is enough — and it closes the
single largest gap in the checking net. See [Validation](Validation).

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

---

## Stations that cannot work

Both of these were found while writing this wiki, not by a gate review, and neither
is reported by any checker. Both would look in game like "the station is broken".

### Two stations have no cargo

`ChefZ_GrainMill` and `ChefZ_MeatGrinder` declare no `class Cargo` block in
`ChefZ_Processing/config.cpp`. The processing station base reads its ingredients
through the fact collector, which returns immediately when the inventory has no
cargo — so these two cannot receive input at all. (The cutting board, which had the
same problem, was removed on 2026-08-29.)

The six other stations all have one.

What this costs: the grain chain stops at its first step, so no flour and therefore
no dough, bread, pasta or dumplings. The sausage chain stops at the cutting board,
so no casing, therefore no raw sausage and none of the six cooked sausage varieties,
and no dry or smoked sausage either.

### The smoker can never run

Two independent reasons, either of which alone would be enough:

`PROCESS_SMOKE` sets `requiresHeat`, but `ChefZ_Smoker.c` is an empty class that
never overrides the heat check, which the base answers with `false`.
`ChefZ_SaltPan` does override it, with a fireplace proximity test — the pattern
exists, it was just not applied here.

Independently, the station record sets `needsFuel` while the class has no fuel
attachment slot, so the powered check fails before the heat check is even reached.

All three smoking transforms are unreachable.

---

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

### Two herbs earn nothing from the perk

`ChefZ_PepperBerries` carries the spice tag, so it earns harvest XP, but it is not
highlighted and gets no yield bonus. `ChefZ_Paprika` carries neither harvest tag and
gets nothing at all. Code comments in the skills module refer to "the seven ChefZ
herbs"; only five actually carry the herb tag. The behaviour may well be intended —
the comments are not.

---

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

---

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

---

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

The validation passes with zero errors and 90 warnings. They are not noise to be
suppressed, and none is a defect:

| Checker | Warnings | What they are |
|---|---:|---|
| `naming` | 56 | Vanilla name collisions unverifiable — the empty reference index |
| `configcpp` | 24 | Every `modded class`, listed on purpose so it stays visible |
| `chefznut` | 8 | Seeds, containers and salt without a nutrition block, correctly so |
| `classrefs` | 1 | Foreign classes unverifiable — same missing index |
| `deltas` | 1 | Two slices define one category with different parents; the concrete one wins |

---

## What to do first

1. Compile. Nothing below this matters until the code builds.
2. Fill `vanilla-classes.txt`. Cheap, and it upgrades a whole class of findings
   from *plausible* to decided.
3. Walk Gate 1's checklist on a server without Terje.
4. Decide the `hiddenSelections` name before commissioning any mesh.
