# Validation

ChefZ ships a static validator: **eighteen checkers**, no dependencies, Node 18
or newer. It reads the project on disk and reports what is wrong before a server
ever sees it.

The tool lives in `tools/chefz-validate/`. Every path it prints is relative to
the project root.

---

## 1. Running it

```bash
node tools/chefz-validate/index.mjs               # readable report
node tools/chefz-validate/index.mjs --json        # JSON report, for tooling
node tools/chefz-validate/index.mjs --only=chefzsym,chefzcore
node tools/chefz-validate/selftest.mjs            # do the checkers still see?
node tools/chefz-validate/build-refindex.mjs      # rebuild the reference index
```

`--only=` takes a comma-separated list of checker names and runs just those, in
the fixed report order. Unknown names are silently skipped.

`CHEFZ_VALIDATE_ROOT=<path>` moves the project root. It exists for the self-test
and nothing else.

Run the full pass **before every pack**. It is fast enough that there is no
reason not to.

---

## 2. Exit codes

| Code | Meaning |
|---|---|
| `0` | clean — 0 errors. Warnings may still be present. |
| `1` | errors found |
| `2` | **a validator itself crashed** — the whole result is unusable |

Code `2` is the one that matters most. A checker that always reports "passed" is
indistinguishable from a checker that is broken, so the runner treats a tool
failure as worse than a finding. In the readable report it looks like this:

```
[WERKZEUGFEHLER] chefzsym
  Validator "chefzsym" ist selbst fehlgeschlagen: <message>
```

and the closing line says:

```
ERGEBNIS: <n> Validator(en) selbst fehlgeschlagen - Ergebnis unbrauchbar.
```

Do not read past that. Fix the tool first.

---

## 3. Reading the report

```
────────────────────────────────────────────────────────────────────────
ChefZ statische Validierung
────────────────────────────────────────────────────────────────────────

[ok] schema  (0 Fehler, 0 Warnungen)

[hinweise] configcpp  (0 Fehler, 24 Warnungen)
  W  Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/config.cpp:752
     Klasse "ChefZ_BulkRawToBaked" steht nicht in units[] von CfgPatches

[FEHLER] classrefs  (3 Fehler, 1 Warnungen)
  E  Psyerns_ChefZ_Core/Addons/ChefZ_Meat/Config/Recipes/Sausage.json:41
     ...

────────────────────────────────────────────────────────────────────────
ERGEBNIS: NICHT BESTANDEN - 3 Fehler, 25 Warnungen.
────────────────────────────────────────────────────────────────────────
```

- Badge `[ok]` — no errors, no warnings
- Badge `[hinweise]` — warnings only
- Badge `[FEHLER]` — at least one error
- `E` / `W` prefixes the finding; `info` findings are not printed in the
  readable report but **are** present in `--json`
- `(projektweit)` instead of a file path means the finding is not tied to one
  file

The checker order is the reading order: first the **form** of the files, then
the **meaning** of their content, last the **rules of the core itself**.

### Severity follows runtime

A finding is an **error** exactly when the core would reject the record at
runtime, or when the result would be silently wrong. Otherwise it is a
**warning**.

- Unknown category in a selector → **error**. The selector compiler rejects it.
- Unknown station in `stationsAllowed` → **warning**. It may come from an
  optional module.

If a namespace is empty project-wide, `chefzsym` reports **one** line saying
"not checkable" instead of a hundred errors. The core alone carries no content;
a validator that is red in the normal state gets ignored after two weeks.

### JSON output

`--json` emits the full report including `info` findings:

```json
{
  "root": "...",
  "checks": [
    { "name": "schema", "ok": true, "errors": 0, "warnings": 0, "items": [] }
  ],
  "errors": 0,
  "warnings": 90,
  "toolFailures": 0,
  "ok": true
}
```

Each item carries `severity`, `summary`, and where applicable `file` and `line`.

---

## 4. The eighteen checkers

### Form of the files

| File | Checks |
|---|---|
| `schema.mjs` | Shape of every ChefZ JSON — recipes, ingredients, deltas. Mandatory fields, types, duplicate recipe IDs, **unknown fields (typos)**. |
| `configcpp.mjs` | `config.cpp` and `$PREFIX$` per module. `CfgPatches` present and unique, `requiredAddons` set, `units[]` complete, no class defined twice, every `modded class` named. |
| `classrefs.mjs` | Every class referenced from JSON, and every parent class, exists — in the project, in a delta, or in the reference index. Covers `cls`, `portionClass`, `emptyClass`, `emptyOnLastPortion`, `returnContainer` and `deviceClasses[]`. |
| `naming.mjs` | `ChefZ_PascalCase`; no collision with foreign classes. |
| `stringtable.mjs` | Every `#STR_CHEFZ_*` is defined; no duplicates; orphaned keys as a warning; the full fifteen-column set is present. |
| `deltas.mjs` | ID collisions between slices, parent categories, category cycles, stations, and whether a merge actually arrived in the central registries. See [Delta Protocol](Delta-Protocol). |

### Meaning of the content

| File | Rule |
|---|---|
| `chefzsym.mjs` | Every symbol reference in JSON and in `CfgChefZ*` exists in the merged registries. Closed value lists (`completion`, `exec`, `scope`, cooking stages, cooking methods …) are checked too. |
| `chefzcore.mjs` | Inside `Addons/ChefZ_Core/**`: no foreign-system name, no content identifier, no content enumeration, no content data record. |
| `chefznut.mjs` | Every edible result and portion class has `class Nutrition` **or** `class Food`, **and** `scope != 0`. Otherwise `PlayerStomach` never registers it and the bite is silently ineffective. |
| `chefzstage.mjs` | Every cookable ChefZ class declares `FoodStageTransitions` — otherwise it burns in the pot. |
| `chefzproc.mjs` | `HANDCRAFT` transforms: 1 to 2 inputs, tool only with a single input, at most 10 outputs. |
| `chefzlog.mjs` | No unguarded `ChefZ_Log.Debug/Trace` call inside a loop. |

`chefzsym` and `chefzcore` are **conditions, not extras**. Without them the
data-driven design is worse than an enum-based one, and the invariants that
keep content out of the core would be declarations of intent instead of rules.

### The central design rule, checked mechanically

| File | Rule |
|---|---|
| `chefzvanilla.mjs` | **Invariant I2.** A ChefZ recipe that can be satisfied *entirely with vanilla ingredients* breaks the rule from the other side: three vanilla mushrooms in a pan would produce a ChefZ dish without the player ever touching ChefZ. Only whether a recipe *is* vanilla-satisfiable is checked; whether it happens often in play is a human judgement, and the rule knows no threshold. |
| `chefzcookable.mjs` | Declared cooking path against the switch that is actually on. Three rules: **A** — class declares `Food > FoodStages` and `FoodStageTransitions` but no class in its script chain enables cookability; the transitions are dead text. **B** — the script class says `CanBeCooked() == true` but the config class has no `Food > FoodStages`; no `FoodStage` object exists and the cooking tick hits nothing. **C** — the class is food but no class in its script chain registers an eat action; vanilla puts eat actions on each food class individually, not on `Edible_Base`. |

### The hard language rules

Added after the first real compiler run, on 28.08.2026. Every rule here cost a
build-and-start cycle before it existed.

| File | Rule |
|---|---|
| `enforce.mjs` | Ten hard Enforce rules: no ternary, no `GetGame()` since 1.29, no `var`/`auto`, no `?.`/`??`, no `delete`, no parent on a `modded class`, one variable per declaration, `ref` only on members, no line beginning with an operator, no variable named like a type. |
| `chefzbase.mjs` | A parent class must be resolvable inside its own `config.cpp` — with a body or as a forward declaration. Otherwise DayZ aborts with *Undefined base class*, in a modal window nobody on a server ever clicks away. |
| `chefzmanaged.mjs` | Anything held by `ref` must be `Managed`, including plain members where the compiler says nothing at all. Without it nothing counts references and the object is freed under the pointer. |
| `chefzswitch.mjs` | A `case` label must be a literal. `static const int FLAG = 1 << 3;` compiles and then matches nothing at runtime — silently. |


`chefzcookable` exists because of a blocker that walked past every other
checker: `ChefZ_Edible_Base` did not override `CanBeCooked()`, vanilla's default
is `false`, and every ChefZ ingredient with a ChefZ script class stayed on `Raw`
forever. **The validator output was byte-identical before and after the bug.**

Note on rule A: "switched on" does not mean `return true`. The core computes the
answer from the class's data. Only a literal `return false;` and nothing else
counts as *switched off* — that is an author's decision, not a gap.

### What `chefzcore` does not punish

Prose. Several core files explicitly write down which content names must **not**
appear there; punishing that would penalise documenting the rule. What is
checked is the code — identifiers and string literals.

**Exception:** foreign-system names are reported in comments too, because a
comment hook is the beginning of a dependency. A foreign-system name **in code**
is an error, without exception. In a **comment** it is a warning, because the
project's vanilla findings prove their claims against shipped foreign mods — and
a proof is the opposite of a dependency but looks exactly like a hook. To keep
the proof, write `I4-BELEG` in the comment block; the finding then drops to an
`info`. That is the only door, it is narrow, and it is greppable:

```bash
grep -rn "I4-BELEG" Psyerns_ChefZ_Core/Addons/ChefZ_Core
```

String literals prefixed `CHEFZ_` are self-test markers and are exempt: the
prefix is reserved and cannot name content.

---

## 5. The self-test

A checker that never finds anything is indistinguishable from a broken
checker — both report "passed".

`selftest.mjs` therefore builds a **throwaway module** in a temporary directory,
one that violates every rule on purpose, runs `index.mjs` against it
(`CHEFZ_VALIDATE_ROOT`), asserts that each rule fires at least once and that the
run ends with exit code `1`, and deletes the tree again.

```bash
node tools/chefz-validate/selftest.mjs            # table: fires / BLIND
node tools/chefz-validate/selftest.mjs --verbose  # plus every individual finding
```

Output:

```
────────────────────────────────────────────────────────────────────────
S19 - Selbstpruefung der Validatoren am Wegwerf-Modul
────────────────────────────────────────────────────────────────────────
zuendet  chefzsym    unbekannte Kategorie in einem Selektor
zuendet  chefzsym    unbekannter Zustand in setStateAfter
zuendet  chefzsym    Wert ausserhalb der geschlossenen Liste
zuendet  chefzsym    keine Vanilla-Garstufe
zuendet  chefzsym    CoreSettings gegen die geschlossene Liste
zuendet  chefzcore   I4: Fremdsystemname im Code
zuendet  chefzcore   I4: Fremdsystemname im Kommentar (Warnung)
zuendet  chefzcore   I3: Content-Wort im Core
zuendet  chefzcore   I3: ID eines Content-Moduls im Core
zuendet  chefzcore   I3: Aufzaehlung ueber eine Content-Dimension
zuendet  chefzcore   I3: Item im Core
zuendet  chefznut    01 V7: Ergebnis ohne Nutrition
zuendet  chefznut    01 V7: scope 0
zuendet  chefzstage  01 V4: kochbar ohne Uebergaenge
zuendet  chefzproc   01 V12: HANDCRAFT mit mehr als zwei Eingaengen
zuendet  chefzlog    18 E2: Wache fehlt
────────────────────────────────────────────────────────────────────────
Wegwerf-Modul: Exit-Code 1 (erwartet 1), 19 Fehler, 10 Warnungen.
ERGEBNIS: BESTANDEN - jede Regel hat mindestens einmal gezuendet.
────────────────────────────────────────────────────────────────────────
```

A `BLIND` line means the rule did not fire on a case that was built to trigger
it. Treat it as a broken checker, not as a passing project.

> **Coverage gap.** The self-test covers **16 rules across 6 checkers**:
> `chefzsym`, `chefzcore`, `chefznut`, `chefzstage`, `chefzproc`, `chefzlog`.
> The other eight — `schema`, `configcpp`, `classrefs`, `naming`,
> `stringtable`, `deltas`, `chefzvanilla`, `chefzcookable` — are **not**
> exercised by the throwaway module. If one of them silently stops finding
> things, nothing here will say so.

---

## 6. The reference index and the known gap

`refindex/*.txt` — one class per line, `#` starts a comment. Built automatically
from the neighbouring mod repositories.

| File | Lines |
|---|---|
| `expansion-classes.txt` | 7789 |
| `vanilla-scripts-classes.txt` | 6537 |
| `terje-classes.txt` | 802 |
| `cot-classes.txt` | 582 |
| `dabs-classes.txt` | 280 |
| `cf-classes.txt` | 196 |
| **`vanilla-classes.txt`** | **0 (header comment only)** |

### The gap

**`refindex/vanilla-classes.txt` is empty.** DayZ's vanilla *item* classes live
in the game configs, not in `scripts - 1.29` — that source contains script
classes, not config classes. As long as the file is empty:

- `classrefs.mjs` cannot verify a reference to a vanilla class. It reports one
  project-wide warning instead:
  ```
  Referenzindex enthaelt 14947 Mod-Klassen, aber keine Vanilla-Item-Klassen.
  Referenzen auf Vanilla (CookingPot, Edible_Base ...) bleiben ungeprueft.
  Abhilfe: refindex/vanilla-classes.txt befuellen.
  ```
- `naming.mjs` cannot detect a **name collision with a vanilla item class**.
  Today it emits 56 warnings of the form:
  ```
  Klasse "Baked" ohne ChefZ_-Praefix - Erweiterung einer Vanilla-Klasse?
  Nicht pruefbar, vanilla-classes.txt ist leer
  ```
  Most of those are legitimate — `Food`, `Baked`, `Boiled`, `Burned`, `Rotten`,
  `FoodStageTransitions` are vanilla config sub-classes. But a genuine collision
  would look exactly the same, and that is the risk.

Unknown `ChefZ_`-prefixed classes are unaffected by the gap and are **always**
an error.

### Closing the gap

Either build the index from unpacked game data:

```bash
node tools/chefz-validate/build-refindex.mjs "D:/path/to/unpacked/dayz/data"
```

The extra argument is added as a source and written to
`refindex/<foldername>-classes.txt`; move or merge its entries into
`vanilla-classes.txt`.

Or produce a class dump on a test server and paste the names into
`refindex/vanilla-classes.txt` by hand — one class per line, `#` for comments.

Once the file has content, the 56 `naming` warnings and the one `classrefs`
warning either disappear or turn into real findings. Both outcomes are progress.

---

## 7. What the checkers cannot do

They read files. That is the whole boundary. **Not checked, and no amount of
green output implies it is fine:**

- **Runtime behaviour.** Whether a recipe actually fires, whether a session
  advances, whether an action appears. Only a server can answer that — see
  [Troubleshooting](Troubleshooting).
- **Cooking logic.** Whether the matcher picks the recipe you intended.
- **Balancing.** Whether a dish is worth its ingredients, whether XP is fair,
  whether spoilage rates make sense.
- **Models and textures.** Nothing here opens a `.p3d` or a `.paa`.
- **The PBO build.** Whether Addon Builder keeps the JSON, whether the prefix
  survived, whether signing worked. See [Installation](Installation).
- **Whether the Enforce script compiles.** The validator parses script files
  textually; it is not a compiler.

Four further areas are unchecked **with a stated reason**, and `chefzsym` says
so in the report rather than staying quiet about it:

- **Effect IDs** — opaque, defined by a foreign system
- **Capabilities** — a foreign module registers them at runtime
- **Event names** — an open namespace by design
- **Liquids** — `cfgLiquidDefinitions` lives in the game config

One more, worth knowing before you trust a green run:

- **`schema.mjs` validates against the documented form.** The *form* of the real
  shipped data files is not covered by it.

---

## 8. Current state of this repository

Last full run:

```
node tools/chefz-validate/index.mjs    -> Exit 0
                                          0 errors, 55 warnings, 18/18 checkers green,
                                          0 tool failures
node tools/chefz-validate/selftest.mjs -> Exit 0, 16/16 rules fire
```

Where the 90 warnings sit:

| Checker | Errors | Warnings | What they are |
|---|---|---|---|
| `schema` | 0 | 0 | |
| `configcpp` | 0 | 24 | classes not listed in `units[]` of `CfgPatches` |
| `classrefs` | 0 | 1 | the empty `vanilla-classes.txt` |
| `naming` | 0 | 56 | non-`ChefZ_` classes, not checkable without the vanilla index |
| `stringtable` | 0 | 0 | |
| `deltas` | 0 | 1 | category `DRIED_HERB` differs only in `parent` between two slices |
| `chefzsym` | 0 | 0 | |
| `chefzcore` | 0 | 0 | |
| `chefznut` | 0 | 8 | result classes without `Nutrition`/`Food` — seeds, empty plates, salt |
| `chefzstage` | 0 | 0 | |
| `chefzproc` | 0 | 0 | |
| `chefzlog` | 0 | 0 | |
| `chefzvanilla` | 0 | 0 | |
| `chefzcookable` | 0 | 0 | |

The eight `chefznut` warnings are the honest kind: the checker cannot decide
statically whether `ChefZ_RawSalt` or `ChefZ_EmptyPlate` is *supposed* to be
edible, so it names the consequence and leaves the decision to a human. If the
class should be edible, add a `Nutrition` block. If not, the warning may stand.

---

## 9. Layout of the tool

```text
index.mjs        runner, report, exit code
lib.mjs          files, config.cpp (flat and as a tree), script classes, reference index
chefzdata.mjs    the ChefZ data model: records from rank 1 + rank 2 + deltas, merged registries
chefzfood.mjs    food facts for chefznut and chefzcookable (inheritance, Food nodes, edibility)
<checker>.mjs    one rule group each, default export returns findings
selftest.mjs     throwaway module, checks the checkers
refindex/        known foreign classes, one per line
```

Adding a checker means adding a `.mjs` file with a default export returning
`{ items: [...] }` and registering its name in the `CHECKS` array in
`index.mjs`. Add a case to `selftest.mjs` in the same commit, or the new checker
is unverifiable by construction.

---

## 10. Portability note

`chefzcookable.mjs` hard-codes an absolute path to the vanilla script sources:

```js
const VANILLA_SCRIPTS = 'C:/Users/Administrator/Desktop/Mod Repositories/scripts - 1.29';
```

`build-refindex.mjs` hard-codes the repository root the same way. On any other
machine those paths do not exist. The checkers degrade rather than crash, but
their coverage drops silently. Adjust both constants, or set up the same
directory layout, before trusting a run on a different machine.

---

## Next

- [Troubleshooting](Troubleshooting) — for everything the validator cannot see
- [Adding Content](Adding-Content) — what the checkers expect from new data
- [Delta Protocol](Delta-Protocol) — what `deltas.mjs` enforces
- [Known Limitations](Known-Limitations)
