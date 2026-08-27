# Architecture

This page describes how ChefZ is built and why. It is written from the code, not
from the design documents — where the two disagree, the code wins.

Nothing on this page has ever been compiled or executed. See
[Known Limitations](Known-Limitations) before you rely on any of it.

---

## 1. The shape of the mod

ChefZ ships as one mod folder with nine addons inside it, plus three separate
compatibility mods:

```
Psyerns_ChefZ_Core/
  Addons/
    ChefZ_Core          the rule engine — 132 script files, no items
    ChefZ_Registry      the merged shared vocabulary — no scripts, no items
    ChefZ_Farming       ChefZ_Ingredients   ChefZ_Processing
    ChefZ_Meat          ChefZ_Baking        ChefZ_Preservation
    ChefZ_Cooking
  _deltas/              registry contributions of each content slice

Psyerns_ChefZ_COT_Comp
Psyerns_ChefZ_Terje_Skills_Comp
Psyerns_ChefZ_Terje_Medicine_Comp
```

Per-addon detail is on [Modules](Modules).

The division is not cosmetic. `ChefZ_Core/config.cpp` declares
`units[] = {}` and `weapons[] = {}` — the core contains no `CfgVehicles` entry
at all, not even an invisible one. Every item in ChefZ belongs to a content
addon.

---

## 2. The central idea: a rule engine without vocabulary

The core knows that *kinds* of records exist. It does not know any of their
*instances*.

`ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_RecordKind.c` holds the only fixed list
in the entire core — fifteen kind names:

```
coreSettings  category  tag  state  qualityTier  toolGroup  device
container  ingredient  nutrition  preservation  process  station
transform  recipe
```

The file says so itself: *"here stands no content vocabulary. `SAUSAGE`,
`SMOKED` or `PREMIUM` do not appear in this file and never may. What stands
here is the statement 'there are categories' — not 'there is the category X'."*

Everything a player can see — the category `SAUSAGE`, the state `SMOKED`, the
quality tier `PREMIUM`, the recipe `RCP_ChefZ_SausagePasta`, the station
`ChefZ_Mortar` — is a data record contributed by a content addon.

There is a matching Def class per kind under
`ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_*Def.c`, a typed registry
(`ChefZ_Registry<T>` in `3_Game/ChefZ/ChefZ_Registry.c`) and a manager that
builds indexes over it: `ChefZ_CategoryManager`, `ChefZ_StateManager`,
`ChefZ_QualityManager`, `ChefZ_IngredientManager`, `ChefZ_NutritionManager`,
`ChefZ_PreservationManager`, `ChefZ_ProcessingManager`, `ChefZ_PortionManager`,
`ChefZ_ContainerRegistry`, `ChefZ_ToolRegistry`, `ChefZ_RecipeEngine`.

### The price, and how it is paid

Records refer to each other by string, interned into a runtime integer
(`ChefZ_SymbolTable.Intern`, `1_Core/ChefZ/ChefZ_SymbolTable.c`). The file names
the cost plainly: *"no compile-time protection. A typo `SAUSGE` only shows up at
runtime."*

That is why `tools/chefz-validate` is treated as a requirement rather than an
extra. `chefzsym.mjs` resolves every symbol reference in every data file against
the merged registries and fails on an unknown one; `chefzcore.mjs` checks that no
content identifier and no foreign-mod name appears anywhere under
`Addons/ChefZ_Core/Scripts/**`. Both currently report zero findings. See
[Validation](Validation).

Runtime symbols are explicitly **not** stable across server starts. Persistence
uses the hash from `ChefZ_Identity`, network sync uses the ordinal from
`ChefZ_IdentityMap`.

---

## 3. Where configuration comes from: three ranks

`ChefZ_ConfigManager` (`3_Game/ChefZ/ChefZ_ConfigManager.c`) is the only place
in the mod that reads files or config trees. It pulls from three sources, in
this order, defined as `ChefZ_SourceRank`:

| Rank | Name in code | Source class | Read on | What it is |
|---|---|---|---|---|
| 1 | `CONFIG_CPP` | `ChefZ_ConfigCppSource` | client + server | named `CfgChefZ*` class trees in any addon's `config.cpp` |
| 2 | `ADDON_JSON` | `ChefZ_AddonJsonSource` | client + server | JSON files inside PBOs, each named in a `dataFiles[]` entry |
| 3 | `PROFILE_OVERLAY` | `ChefZ_ProfileOverlaySource` | **server only** | `$profile:ChefZ\Core.json` and `$profile:ChefZ\Overlay\*.json` |

### Rank 1 — `config.cpp`

`ChefZ_ConfigCppSource` reads exactly ten named root nodes and nothing else:

```
CfgChefZCategories   CfgChefZTags        CfgChefZStates
CfgChefZQualityTiers CfgChefZTools       CfgChefZDevices
CfgChefZContainers   CfgChefZIngredients
CfgChefZProcesses    CfgChefZStations
```

There is no `CfgVehicles` full scan. The file gives the reason: walking >10^4
classes costs unknown startup time for no benefit, because declaration is
explicit anyway.

Processes and stations live here — and not only in JSON — because
`ChefZ_ActionProcessAtStation.ActionCondition()` runs on the **client** and has
to know which processes a station offers, which tool group a process wants and
what the action is called. The client is guaranteed to read rank 1.

Transforms deliberately do **not** appear in rank 1: they carry selectors and
nested output descriptions, and they are a server-side decision.

### Rank 2 — addon JSON

An addon registers itself with the core in its own `config.cpp`:

```cpp
class CfgChefZ
{
    class ChefZ_DishesA
    {
        chefzApiVersion      = 1;
        loadOrder            = 330;
        handcraftRecipeSlots = 0;
        dataFiles[] = { "ChefZ_Cooking/Config/Recipes/Dishes_A.json" };
    };
};
```

`ChefZ_ManifestReader.ReadAll()` (`3_Game/ChefZ/ChefZ_RecordSource.c`)
enumerates `CfgChefZ` the same way vanilla's `ModLoader` enumerates `CfgMods`,
sorts ascending by `loadOrder` with an ordinal name tiebreak, and hands each
slice to a source instance. The name tiebreak is not cosmetic: without it, equal
`loadOrder` would make the merge depend on addon load order, and "first wins"
would no longer be reproducible.

**Files are never scanned, always declared.** The reason is technical: DayZ's
`FindFileFlags` knows only `DIRECTORIES` and `ARCHIVES(.pak)`; PBO contents
cannot be enumerated. A JSON file without a `dataFiles[]` entry is a dead file.
This has already happened once in this project — see
[Delta Protocol](Delta-Protocol).

Every runtime path is rooted at the addon's PBO prefix, which is the addon's
**folder name** (`$PREFIX$` contains `ChefZ_Cooking`, so paths begin
`ChefZ_Cooking/`). `ChefZ_PathTools.Resolve()` tries the forward-slash form
first and the backslash form second, and logs once which one carried.

Document form, identical for every data file (`3_Game/ChefZ/ChefZ_JsonDocs.c`):

```json
{ "kind": "recipe", "schemaVersion": 1, "records": [ ] }
```

One document carries exactly one kind. That is a hard requirement of
`JsonFileLoader<T>`, which needs a concrete target type; a mixed document would
have none.

### Rank 3 — the `$profile` overlay

Server only, and this is deliberate: on a client `$profile:` is
`%localappdata%\dayz`. An overlay there would be a file on the player's machine
deciding server rules.

`ChefZ_ProfileOverlaySource.EnsureLayout()` creates `$profile:ChefZ`,
`$profile:ChefZ\Overlay` and `$profile:ChefZ\Logs` and copies the template
`ChefZ_Core/Config/Templates/Core.overlay.json` if `Core.json` is missing. It
**never** overwrites an existing operator file. A directory scan is legal here
because `$profile:` is a real filesystem directory, unlike a PBO.

Settings tuning is documented on [Configuration](Configuration).

### One deliberate exception to the rank order

`ChefZ_Core/Config/Core.json` — the core's own settings — is read **before**
rank 1. `ChefZ_ConfigManager.LoadAll()` states why: `strictMode` and
`safeModeErrorThreshold` decide how errors are handled, so they must be in
effect before the first error occurs, and the log level decides whether anyone
sees the rest. It is safe because the kind `coreSettings` cannot come from rank 1
at all — `ChefZ_ConfigCppSource` has no reader for it and there is no
`CfgChefZ` node for it.

After rank 3 has been read, settings are resolved a second time and the log
reconfigured, because the overlay is allowed to patch them.

### How the ranks merge

All sources feed one `ChefZ_RecordSink` (`3_Game/ChefZ/ChefZ_RecordSink.c`),
which decides per `(kind, id)`:

| Situation | Result |
|---|---|
| first occurrence | accepted |
| same rank, same id | **first wins**, second rejected, ERROR |
| higher rank, same id | field-wise patch, DEBUG |
| lower rank, same id | ignored, WARN (sources were read out of order) |

"First wins" rather than "last wins" is explicit: within a rank the source order
is deterministically sorted, so the merge is reproducible. If the last one won,
the load order of two addons would decide game behaviour.

`state` and `qualityTier` are **sync-relevant** (`ChefZ_RecordKind.IsSyncRelevant`).
Their ordinals are derived independently on client and server from the same
sorted list, so they may only come from rank 1. A rank 2 or 3 record of those
kinds is loaded but gets no ordinal and is reported as an error
(`ChefZ_ConfigManager.BuildIdentities`). An overlay *field patch* on such a
record is allowed — it does not change the id and adds no record, so it cannot
move an ordinal.

---

## 4. The load pipeline

`ChefZ_ConfigManager.LoadAll(isServer)` runs, verbatim from the code:

```
Sources (rank 1 -> 2 -> 3)  ->  Sink  ->  NORMALIZE  ->  MERGE
  -> VALIDATE  ->  ASSIGN IDENTITIES  ->  COMPILE  ->  INDEX
  -> AUDIT  ->  FREEZE
```

Registries are filled in `ChefZ_RecordKind.LoadOrder()` order — the fifteen
kinds listed in section 2, in that sequence. This is a dependency order, not a
preference: each kind is validated against the kinds loaded before it, and
recipes come last because they check against everything.

Managers are then built in a fixed sequence, and `BuildRegistries()` writes out
the ordering constraint for each one: category tree → states → quality tiers →
ingredient bindings → nutrition → preservation → selector context → recipe
engine → recipe quality rules → tool groups → processing → containers →
portions → container audit → nutrition audit → capability wiring → freeze.

Every one of those `Build()` calls is unconditional. A manager with zero records
is *ready and empty*, which answers every query with a quiet `false`. The
alternative — erroring with "called before build" — would make missing content
look like a broken core.

`FreezeAll()` closes every registry. After the boot no registry may grow: a
record inserted later would have no compiled symbol, no sync ordinal and no
position in `Keys()`, so it would differ between client and server.

There is **no runtime reload** in V1. A second `LoadAll()` logs a warning and
does nothing — it would reassign sync ordinals while items already carry state
values.

### Boot timing

`ChefZ_CoreEntry` (`5_Mission/ChefZ/ChefZ_CoreEntry.c`) hooks
`MissionServer.OnInit` and `MissionGameplay.OnInit` and calls `ChefZ_Boot`. That
point is after the engine has merged all configs (so `CfgChefZ` is complete) and
before any player can cook.

One thing has to happen earlier: handcraft recipe slots. Vanilla builds its
recipe list in the `MissionBase` **constructor**, and a vanilla craft action
transmits a recipe's *position* in `m_RecipeList`, not its identity. Two sides
that register the same recipes in a different order mean different things by the
same number. So `ChefZ_HandcraftBridge.Reserve()` runs from `RegisterRecipies()`
and inserts N empty, inert recipe objects at the same point of the
`modded class` chain on both sides; `FillReserved()` later parameterises them
from data without registering anything or moving any position.

N is the sum of `handcraftRecipeSlots` over all `CfgChefZ` slices — the only
number that is provably identical on client and server at that moment. Rank 3 is
deliberately excluded from it, because only the server sees an overlay.

The stated price: a slice with `HANDCRAFT` transforms **must** declare
`handcraftRecipeSlots`. Declaring too few rejects the surplus transforms with a
plain-text error line, loudly and identically on both sides.

### Health states

`ChefZ_ConfigHealth` is `UNINITIALIZED / OK / DEGRADED / SAFE_MODE`.

- `enabled = false` in `Core.json` → core inert, one banner line, pure vanilla.
- zero errors → `OK`.
- `strictMode = true` and any error → `SAFE_MODE`.
- errors above `safeModeErrorThreshold` (default 25) → `SAFE_MODE`.
- otherwise → `DEGRADED`.

`SAFE_MODE` clears every registry and the category tree with it — a surviving
tree would assert memberships for which no data exists. The comment in
`EnterSafeMode()` is the whole doctrine: *"better fully vanilla than half
ChefZ."*

The nutrition audit deliberately does **not** feed into health. Findings that
really break a recipe were already counted by `ChefZ_RecipeCompiler` and the
recipe rejected; counting them twice could push a server over the safe-mode
threshold for a single cause. Everything else the audit finds is balancing, and
a dish that misses its target by 30 % is not a reason to switch off a server's
cooking system.

---

## 5. The central design rule

> If no ChefZ recipe fits, DayZ keeps cooking exactly as it always did.

This is not a promise in a document. It is enforced structurally, in three
places.

### 5.1 The hook itself

`4_World/ChefZ/Cooking/ChefZ_ModdedCooking.c` is the core's only intervention
in the cooking path:

```cpp
modded class Cooking
{
    override int CookWithEquipment(ItemBase cooking_equipment, float cooking_time_coef = 1)
    {
        int vanillaResult = super.CookWithEquipment(cooking_equipment, cooking_time_coef);

        if (ChefZ_CookingHook.ShouldObserve(cooking_equipment))
        {
            ChefZ_CookingHook.AfterVanillaCook(cooking_equipment,
                                               cooking_time_coef,
                                               m_UpdateTime,
                                               GetCookingMethodWithTimeOverride(cooking_equipment));
        }

        return vanillaResult;
    }
}
```

Three properties, named in the file as non-negotiable:

1. `super` is called **unconditionally and as the first statement**. There is no
   condition before it and no code path that skips it.
2. The return value is vanilla's, unchanged.
3. `AfterVanillaCook` returns `void`. There is no return channel — no reference
   with which the already-completed vanilla tick could be undone.

The consequence is structural rather than intentional: if the entire ChefZ half
fails — broken config, empty recipes, an exception mid-evaluation — the tick is
indistinguishable from a server without ChefZ, because vanilla has already
finished. Breaking the rule would require changing the signature, and that shows
up in any diff.

`Cooking` was chosen over `FireplaceBase` because `Cooking.CookWithEquipment` is
the common funnel for *all* vessel cooking — fireplace, barrel, gas stove,
cooking stand, direct-cooking slots. One hook, every device; and a new vanilla
cooking device in a DayZ update needs no core change. `FireplaceBase` would also
be the place where ChefZ collided with every other fireplace mod.

`CookOnStick` and `SmokeItem` are not hooked at all: both work on a single
`Edible_Base` without a vessel, and ChefZ recipes are vessel-based. Vanilla
smoking in a barrel stays exactly vanilla smoking.

The cooking method is **asked for**, not reconstructed: `m_UpdateTime` and
`GetCookingMethodWithTimeOverride` are `protected` and reachable only from
inside a `Cooking` derivation. A reimplementation would be subtly wrong — the
method flips from `BOILING` to `BAKING` once the water has evaporated, and
vanilla refreshes it mid-run.

### 5.2 The no-match path

`ChefZ_CookingDeviceAdapter` runs in four stages of rising cost — gate (a few
comparisons, every tick), signature (one cargo walk, no registry access), full
match (only on real change, additionally throttled), completion (only with a
bound recipe). When the full match finds nothing (`RunFullMatch`,
`4_World/ChefZ/Cooking/ChefZ_CookingDeviceAdapter.c`):

```cpp
if (!matched)
{
    session.state   = ChefZ_ESessionState.DONE;
    session.outcome = null;
    // DEBUG: "no match -> vanilla cooking continues unchanged"
    return true;
}
```

The method's own doc comment says the return value means *"the evaluation ran —
not whether it found something. 'No match' is the most frequent outcome and
explicitly not an error."*

The same exit is taken when a subscriber cancels `ChefZ_OnRecipeMatched`. That
event is raised **before** any effect, so a cancellation costs nothing and
leaves nothing behind — it is the clean lever for hard recipe locks from an
external mod, without the core containing a single word about skills.

`ChefZ_CookingHook.ShouldObserve()` is the cheap pre-test: on a client, with
`enabled = false`, in `SAFE_MODE`, or before the config has loaded, the whole
hook is literally one boolean test.

### 5.3 The rule from the other side

A ChefZ recipe must **not** be fully satisfiable with vanilla ingredients. If it
were, it would hijack vanilla cooking: put three vanilla mushrooms in a pan and
you get a ChefZ dish instead of fried mushrooms, without ever having used
anything from ChefZ.

`tools/chefz-validate/chefzvanilla.mjs` enforces this mechanically. It builds
class → category and class → tag maps from every `ingredient` record (including
category ancestors, so a slot for `MEAT` also accepts `DOMESTIC_MEAT`), then for
each recipe context checks whether every non-optional slot can be satisfied by
at least one non-`ChefZ_` class. If so it emits an error naming the exact
ingredient combination that would trigger it:

> `Invariant I2: recipe "<id>" is fully satisfiable with vanilla ingredients
> (...). At least one MANDATORY ingredient must require a ChefZ item.`

A slot whose satisfier set is empty aborts the check for that context rather
than asserting anything — the checker never claims a violation it cannot back
up. It judges only whether a recipe *is* vanilla-satisfiable; whether that
happens often in play is a human decision, and the rule has no threshold. It
currently reports zero findings across all 44 recipes.

`chefzcookable.mjs` covers the complementary trap: a class that declares
`Food > FoodStages` and `FoodStageTransitions` but whose script chain never
enables cookability, or the reverse, or a food class with no eating action. That
checker exists because a real blocker walked past every other one —
`ChefZ_Edible_Base` did not override `CanBeCooked()`, vanilla's default is
`false`, every ChefZ ingredient stayed `Raw` forever, no `ON_STAGE` recipe could
ever complete, and the validator output was byte-identical before and after the
bug.

---

## 6. Script layers

`ChefZ_Core/config.cpp` declares four script modules. The split is not
decorative — it decides what an object may touch.

| Layer | Files | Lines | Contains |
|---|---|---|---|
| `1_Core` | 57 | 21,245 | record definitions, matcher, selector compiler, symbol table, quality scoring — pure data processing, no engine type |
| `3_Game` | 40 | 28,990 | config manager, sources, sink, registries, managers, event bus — `JsonFileLoader` and `g_Game.ConfigGet*` live here |
| `4_World` | 30 | 17,094 | fact collector, cooking adapter, applicator, station base, portion base, container service — `ItemBase`, `EntityAI` |
| `5_Mission` | 5 | 2,532 | boot, diagnostics, admin commands |

18 of those files are self-tests (`ChefZ_*SelfTest.c`), which is why the matcher
and the config pipeline can be reasoned about without a running game.

Because `1_Core` must not call into `3_Game`, two inversion points exist:
`ChefZ_CapabilityGate` (`1_Core`) declares *where* a capability is asked, and a
`3_Game` implementation hooks itself in. `ChefZ_QualityManager.SetCapabilityProbe()`
does the same for quality rules. Without a provider, both answer "no" and block
nothing — a self-test, a server without config and a server without a capability
registry behave identically.

---

## 7. What the core is allowed to override

Seven `modded class` declarations exist in `ChefZ_Core`:

| Class | File | Purpose |
|---|---|---|
| `Cooking` | `ChefZ_ModdedCooking.c` | the cooking hook (section 5.1) |
| `PluginRecipesManagerBase` | `ChefZ_ModdedRecipes.c` | reserve handcraft slots; `super` first, additive only — `UnregisterRecipe` appears nowhere in the core |
| `ActionWorldCraft`, `WorldCraftActionData`, `WorldCraftActionReciveData` | `ChefZ_ModdedWorldCraft.c` | carry a position-independent recipe identifier alongside vanilla's position, and let the server refuse an action whose position means different things on the two sides |
| `MissionServer`, `MissionGameplay` | `ChefZ_CoreEntry.c` | boot entry points |

Item state is *not* done by overriding vanilla. `ChefZ_Edible_Base` and
`ChefZ_Item_Base` are **derivations**, not `modded class Edible_Base`. The
consequences: no extra byte on a vanilla steak, no extra `OnStoreSave` block in
foreign save files, and no override competing with another food mod for the same
method.

---

## 8. The transaction

`4_World/ChefZ/Cooking/ChefZ_Applicator.c` is the only place in the core that
creates or consumes items. Its order is fixed:

```
1. REVALIDATE   handles still present, same vessel, quantity sufficient?
2. CHECK SPACE  does every output fit into the target?
3. CONTAINER    containerCategory required? find one
4. CREATE       spawn outputs and by-products
                failure -> RollbackCreated(), NOTHING consumed
5. PROPERTIES   state, quality, freshness, portions, temperature, agents, ...
6. CONSUME      last, without exception
7. EVENTS
```

Step 6 being last is the point. Any abort before it leaves the ingredients
untouched. The file's own summary: *"a lost dish is a bug report, lost
ingredients are an angry player."* The residual risk of this ordering — a
duplicate if something fails between step 4 and step 6 — is what
`RollbackCreated()` covers.

Three negative tests are named as acceptance conditions: full cargo, ingredient
removed mid-flight, missing output class. Each must consume nothing and create
nothing. None has been run — see [Known Limitations](Known-Limitations).

---

## 9. Extension points

Comp mods depend on ChefZ; ChefZ never depends on them. `chefzcore.mjs` enforces
that no foreign system name appears in the core, in code *or* in comments.

- **`ChefZ_EventBus`** (`3_Game`) — named events including the cancellable
  `ChefZ_OnRecipeMatched`. `HasSubscribers()` is checked before any payload is
  built, so on a server without comp mods a full match costs one map lookup.
- **`ChefZ_CapabilityRegistry`** / **`ChefZ_CapabilityGate`** — a provider
  answers "how good is this player at X". Without a provider the configured
  default applies and nothing is blocked.
- **`ChefZ_ProgressRegistry`** — a sink for "the player did something"; the Terje
  skills bridge attaches here.

See [Terje Compatibility](Terje-Compatibility) and
[COT Compatibility](COT-Compatibility).

---

## 10. Related pages

- [Modules](Modules) — what each addon contains and depends on
- [Delta Protocol](Delta-Protocol) — how shared vocabulary is merged
- [Adding Content](Adding-Content) — step by step for a new dish, ingredient or station
- [Recipes](Recipes) and [Recipe Reference](Recipe-Reference) — the recipe format
- [Food States](Food-States), [Quality and Nutrition](Quality-and-Nutrition),
  [Portions and Containers](Portions-and-Containers)
- [Validation](Validation) — the fourteen static checkers
- [Known Limitations](Known-Limitations) — what has not been verified
