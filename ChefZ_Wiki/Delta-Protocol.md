# Delta Protocol

This page is for contributors. It explains why a content module never writes the
central registries itself, what a delta file looks like, what the integrator
does with it, and how collisions are resolved.

If you only want to add a dish, an ingredient or a station, start with
[Adding Content](Adding-Content) — it tells you when you need a delta and when
you do not.

## 1. The problem

Categories, tags, nutrition values and spoilage rules are **shared vocabulary**.
A recipe in `ChefZ_Cooking` writes `{ "category": "SAUSAGE" }` and expects that
category to exist; the category itself is a consequence of what `ChefZ_Meat`
produces. Several modules need the same names.

If every module declared its own share of the shared registries directly, two
things would break:

1. **Same-rank duplicates.** `ChefZ_RecordSink` (`3_Game/ChefZ/ChefZ_RecordSink.c`)
   rejects a second record with the same `(kind, id)` from the same rank — first
   wins, second is an error. It does **not** merge them. Two modules both
   declaring the category `VEGETABLE` would produce a load error, not a shared
   category.
2. **No single owner.** Nobody could answer "what is the complete category tree"
   without reading nine `config.cpp` files and thirty JSON documents.

So the registries have exactly one writer, and everyone else files a request.

## 2. The two halves

```
Psyerns_ChefZ_Core/_deltas/<slice>.json      written by the content author
                    |
                    |  chefz-registry-integrator
                    v
Psyerns_ChefZ_Core/Addons/ChefZ_Registry/Config/
    Categories.json  Tags.json  Nutrition.json  Preservation.json
```

A **delta** is the content author's contribution to the shared vocabulary. It is
not loaded by the game — it is not inside any PBO path and no `dataFiles[]`
entry points at it. It is a build-time input.

`ChefZ_Registry` is the merged result, and only the merged result. See
[Modules](Modules#chefz_registry) for why it is a separate addon rather than part
of the core.

There are currently fifteen deltas: `apiary`, `dairy`, `dishes-a`, `dishes-b`,
`dishes-c`, `dishes-vanilla`, `grain`, `herbs`, `meat`, `preservation`, `produce`,
`salt`, `sauces`, `serving`, `vanilla-foods`.

The last three joined after the original twelve: `vanilla-foods` and
`dishes-vanilla` when ChefZ started filing vanilla items into its own categories
rather than cloning them, and `apiary` with beekeeping. `apiary` and `serving` are
the two thin ones — they declare only `processes` and `classes`, no vocabulary.

## 3. What a delta looks like

A real one — `Psyerns_ChefZ_Core/_deltas/salt.json`, complete and unedited:

```json
{
    "slice": "salt",

    "categories": [
        { "id": "SPICE", "parent": null, "displayName": "#STR_CHEFZ_CAT_SPICE" },
        { "id": "SALT", "parent": "SPICE", "displayName": "#STR_CHEFZ_CAT_SALT" }
    ],

    "tags": [
        { "id": "CHEFZ_SALT" }
    ],

    "processes": [
        { "id": "PROCESS_BOIL_BRINE", "station": "ChefZ_FryingPan", "durationSec": 900 },
        { "id": "PROCESS_DRY_SALT", "station": "ChefZ_FryingPan", "durationSec": 1200 }
    ],

    "preservation": [
        { "id": "SALTED", "scope": "state", "spoilageMultiplier": 0.5 }
    ],

    "classes": [
        "ChefZ_RawSalt",
        "ChefZ_Salt",
        "ChefZ_FryingPan"
    ]
}
```

### The sections

| Section | Key | Goes into the registry? | Purpose |
|---|---|---|---|
| `slice` | — | no | who filed this |
| `categories` | `id` | **yes** → `Categories.json` | category tree |
| `tags` | `id` | **yes** → `Tags.json` | tags |
| `nutrition` | `class` | **yes** → `Nutrition.json` | nutrition values |
| `preservation` | `id` | **yes** → `Preservation.json` | spoilage multipliers |
| `processes` | `id` | **no** — see section 6 | collision check only |
| `states` | `id` | **no** — see section 5 | collision and reference check only |
| `classes` | — | no | announcement, so other slices can name a class that does not exist yet |

`classes` is the reason a delta can be filed before the `config.cpp` exists.
`deltas.mjs` warns for each announced class that is not yet defined anywhere,
and a process may name a station that is only announced — so the salt chain can
reference `ChefZ_FryingPan` before anyone has modelled it.

## 4. What the integrator does

The integrator (`chefz-registry-integrator`) is the only writer of
`Psyerns_ChefZ_Core/Addons/ChefZ_Registry/**`. It never corrects content in a
module folder — it reports the error back to the owning slice.

Its steps:

1. **Read** every file in `_deltas/*.json`.
2. **Collision check** per section, keyed as in the table above.
3. **Reference check.** Every `parent` category must exist after the merge.
   Every `station` named by a process must exist — either as a real class in
   some `config.cpp` or as an announced class in some delta.
4. **Deterministic merge.** Slices sorted alphabetically, records within a slice
   sorted by id. The same input must always produce the same output file,
   otherwise every run generates meaningless diffs.
5. **Write** the four registries.
6. **Run** `node tools/chefz-validate/index.mjs`.

### The rename is the work

A delta uses the field names the content author finds natural. The registry uses
the field names the core actually deserialises, taken from
`ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_*Def.c`. Translating between them is the
integrator's job, and it is not optional — a field the core has no member for is
at best ignored.

Nutrition is the clearest case. Delta:

```json
{ "class": "ChefZ_Parsley", "energy": 15, "water": 12, "stomach": 5 }
```

Registry, after the merge:

```json
{ "id": "ChefZ_Parsley", "scope": "class", "energy": 15, "water": 12, "fullness": 5 }
```

- `class` → `id` — `ChefZ_Record` keys everything on `id`.
- `stomach` → `fullness` — `ChefZ_NutritionDef` has `fullness`, not `stomach`.
- `scope` is added explicitly. `ChefZ_NutritionScope` accepts `class`,
  `category` or `tag`.

Tags lose a field entirely. A delta may write:

```json
{ "id": "CHEFZ_HERB", "displayName": "#STR_CHEFZ_TAG_HERB",
  "appliesTo": ["ChefZ_Parsley", "ChefZ_Thyme", ...] }
```

`ChefZ_TagDef` has only `id` and `displayName`. There is no `appliesTo` — a tag
does not know its members; an ingredient binding names its own tags in
`tags[]`. So the merged `Tags.json` record is just
`{ "id": "CHEFZ_HERB", "displayName": "#STR_CHEFZ_TAG_HERB" }` and the membership
information lives where the core reads it.

Every registry file carries the core's document form:

```json
{ "kind": "category", "schemaVersion": 1, "records": [ ] }
```

with `kind` being `category`, `tag`, `nutrition` or `preservation`.

**If a delta uses a field name the core does not have, that is a conflict to
report — not a rename to make quietly.** A silent rename hides the fact that the
author and the core disagree about what the field means.

## 5. What deltas do *not* become registry files

### States

`preservation.json` declares all ten food states in a `states` section. None of
them ends up in `ChefZ_Registry` — there is no `States.json`, and that is
deliberate.

States are **sync-relevant**. `ChefZ_RecordKind.IsSyncRelevant()` returns true
for `state` and `qualityTier`, and `ChefZ_ConfigManager.BuildIdentities()` will
only assign a sync ordinal to a record that came from rank 1, because client and
server must derive the same ordinal independently from the same sorted list. A
state loaded from JSON stays loaded but gets no ordinal and is reported as an
error.

So the ten states live in `ChefZ_Preservation/config.cpp` under
`class CfgChefZStates`. Their `states` section in the delta exists for one
reason: `deltas.mjs` checks every `preservation` record with `scope: "state"`
against it and **errors** if the state is not declared anywhere. A spoilage rule
for a state nobody declared is statically clean and completely inert at runtime
— `ChefZ_PreservationManager.Build()` never finds the id — and a green run that
makes a dead system look finished is worse than a red one.

The same applies to quality tiers, which live in `ChefZ_Cooking/config.cpp`
under `class CfgChefZQualityTiers`.

### Processes — the deliberate omission

There is no `Processing.json` in `ChefZ_Registry`, and this is the one place
where the current code contradicts the older project documents. `CORE_REGISTRIES`
in `tools/chefz-validate/lib.mjs` lists four files, not five, with the reason
written next to it.

Process records belong to the slices that already declare them authoritatively —
in rank 1 (`CfgChefZProcesses`) or rank 2 (`kind: "process"` JSON). If the
registry also carried them, it would introduce the same id **in the same rank**
a second time. `ChefZ_RecordSink` rejects a same-rank duplicate rather than
patching it, so `PROCESS_MILL`, `PROCESS_KNEAD` and `PROCESS_ROLL` were not
writable at all. Neither the registry nor the slices were wrong on their own —
the mixture was.

The `processes` section of a delta therefore still exists and is still checked:
`deltas.mjs` runs the cross-slice id collision check over it and verifies each
named station. Its result is a report, not a file.

The comment in `ChefZ_Registry/config.cpp` records this as decision **K1**,
taken after step S19.

## 6. Collision resolution

Two slices defining the same id differently is a conflict. The integrator
**never** overwrites silently. A conflict is reported with: which id, which
slices, which definitions, and a proposed resolution — and the workflow hands it
back to the owning content author.

Exactly two cases may be resolved without asking.

### Identical definitions — silent dedup

`herbs.json` and `meat.json` both file:

```json
{ "id": "DRIED", "scope": "state", "spoilageMultiplier": 0.15 }
```

Byte-identical after key-sorted serialisation, so one record is kept and nothing
is reported. Both slices genuinely need dried goods to keep for the same length
of time; requiring one of them to delete it would only create a hidden
dependency.

### `parent: null` against a concrete parent — the concrete one wins

This one is live in the current tree. `herbs.json` declares:

```json
{ "id": "DRIED_HERB", "parent": "HERB", "displayName": "#STR_CHEFZ_CAT_DRIED_HERB" }
```

and `meat.json` declares the same category with `parent: null`. Everything
except `parent` is equal, and one of the two parents is `null`, so the concrete
parent wins and the run carries a warning:

```
W  Psyerns_ChefZ_Core/_deltas/meat.json:17
   Kategorie "DRIED_HERB": Slice "herbs" und "meat" unterscheiden sich nur im
   parent (HERB / null) - der konkrete gewinnt
```

This is the only warning `deltas.mjs` currently produces. It is a warning and
not an error because the resolution is unambiguous: a slice that does not know
the parent writes `null`, a slice that does know it writes it down.

**Everything else goes back.** Two different `spoilageMultiplier` values for the
same state, two different parents that are both concrete, two different energy
values for the same class — none of those has a defensible automatic answer.

## 7. What the validator checks

`tools/chefz-validate/deltas.mjs`, in order:

1. **Cross-slice id collisions** in `categories`, `tags`, `processes`,
   `nutrition`, `preservation`, `states`. Identical → dedup; the `parent` case
   above → warning; anything else → **error**.
2. **Parent categories** must exist after the merge; a category may not be its
   own parent; no cycles. All three are **errors**.
3. **Stations** named by a process must be defined in a `config.cpp` or
   announced in some delta's `classes`. **Error** otherwise.
4. **Preservation rules with `scope: "state"`** must point at a state some delta
   declares. **Error** — see section 5.
5. **Announced classes** that exist in no `config.cpp` yet → **warning**.
6. **Did the merge arrive?** For each of the four registry files: does it exist,
   is it readable, and does it contain every id that appears in a delta? A
   missing file is a warning ("integrator not run yet?"), a missing id is an
   **error** ("the merge is incomplete").

Check 6 is the one that catches the failure mode that actually happened: a delta
filed, a registry not regenerated, and nothing else noticing.

Run it with:

```
node tools/chefz-validate/index.mjs --only=deltas
```

or as part of the full run — see [Validation](Validation).

## 8. Checklist for a content author

You need a delta when your module introduces:

- a new **category** or **tag** that anything outside your module might name
- a **nutrition value** for a class you own
- a **spoilage rule** for a state or a category
- a new **process id** (for the collision check, even though it produces no file)
- a new **state** (for the reference check; the state itself goes into your
  `config.cpp` under `CfgChefZStates`)

You do **not** need a delta for recipes, transforms, stations, ingredient
bindings, containers, devices, tool groups or quality tiers. Those are declared
directly in your module and read from there.

When you file one:

1. Add or extend `Psyerns_ChefZ_Core/_deltas/<your-slice>.json`.
2. List every class your module will define in `classes`, even the ones that do
   not exist yet.
3. **No `_comment` fields.** This applies to the delta as much as to a data file
   — see [Adding Content](Adding-Content#pitfalls).
4. Run the integrator, then `node tools/chefz-validate/index.mjs`.
5. If a collision is reported, resolve it with the other slice's owner. Do not
   edit their delta and do not edit `ChefZ_Registry` by hand.

## Related pages

- [Adding Content](Adding-Content) — the practical walkthrough
- [Modules](Modules) — what each slice owns
- [Architecture](Architecture) — ranks, merge rules and load order
- [Validation](Validation) — the full checker list
- [Known Limitations](Known-Limitations)
