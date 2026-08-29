# Terje Compatibility

ChefZ ships two optional compatibility PBOs for [TerjeMods](https://github.com/TerjeSummer):

| PBO | Requires | What it does |
|---|---|---|
| `Psyerns_ChefZ_Terje_Skills_Comp` | `TerjeCore`, `TerjeSkills`, `ChefZ_Core`, `ChefZ_Farming` | Grants Survival XP for ChefZ cooking, processing and herb harvesting; adds one perk (`chefzherb`); exposes skill values as ChefZ capabilities |
| `Psyerns_ChefZ_Terje_Medicine_Comp` | `TerjeCore`, `TerjeMedicine`, `ChefZ_Core` | Attaches Terje immunity/regeneration effects to three herbal tea items |

Neither PBO defines a single item, recipe, process or nutrition record. They connect
two existing systems and nothing else. If you want ChefZ content, you want the main
mod ([Modules](Modules)); these two are wiring.

> **Read this first:** the Medicine module is currently **dormant**. The three tea
> items it describes do not exist in the main mod yet. See
> [The Medicine module is dormant](#the-medicine-module-is-dormant) below.

---

## The ground rule: ChefZ runs fully without Terje

ChefZ Core has **no Terje dependency in any direction that matters**. The dependency
graph only ever points one way:

```
        TerjeSkills            TerjeMedicine
             ^                       ^
             |                       |
  ChefZ_Terje_Skills_Comp   ChefZ_Terje_Medicine_Comp
             |                       |
             v                       v
                     ChefZ_Core
```

The comp modules know ChefZ and they know Terje. ChefZ knows neither.

### Where this is enforced

Three places, in increasing order of hardness:

1. **`requiredAddons[]` in the comp modules.**
   `Psyerns_ChefZ_Terje_Skills_Comp` lists `TerjeCore` and `TerjeSkills`;
   `Psyerns_ChefZ_Terje_Medicine_Comp` lists `TerjeCore` and `TerjeMedicine`.
   DayZ does not load an addon whose `requiredAddons` cannot be resolved. Without
   Terje installed, these PBOs are simply not there — their `modded class`
   declarations never compile, their config nodes never exist.

2. **The Core's own architecture.** ChefZ Core does not call out to anything. It
   *reports* completions through `ChefZ_ProgressRegistry.Report()` and *asks*
   through `ChefZ_CapabilityRegistry`. Both are empty registries by default. See
   [Architecture](Architecture) and [Delta-Protocol](Delta-Protocol).

3. **A validator that fails the build.** `tools/chefz-validate/chefzcore.mjs`
   implements project invariant **I4**. It scans every Core script file for
   foreign-system identifiers (`Terje*`, `Expansion*`, `CF_*`, `JM_*`, `COT_*`,
   `Dabs*`, plus everything in the reference index carrying a foreign prefix):

   * a match **in code** is an **error** — the build is red;
   * a match **in a comment** is a **warning**, downgraded to *info* only if the
     block is explicitly marked `I4-BELEG` (verified citation).

   The reason a comment is only a warning and not silently allowed: a comment
   saying *"call Terje here later"* looks exactly like a citation, and it is the
   first line of a dependency that should not exist.

   Current state of ChefZ Core: **zero code matches, three comment matches**, all
   three of which state what is deliberately *absent* —

   * `ChefZ_Cooking/config.cpp:3187` — a note that the five dish container slots
     stay generic for later systems, "Terje comp" among them
   * `ChefZ_Cooking/.../ChefZ_DishesBItems.c:34` — "no modded class, no override,
     no Terje reference"
   * `ChefZ_Meat/config.cpp:4-6` — "butchering itself belongs to Vanilla resp.
     Terje Hunting ... no Terje reference, in any form"

   See [Validation](Validation) for how to run the validators yourself.

### What a server operator gets from this

You can load ChefZ without Terje, Terje without ChefZ, or both. There is no build
of ChefZ that requires you to run TerjeMods, and adding or removing a comp PBO does
not touch any ChefZ save data — neither module writes persistent state.

---

## Module 1: `Psyerns_ChefZ_Terje_Skills_Comp`

### How XP reaches Terje

The module registers one `ChefZ_IProgressSink` (`ChefZ_TerjeProgressSink`) with the
Core's progress registry at `MissionServer.OnInit`. That is the entire hook surface
on the ChefZ side. Everything Terje-facing goes through one class,
`ChefZ_TerjeSkillsBridge`, which calls exactly five Terje methods:

```
TerjePlayerSkillsAccessor.GetSkillLevel(skillId)
TerjePlayerSkillsAccessor.AddSkillExperience(skillId, value, affectModifiers, showNotification)
TerjePlayerSkillsAccessor.GetPerkLevel(skillId, perkId)
TerjePlayerSkillsAccessor.GetPerkValue(skillId, perkId, out result)
TerjePlayerSkillsAccessor.IsPerkRegistered(skillId, perkId)
```

`affectModifiers` is passed as `true`, so your global
`SKILLS_EXPERIENCE_GAIN_MODIFIER` applies to ChefZ XP the same way it applies to
hunting and fishing XP. The module never touches `GetTerjeProfile()` directly — the
experience → level → active perk stage conversion stays inside Terje.

Everything goes into the **Survival** skill (`surv`). There is no ChefZ cooking
skill; V1 deliberately does not add one.

### The XP matrix

All values below are read from `CfgChefZTerjeSkills` in the module's `config.cpp`
via `GetTerjeGameConfig()`. That means **every single number here is overridable**
by the operator in `$profile:TerjeSettings\Core\GameOverrides.xml` without rebuilding
a PBO. There is no hard-coded XP value anywhere in the module's scripts.

#### Cooking — `ChefZ_ProgressKind.COOK`

Awarded once, after the dish exists and the ingredients are consumed. Classified by
the number of **actually consumed ingredient entries** reported in the completion
payload:

| Consumed entries | Class | Survival XP |
|---:|---|---:|
| ≤ 2 | simple meal | **3** |
| 3 – 5 | complex meal | **8** |
| > 5 | premium dish | **15** |

Per-recipe overrides live in `ChefZ_Xp > ChefZ_Cook > ChefZ_Recipes` and beat the
classification. **That table is currently empty** — the whole V1 recipe set is
carried by the classification above. See [Recipe-Reference](Recipe-Reference) for
which recipes consume how much.

#### Processing — `ChefZ_ProgressKind.PROCESS`

Keyed on the **process ID**, resolved from the reported transform via
`ChefZ_ProcessingManager.GetTransform(sym).processSym`. Anything not listed gets
`defaultXp = 1`.

| Process | XP | Note |
|---|---:|---|
| `PROCESS_CUT_MEAT` | 1 | |
| `PROCESS_CARVE_BOWL` | 1 | |
| `PROCESS_CARVE_PLATE` | 1 | |
| `PROCESS_GRIND_MEAT` | 2 | |
| `PROCESS_SEPARATE_CREAM` | 2 | |
| `PROCESS_GRIND_HERB` | 2 | |
| `PROCESS_GRIND_SPICE` | 2 | |
| `PROCESS_ROLL` | 2 | step 1 of pasta |
| `PROCESS_DRY_SALT` | 2 | step 2 of salt |
| `PROCESS_SALT_CURE` | 2 | step 1 of dried/cured meat |
| `PROCESS_CHURN_BUTTER` | 3 | |
| `PROCESS_PRESS_CHEESE` | 3 | |
| `PROCESS_BOIL_BRINE` | 3 | step 1 of salt |
| `PROCESS_MILL` | 3 | flour |
| `PROCESS_KNEAD` | 3 | dough |
| `PROCESS_DRY` | 3 | herbs, pepper; also step 2 of dried meat |
| `PROCESS_SMOKE` | 3 | step 2 of smoked sausage |
| `PROCESS_STUFF_SAUSAGE` | 5 | |

All 21 process IDs that exist in the ChefZ data set are listed. `defaultXp = 1`
covers any process a future content module adds.

Per-transform overrides in `ChefZ_Xp > ChefZ_Process > ChefZ_Transforms` beat the
process value. Two entries exist:

| Transform | XP | Why it overrides |
|---|---:|---|
| `TR_DoughToRawPasta` | 5 | pasta is a single step since 2026-08-29 (one dough, pasta machine); the transform carries the whole value |
| `TR_FishToSmoked` | 5 | fish smoking is single-step (no curing stage), so this one transform carries the full value |

**Chain totals.** The design target was stated per *result*, the Core reports per
*step*. The values are chosen so the sums line up:

| Chain | Steps | Total |
|---|---|---:|
| Salt from brine | `PROCESS_BOIL_BRINE` 3 + `PROCESS_DRY_SALT` 2 | 5 |
| Pasta | `TR_DoughToRawPasta` 5 | 5 |
| Dried meat | `PROCESS_SALT_CURE` 2 + `PROCESS_DRY` 3 | 5 |
| Smoked sausage | `PROCESS_STUFF_SAUSAGE` 5 + `PROCESS_SMOKE` 3 | 8 |
| Smoked fish | `TR_FishToSmoked` 5 | 5 |

**Consequence worth knowing:** a player who skips a step gets only that step's
share. Smoking meat *without* curing it first pays 3, not 5. This is intentional —
the XP follows the work actually done — but it does mean the "chain total" numbers
only hold for players who run the full chain. See [Production-Chains](Production-Chains).

#### Herb harvesting — removed

Since 2026-08-29 the herbs are **found**, like vanilla mushrooms: there are no herb
plants, no seeds and no `Harvest()` to hook. The harvest XP and the yield bonus that
used to live in `modded class ChefZ_HerbPlantBase` are gone with the plant; the
`harvestTags[]` / `ChefZ_Harvest` config nodes remain as inert configuration. What
survives of the herbalist is the highlight on herbs lying in the world
(`modded class ChefZ_FreshHerbBase`, see below).

#### What gives no XP at all

| Progress kind | Awarded? | Why not |
|---|---|---|
| `cook` | yes | |
| `process` | yes | |
| `preserve` | **no** | Smoking, drying and salting already run as transforms and already report `process`. The state change into a preserved state fires `preserve` *on top*. Taking both would pay dried meat and smoked sausage twice. On top of that, the `preserve` payload carries no `identityId` — there would be nobody to credit. |
| `consume` | **no** | See [System boundaries](#system-boundaries). |
| `discover` | **no** | A first-time recipe success already paid through `cook` in the same operation. |

---

### System boundaries

Three things ChefZ deliberately does **not** award XP for, because someone else
already does:

**Butchering animals belongs to Terje Hunting.** ChefZ does not mod any butchering
action. Terje awards `hunt` XP in its own `PrepareAnimal.c`. ChefZ_Meat's config
says so in its own header. Grinding, mincing and stuffing that meat afterwards is
*further processing* and pays **Survival** XP — never Hunting XP.

**Filleting fish belongs to Terje Fishing.** Same shape: Terje awards `fish` XP in
`PrepareFish.c`; ChefZ picks the fillet up afterwards and pays Survival for smoking
or salting it.

**Eating gives Metabolism XP automatically.** Terje hooks `PlayerStomach.AddToStomach`
and awards `mtblsm` XP for anything that enters the stomach — including every ChefZ
dish, with no ChefZ code involved. If ChefZ also awarded XP for `consume`, every
meal would pay twice: once from Terje's stomach hook and once from ChefZ. That is
why `consume` is refused in the sink, and why the tea items in the Medicine module
are contractually forbidden from carrying `<skillId>SkillExpAddToSelf` /
`...AddToTarget` parameters (TerjeCore would turn those into a *second* XP source
for the same act of drinking).

The general rule: **if Terje already covers an action, ChefZ stays out of it.** The
double-XP failure mode is invisible in play — nothing breaks, a skill bar just
climbs faster than it should — which is exactly why it is guarded structurally
rather than by testing.

---

### The Herbalist perk (`chefzherb`)

One perk. Exactly one. It is attached to Terje's existing **Survival** skill via
`class CfgTerjeSkills > class Survival > class Perks`, following the pattern
TerjeRadiation uses to hang perks into `Immunity`. No Terje file is edited; the
engine's config merge extends the existing node.

The perk ID starts with `chefz` so it cannot collide with a future Terje perk.

| Stage | Required Survival level | Perk points | `values[]` (yield bonus) | Highlight range |
|---:|---:|---:|---:|---:|
| I | 5 | 1 | 0.00 | 12 m |
| II | 10 | 1 | 0.10 | 12 m |
| III | 15 | 1 | 0.20 | 20 m |
| IV | 25 | 2 | 0.30 | 30 m |
| V | 35 | 2 | 0.50 | 45 m |

Stage I intentionally grants **no** yield bonus — it only unlocks the highlight.
Stage II adds yield at the same range; the range only starts growing at stage III.

#### What it actually does

**1. Highlighting.** Client-side only. Ripe ChefZ herb plants and loose fresh herb
items within range get Terje's mushroom highlight particle
(`ParticleList.TERJE_SKILLS_MUSHROOMS_HIGHLIGHT`). Technically this is a copy of
Terje's own `MushroomBase` pattern: `IsTerjeClientUpdateRequired()` returning a
constant `true`, `OnTerjeClientUpdate(float)` as the per-second tick, and
`GetHierarchyParent() == null` as the "lying free in the world" condition. No
registration is needed — items enrol themselves in Terje's
`PluginTerjeClientItemsCore` ticker.

By default only *harvestable* plants glow (`highlightUnripe = 0`); flipping that to
`1` also lights up growing ones, which defeats the point of the perk but is your
server.

**2. Yield bonus.** Removed on 2026-08-29 together with the herb plants — there is
no harvest any more, so there is nothing to multiply.

The bonus is clamped to `[0.0, 2.0]` in the bridge even though the values come from
the module's own config — because `GameOverrides.xml` can overwrite them, and a
slipped decimal should produce bad balance, not an infinite herb farm.

#### What it keys on

The perk asks **only** for the ChefZ food tag `CHEFZ_HERB`, resolved through
`ChefZ_IngredientManager` and the symbol table. It never checks an individual herb
class. A new herb added by any content module works with the perk the moment it
carries the tag — no code change in this module. See [Adding-Content](Adding-Content).

In the current data set that is five classes:
`ChefZ_Parsley`, `ChefZ_Dill`, `ChefZ_Thyme`, `ChefZ_Rosemary`, `ChefZ_WildGarlic`.
`ChefZ_PepperBerries` carries `CHEFZ_SPICE` instead, so it earns harvest XP but is
**not** highlighted and gets **no** yield bonus. `ChefZ_Paprika` carries neither.

#### Icon and localisation

The perk reuses Terje's existing `tp_mushpremonition` icon from
`TerjePerk.imageset` / `TerjePerkBlack.imageset`. The module ships no artwork, and a
config pointer to a missing texture would render as an empty slot in the perk tree.
Display name and description are shipped in the module's stringtable in the project's
standard column set — `original` plus 13 languages — under
`STR_CHEFZ_PERK_HERBALIST` and `STR_CHEFZ_PERK_HERBALIST_DESC`.

---

### Anti-exploit

Four measures. Two of them are not in this module at all, because they are enforced
in the Core where they cannot be bypassed.

**1. XP only after a successful completion — structural, in the Core.**
`ChefZ_ProgressRegistry.Report()` is called from exactly two places in the Core, and
both sit *behind* a completed operation: after the ingredients are consumed and the
result exists. There is no call on insert, no call on start, no call on match. A
comp module that uses only this interface **cannot** violate the rule, even
deliberately — the call it would need does not exist. This is also why
`ChefZ_ProgressKind` has five names and every one of them denotes something
finished: `cook`, `process`, `preserve`, `consume`, `discover`. No `started`, no
`inserted`, no `attempted`.

Practical consequence: **inserting and removing ingredients pays nothing.** Not a
reduced amount — nothing. There is no event to hook.

**2. No progress on a bare state change — also in the Core.**
`ChefZ_ItemStateComponent.RaiseStateChanged` only reports progress for *preserved*
states. An item cycling `RAW → RAW_CHOPPED → RAW` would otherwise be an XP loop.

**3. Batch damping.** `ChefZ_TerjeXpDamper.BatchBonus()`. Producing ten items at
once does **not** pay ten times. The bonus is double-capped:

* at `batchMaxUnits = 3` extra units, at `batchBonusPerUnit = 1` XP each, and
* at `batchCapPercent = 50` % of the base XP.

Ten sausages at base 5 pay **5 + 2 = 7**, not 50. A single onion at base 1 pays
**1 + 0 = 1**. The batch bonus is applied *before* the repeat damping — the bonus
rewards the work, the damping judges its repetition.

**4. Repeat damping.** `ChefZ_TerjeXpDamper.RepeatPercent()`. Doing the same step
over and over in quick succession pays progressively less:

| Setting | Default | Meaning |
|---|---:|---|
| `repeatFreeCount` | 5 | first 5 executions pay 100 %; `0` disables damping entirely |
| `repeatStepPercent` | 25 | each further execution drops the factor by 25 points |
| `repeatMinPercent` | 25 | floor — never below 25 % |
| `repeatWindowSec` | 900 | after 15 minutes without *that* action, the counter resets |

The counter key is the recipe or transform ID, so **different actions do not damp
each other**: alternating between baking bread and stuffing sausage is not punished.
Herb harvesting uses its own key space (`HARVEST:<cropsType>`) so a harvest never
damps a same-named processing step.

A damped-but-successful action never drops to zero XP as long as the damping factor
is above 0 %: the result is floored at 1.

**Also worth knowing about the damper:** it is deliberately **volatile**. It is not
saved, not synchronised, and does not survive a server restart. It is also cleaned
up on player disconnect and hard-capped at 128 tracked keys per player. Restarting
your server resets everyone's damping — which matters only for someone who was
mid-grind at the moment of the restart.

**No special zero any more.** `PROCESS_CUT_OUT_SEEDS = 0` used to close the only
circular chain in the data set (onion → seeds → plant → onion). Seeds and plants are
gone since 2026-08-29, and the entry with them.

---

### Capability provider — read this before you expect recipe locks

`ChefZ_TerjeCapabilityProvider` registers with `ChefZ_CapabilityRegistry` at
priority 100 and answers exactly one question: *"what is the value of capability X
for player Y?"*

| Capability name | Source | Reads |
|---|---|---|
| `CHEFZ_CAP_SURVIVAL` | `skill` | Survival skill level (0..50) |
| `CHEFZ_CAP_HERBALIST` | `perkLevel` | active `chefzherb` stage (0..5) |
| `CHEFZ_CAP_HERBALIST_YIELD` | `perkValue` | `values[]` of the active stage |

**This is not a recipe lock, and there are no recipe locks in ChefZ V1.** What
happens with the answer is decided by the recipe author through a `requires` field
in the recipe record — and that field does not appear anywhere in the ChefZ data
set. As long as nobody uses it, this provider is never asked. It is the socket, not
the appliance.

The reason it stops there is a genuinely open design question (recorded as **OF-08**,
"how hard are recipe locks?"). Until that is decided, nothing is blocked, nothing is
downgraded and no yield is cut based on skill. The Core's `ChefZ_CapabilityGate`
returns `false` — "does not block" — by default and stays that way. See
[Known-Limitations](Known-Limitations).

Operators who want the provider gone entirely: `CfgChefZTerjeSkills >
ChefZ_Capabilities > enabled = 0`.

---

### Configuration switches

Everything lives under `CfgChefZTerjeSkills` and is readable — and overridable — via
`GetTerjeGameConfig()`, i.e. `$profile:TerjeSettings\Core\GameOverrides.xml`.

| Path | Default | Effect |
|---|---:|---|
| `enabled` | 1 | master switch; `0` and the module does nothing at all |
| `ChefZ_Xp > enabled` | 1 | `0` keeps the perk but stops all XP |
| `ChefZ_Xp > skill` | `surv` | target skill |
| `ChefZ_Xp > showNotification` | 1 | `0` awards XP silently |
| `ChefZ_Herb > highlightEnabled` | 1 | perk highlighting on/off |
| `ChefZ_Herb > highlightUnripe` | 0 | `1` also lights up growing plants |
| `ChefZ_Herb > yieldEnabled` | 1 | perk yield bonus on/off |
| `ChefZ_Herb > tag` | `CHEFZ_HERB` | which food tag counts as "herb" |
| `ChefZ_Capabilities > enabled` | 1 | register the capability provider |
| `ChefZ_Capabilities > priority` | 100 | provider priority in the registry |

Integer settings are clamped when read, so a typo in `GameOverrides.xml` produces
bad balance rather than a broken round. The startup banner in the RPT prints the
resolved summary:

```
TerjeSkills-Anbindung v0.0.1 aktiv  aktiv=1 xp=1 skill=surv kraut=CHEFZ_HERB hervorheben=1 ausbeute=1 faehigkeiten=1
```

See [Configuration](Configuration) for ChefZ's own settings and
[Troubleshooting](Troubleshooting) if the banner does not appear.

---

## Module 2: `Psyerns_ChefZ_Terje_Medicine_Comp`

### The Medicine module is dormant

The module describes Terje medicine effects for three items:

```
ChefZ_ThymeTea
ChefZ_WildGarlicTea
ChefZ_HerbalTea
```

**None of these three classes exists in the main mod.** There is no `CfgVehicles`
entry and no `CfgTerjeCustomLiquids` entry for any of them. The tea items belong to
ChefZ, and this module deliberately does not create them — a compatibility module
that ships content is no longer a compatibility module.

The consequence is benign: no such item is ever consumed, the resolver never
matches, and nothing happens. But it is also invisible, so the module logs the state
at server start (`ChefZ_TerjeMedStartupCheck`, `MissionServer.OnInit`):

```
ChefZ_TerjeMedicineComp: 'chefz_thymetea' hat hinterlegte Medizinwerte, aber im
Hauptmod existiert weder CfgVehicles noch CfgTerjeCustomLiquids dazu. Die Wirkung
bleibt schlafend.
ChefZ_TerjeMedicineComp: 3 von 3 Eintraegen ohne Item im Hauptmod.
```

If you see that in your RPT, the module is working correctly and there is simply
nothing for it to attach to. When the main mod ships the tea classes, the effects
below start applying with no further change.

**Loading this PBO today buys you nothing and costs you one set lookup per bite or
sip taken by any player on your server.** That is the honest trade. There is no
reason to load it until the teas exist. Tracked in [Known-Limitations](Known-Limitations).

### Where the values live, and why not on the item

Terje reads consumable parameters as `GetTerjeGameConfig().ConfigGetFloat(classname
+ " medImmunityGainForce")`, where `classname` is `CfgVehicles <ItemType>`. The
idiomatic move would be to re-open `CfgVehicles/ChefZ_ThymeTea` in this module and
append the `med*` parameters — which is exactly what TerjeMedicine's own FixVanilla
does to `VitaminBottle`.

That route is closed in this project: `tools/chefz-validate/configcpp.mjs` reports
any class path defined twice with a body as an **error**, because the later
definition silently overrides the earlier one. A compatibility module must not
patch a ChefZ class through config.

So the module uses its own config root, `CfgChefZTerjeMedicine`. That is a different
path from `CfgVehicles`, so there is no collision and the main mod stays untouched.
The parameter *names* are deliberately identical to Terje's, so that (a) anyone who
knows Terje reads them without translation, and (b) `GameOverrides.xml` can
override them through the same mechanism.

### The effect table

| Item | `chefzServingSize` | `medImmunityGainForce` | `medImmunityGainTimeSec` | `medImmunityGainMaxTimer` | `medHealthgainTimeSec` | `medHealthgainMaxTimeSec` |
|---|---:|---:|---:|---:|---:|---:|
| `ChefZ_ThymeTea` | 1 | 0.20 | 180 | 360 | 20 | 45 |
| `ChefZ_WildGarlicTea` | 1 | 0.40 | 300 | 600 | 0 | 0 |
| `ChefZ_HerbalTea` | 1 | 0.30 | 240 | 480 | 15 | 40 |

`chefzServingSize` converts Terje's `amount` unit into "one full serving". For a
piece item that is 1; if a tea is ever implemented as a liquid, it becomes millilitres
per cup. Without it, a 250 ml mug would be 250× as strong as a piece item.

### How strong is that, really?

Terje's own vitamin preparation is the yardstick
(`VitaminBottle`: force 1, 120 s per unit, 1800 s cap). Terje's immunity modifier
computes `internalImmunity += 0.25 * force * dt * 0.001`, and internal immunity is
clamped to 1.0. Full effect over the full runtime:

| Consumable | Calculation | Internal immunity gained | Relative |
|---|---|---:|---:|
| `VitaminBottle` | 0.25 × 1.00 × 1800 × 0.001 | 0.450 | 100 % |
| `ChefZ_WildGarlicTea` | 0.25 × 0.40 × 600 × 0.001 | 0.060 | **13 %** |
| `ChefZ_HerbalTea` | 0.25 × 0.30 × 480 × 0.001 | 0.036 | **8 %** |
| `ChefZ_ThymeTea` | 0.25 × 0.20 × 360 × 0.001 | 0.018 | **4 %** |

The strongest tea sits at roughly one eighth of vanilla vitamins.

For health regeneration, Terje regenerates `0.5 HP/s × (1 - health01)` while the
timer runs. Terje's own injectors run 180 s (Reanimatal) and 45 s (Propital). The
teas cap at 40–45 s — **below the weakest injector** — and a single cup contributes
15–20 s. At half health that is roughly 3–5 HP per cup. Lightly regenerative and
nothing more.

### The rule that matters: teas do not replace medication

Terje only accepts a new immunity value if `medImmunityGainForce >= the currently
active force`. With 0.20–0.40 against a `VitaminBottle`'s 1.00, **a tea can never
displace a running medication** — it loses the comparison and does nothing.
Conversely, the medication overrides a running tea immediately.

That ordering is the entire reason the force values are kept small. A tea with force
1.0 would be allowed to overwrite an active vitamin course, which would make brewing
tea strictly better than looting medicine. It is preventive and supportive, not
curative.

The in-game tooltip says so explicitly. The module appends its own line —
`STR_CHEFZ_TERJEMED_HERBAL_NOTE`, *"Herbal infusion – supportive, not a substitute
for medication."* — to Terje's own, already-translated effect strings
(`STR_TERJEMED_EFFECT_IMMUNGAIN`, `STR_TERJEMED_EFFECT_HEALTHREGEN`). Note that
Terje only shows consumable effects to players who have the `surv/expert` perk; this
module does not change that condition.

### Pharmacologist

Terje multiplies effect durations by `(1.0 + perkValue("med", "pharmac"))`. The tea
effects apply **exactly the same modifier**, using the same code shape as Terje's
own and TerjeRadiation's `TerjeConsumableEffects`. So the teas interact with the
Pharmacologist perk with no extra code.

The `medImmunityGainMaxTimer` cap limits the gain: Pharmacologist saves you cups, it
does not raise the ceiling.

### Where it hooks in

One `modded class TerjeConsumableEffects`, overriding `Apply()` and `Describe()`.
`super` is called **first** in both — Terje's own effects and TerjeRadiation's run
unchanged; this module adds afterwards and takes nothing away.

The call chain is `Edible_Base.Consume()` → `ItemBase.ApplyTerjeConsumableEffects()`
(which carries Terje's own `IsDedicatedServer()` gate) → `TerjeConsumableEffects.Apply()`.
The effect therefore happens exactly once per consumption and only after vanilla's
`Consume()` succeeded. There is no second entry point in the module: no action hook,
no modifier, no recipe hook.

A prefix guard rejects anything in `CfgChefZTerjeMedicine` that does not start with
`chefz_` and logs a warning. A compatibility module must never redirect foreign
items.

### Food poisoning — deliberately not touched

`FOOD_POISON` (agent 16) on spoiled food and `SALMONELLA` (agent 4) on raw meat are
already set **in the main mod**, in `nutrition_properties[]` index 5 — see e.g.
`ChefZ_Meat/config.cpp` where raw is 4 and rotten is 16. Terje picks these vanilla
agents up by itself: `TerjePlayerModifierPoison` converts `FOOD_POISON`,
`SALMONELLA` and `CHOLERA` into its own poison value via `TransferVanillaAgents`,
damped by immunity and — for `FOOD_POISON` — by the existing `immunity/svdinner`
perk.

Adding anything here would be exactly the forbidden duplication: a second
food-poison path next to Safe Dinner. **This module does not touch agents.**

`CHOLERA` currently has no carrier in ChefZ — there is no ChefZ water or liquid
item. Open point, see [Known-Limitations](Known-Limitations).

### The double-XP guard

`ChefZ_TerjeMedStartupCheck` also scans each existing tea class for
`<skillId>SkillExpAddToSelf` / `...AddToTarget` parameters, iterating over the
skills actually registered in `GetTerjeSkillsRegistry()` rather than a hard-coded
list. If it finds one, it logs an **error**:

```
ChefZ_TerjeMedicineComp: '<id>' traegt <skill>SkillExpAddToSelf. Essen und Trinken
geben bereits Metabolism-XP ueber PlayerStomach - das ist eine zweite XP-Quelle fuer
dieselbe Handlung. Parameter entfernen.
```

This is the one rule whose violation you would never notice in play, so it is
checked at startup rather than left to testing.

---

## What Terje already has, and ChefZ therefore does not duplicate

| Terje feature | Where | Why ChefZ leaves it alone |
|---|---|---|
| **Safe Dinner** (`immunity/svdinner`) | `TerjeMedicine/immunity.hpp` | Already reduces food poison risk. A second ChefZ food-poison resistance perk would stack invisibly with it. |
| **Wild Meat Lover** (`wmlover`) | `TerjeSkills/metabolism.hpp` | Already covers eating game meat. ChefZ adds no second wild-meat perk. |
| **Metabolism XP for eating** | `PlayerStomach.AddToStomach` | Automatic for every ChefZ dish, with no ChefZ code. See [System boundaries](#system-boundaries). |
| **Hunting XP for butchering** | `PrepareAnimal.c` | ChefZ mods no butchering action at all. |
| **Fishing XP for filleting** | `PrepareFish.c` | Same. |
| **Pharmacologist duration scaling** | `TerjeConsumableEffects` | The tea module applies Terje's own formula rather than inventing one. |
| **Mushroom Premonition** highlight machinery | `MushroomBase.c`, `ParticleList` | The Herbalist reuses Terje's particle and its client-tick pattern instead of shipping its own. |

The one perk ChefZ does add — `chefzherb` — covers ground Terje has no perk for:
finding and harvesting cultivated herbs.

---

## Who owns a dish: attribution and its known edge case

Every XP award above depends on one number: `ChefZ_EventArgs.identityId`, the player
the completed operation is attributed to. For processing and harvesting this is
trivial — a player performed an action, the action knows who. For **cooking** it is
not, and the answer is a deliberate design decision with a documented edge case.

Read `ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_CookActor.c` if you want the
full reasoning; the summary follows.

### Why there is a problem at all

A pot on a fire has no owner. That is a property of the engine, not an oversight:

* Vanilla keeps no player identity on any cooking device.
* The only place the engine reports *"player X moved item Y to Z"* is
  `EEItemLocationChanged` / `EEInventoryOut` **on the moved item**. Using those would
  mean `modded class ItemBase` — and ChefZ maintains a closed list of vanilla
  classes it does **not** mod: `ItemBase`, `Edible_Base`, `Pot`, `FryingPan`,
  `Cauldron`, `FireplaceBase`. That list is a hard commitment: if it grows, the
  design was wrong.
* A custom sync or persistence field on the vanilla vessel is out for the same
  reason. ChefZ only stores data on ChefZ classes. A station is a ChefZ class; a
  cooking pot is not.

There is therefore no way to *learn* who inserted the ingredients. What remains is
to *observe* it.

### The rule

Vanilla's cooking loop never puts anything **into** a vessel. `Cooking.CookWithEquipment`
iterates existing cargo, raises temperature and cooking time, subtracts quantity and
can make an item vanish at quantity 0 — it never creates one. Therefore, without
exception:

> **A cooking device's cargo grows exactly when a player put something in.**

That tick — and no other — is when the adapter asks who is standing at the device
and writes the answer into the cook session. A dish maturing, a cooking method
flipping, water evaporating, a class swap by the state system: none of these grow
the cargo, and none of them open a claim. Standing nearby inherits nothing.

If a device is in a player's hierarchy (in hand, in a backpack, on a gas stove in
their inventory), the question does not need asking — the engine has already
answered it. A device in a *corpse's* inventory attributes to nobody; a corpse does
not cook.

Otherwise the search radius applies: `cookActorRadius`, default **6 m**, configurable
in `Core.json`, hard-capped at 64 m. `0` disables attribution entirely. Six metres is
close enough that a passer-by does not pass for the cook and far enough that you may
stand beside the fire and move around. The cap is a safety rail, not a setting — a
one-kilometre radius would not be a generous server, it would be attribution to a
coincidence.

### When more than one player is standing there

The dish belongs to **nobody**, with one exception: the current claimant keeps the
claim as long as they are themselves in range.

```
if (incumbent != NOBODY && incumbentPresent)  ->  incumbent
if (presentCount == 1)                        ->  that player
otherwise                                     ->  NOBODY
```

Two properties follow, and they are the point of the whole design:

1. **Nobody can take a claim** by walking up to it. If the cook is present, it stays
   their pot. If two strangers are present, neither gets anything.
2. **Nobody can deny a claim** by walking up to it. The cook is present, so they
   keep it.

The safe direction is always `NOBODY` (`identityId == 0`). At 0, no progress sink
awards anything — the sink refuses immediately — and `ChefZ_CapabilityGate` blocks
nothing. A cooking operation with no player nearby completes fully and produces its
dish; it just has no owner.

The rule itself is a pure function (`ChefZ_CookActor.Decide`) with eight self-test
cases, because a wrong attribution looks exactly like a correct one and would never
be caught by playing.

### The edge case, stated plainly

**The first measurement of a session has no prior state and treats whatever cargo it
finds as growth.**

A cook session is created fresh when a cooking device ticks for the first time after
a **server restart**, or after `sessionTtlSec` (default 300 s) has passed without a
cooking tick.

So: if you are the **only** player standing at a burning, already-loaded fire owned
by someone else at the exact moment its session is recreated, you can inherit the
claim.

The window is narrow. It requires the original cook to be completely absent, it
requires you to be the only person in a 6 m radius, and it costs the original cook
nothing they would otherwise have received — if they are not there, they were not
going to be credited anyway. But it exists, and it is written down here rather than
left out.

The alternative — adopt the first cargo reading but never attribute from it — was
rejected because it breaks the *normal* case: someone who puts a full pot on a
fireplace and lights it produces no further cargo growth and would never earn a
claim at all.

Practically, for a server operator: after a restart, the first dish finished at a
pre-existing, unattended fire may award its Survival XP to whoever happens to be
standing there. This is not a duplication (only one player is ever credited) and not
an item exploit (nothing extra spawns) — only a misattributed XP grant of at most 15
XP. If that matters on your server, set `cookActorRadius = 0` and switch attribution
off entirely; you lose all ChefZ cooking XP with it.

---

## Not yet confirmed in game

The following are code-verified but have **not** been confirmed on a live server, and
belong to [Known-Limitations](Known-Limitations) until they are:

* The Herbalist highlight actually rendering on ChefZ herb plants through Terje's
  client ticker.
* The perk appearing in the Survival tree with the reused
  `tp_mushpremonition` icon and correct stage gating.
* The yield bonus spawning the correct number of extra crops with fertilised plants.
* The three tea effects — **untestable at present**, the items do not exist.
* Repeat-damping behaviour under real multi-player load.
* Whether `CfgTerjeSkills > Survival > Perks` merges cleanly with every combination
  of other perk-adding Terje-compatible mods on the same server.

---

## See also

* [Modules](Modules) — the full PBO list and load order
* [Installation](Installation) — what to put on the server
* [Configuration](Configuration) — ChefZ's own settings
* [COT-Compatibility](COT-Compatibility) — the admin-tool module
* [Architecture](Architecture) — event bus, registries, capability gate
* [Validation](Validation) — running the validators, including the I4 check
* [Known-Limitations](Known-Limitations) — open points and unconfirmed behaviour
