# Installation

This page describes what a server operator has to do to run ChefZ.

> **Read this first.** The PBOs have been built and the server has been started —
> `tools/chefz-pack/pack.mjs` packs all thirteen, unsigned and unbinarised, and
> `tools/chefz-pack/testrun.ps1` launches the test server. What has never happened
> is a server that *stays up*: it registers every addon, loads its config, and then
> dies in the mission's `OnInit` chain. Nothing here has been signed or binarised,
> and no gate checklist has been run in a live game.
> See [Known Limitations](Known-Limitations).

## 1. What ChefZ consists of

ChefZ is shipped as **four mod folders**, which pack into **fifteen PBOs** — of which
two do not build today, see [Known Limitations](Known-Limitations#two-asset-addons-that-never-reach-a-pbo).

### `Psyerns_ChefZ_Core` — the main mod (12 PBOs)

One PBO per subfolder of `Psyerns_ChefZ_Core/Addons/`. Each subfolder carries a
`$PREFIX$` file whose content is the PBO prefix, and the prefix is the root of
every runtime path inside that PBO.

| Addon (`CfgPatches` class) | `$PREFIX$` | Mandatory? | `requiredAddons[]` |
|---|---|---|---|
| `ChefZ_Core` | `ChefZ_Core` | **yes** | `DZ_Data` |
| `ChefZ_Registry` | `ChefZ_Registry` | yes | `DZ_Data`, `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Ingredients`, `ChefZ_Processing`, `ChefZ_Meat`, `ChefZ_Baking`, `ChefZ_Preservation`, `ChefZ_Cooking` |
| `ChefZ_Farming` | `ChefZ_Farming` | yes | `DZ_Data`, `DZ_Gear_Cultivation`, `DZ_Gear_Food`, `ChefZ_Core` |
| `ChefZ_Processing` | `ChefZ_Processing` | yes | `DZ_Data`, `DZ_Gear_Camping`, `DZ_Gear_Tools`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Farming`, `DZ_Gear_Cooking` |
| `ChefZ_Ingredients` | `ChefZ_Ingredients` | yes | `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Processing`, `DZ_Gear_Consumables` |
| `ChefZ_Meat` | `ChefZ_Meat` | yes | `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing` |
| `ChefZ_Baking` | `ChefZ_Baking` | yes | `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Processing` |
| `ChefZ_Preservation` | `ChefZ_Preservation` | yes | `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing`, `ChefZ_Meat` |
| `ChefZ_Cooking` | `ChefZ_Cooking` | yes | `DZ_Data`, `DZ_Gear_Food`, `DZ_Gear_Cooking`, `ChefZ_Core`, `ChefZ_Ingredients`, `ChefZ_Farming`, `ChefZ_Meat`, `ChefZ_Processing`, `ChefZ_Baking`, `ChefZ_Preservation` |
| `ChefZ_Cookbook` | `ChefZ_Cookbook` | yes | `DZ_Data`, `DZ_Gear_Books`, `ChefZ_Core` |
| `ChefZ_Devices` | `ChefZ\ChefZ_Devices` | yes | `DZ_Data` |
| `ChefZ_Items` | `ChefZ\ChefZ_Items` | yes | `DZ_Data` |

"Mandatory" here means: the dependency graph above is closed. `ChefZ_Cooking`
requires seven other ChefZ addons; `ChefZ_Registry` requires eight. You cannot
ship a subset of the main mod without editing `requiredAddons[]`. Treat
`Psyerns_ChefZ_Core` as one indivisible mod.

`ChefZ_Core` itself only requires `DZ_Data`. It carries no items
(`units[] = {}`) — it is the engine, not the content. See [Modules](Modules).

### The three compatibility mods (1 PBO each, all optional)

These are **standalone mods** loaded in addition to the main mod. Each mod
folder *is* the addon folder — there is no `Addons/` level.

| Mod folder / `CfgPatches` class | `requiredAddons[]` | Needs |
|---|---|---|
| `Psyerns_ChefZ_Terje_Skills_Comp` | `DZ_Data`, `ChefZ_Core`, `ChefZ_Farming`, `TerjeCore`, `TerjeSkills` | Terje Core + Terje Skills |
| `Psyerns_ChefZ_Terje_Medicine_Comp` | `TerjeCore`, `TerjeMedicine`, `ChefZ_Core` | Terje Core + Terje Medicine |
| `Psyerns_ChefZ_COT_Comp` | `JM_COT_Scripts`, `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Ingredients`, `ChefZ_Baking`, `ChefZ_Meat`, `ChefZ_Preservation`, `ChefZ_Processing`, `ChefZ_Cooking` | Community Online Tools |

**ChefZ runs completely without all three.** None of them adds an item class
(`units[] = {}`, `weapons[] = {}` in all three). They add XP attribution, tea
effects, and COT spawn categories respectively. Loading a comp mod without its
foreign mod will make the server refuse to start, because `requiredAddons[]`
names classes that do not exist.

See [Terje Compatibility](Terje-Compatibility) and
[COT Compatibility](COT-Compatibility).

## 2. Building the PBOs

**Nobody has done this yet.** The repository contains source folders only —
no `.pbo`, no `.bikey`, no `.biprivatekey`, no `mod.cpp`, no `meta.cpp`.

### 2.1 Prerequisites

- DayZ Tools (Steam → Library → Tools → *DayZ Tools*)
- A `P:` drive mounted through the *Workdrive* tool
- The `Addon Builder` and `DSUtils` (signing) components installed

### 2.2 Pack each addon separately

Nine packing runs for the main mod, one per subfolder of
`Psyerns_ChefZ_Core/Addons/`. Addon Builder, per addon:

```
Source directory:       ...\Psyerns_ChefZ_Core\Addons\ChefZ_Core
Destination directory:  ...\@Psyerns_ChefZ_Core\addons
```

Then repeat for `ChefZ_Registry`, `ChefZ_Farming`, `ChefZ_Processing`,
`ChefZ_Ingredients`, `ChefZ_Meat`, `ChefZ_Baking`, `ChefZ_Preservation`,
`ChefZ_Cooking`.

And once per compatibility mod:

```
Source directory:       ...\Psyerns_ChefZ_COT_Comp
Destination directory:  ...\@Psyerns_ChefZ_COT_Comp\addons
```

**The prefix is not optional.** Every addon folder carries a `$PREFIX$` file,
and its content must end up as the PBO prefix. If it does not, DayZ silently
skips the script modules of that PBO — no error, no RPT line, just classes that
do not exist at runtime. Addon Builder picks `$PREFIX$` up automatically; verify
it afterwards with a PBO viewer.

The prefixes are exactly the folder names:

```
ChefZ_Core  ChefZ_Registry  ChefZ_Farming  ChefZ_Processing  ChefZ_Ingredients
ChefZ_Meat  ChefZ_Baking    ChefZ_Preservation  ChefZ_Cooking
Psyerns_ChefZ_COT_Comp  Psyerns_ChefZ_Terje_Medicine_Comp  Psyerns_ChefZ_Terje_Skills_Comp
```

### 2.3 Files that must be included, not filtered

Addon Builder's default include list drops file types it does not know. ChefZ
ships data that must survive packing:

- `**/Config/**/*.json` — the whole rank-2 data layer
- `**/stringtable.csv` — display names, next to `config.cpp`
- `**/Config/Templates/Core.overlay.json` — the operator settings template

If the JSON is filtered out, the mod loads and does nothing: the config manager
reports `slices=0 files=0 records=0` and every ChefZ recipe is absent while
vanilla cooking keeps working. See [Troubleshooting](Troubleshooting).

Eleven of the twelve addon folders ship an `include.txt` that tells Addon
Builder what to keep:

```
*.c;*.json;*.csv;*.xml;*.layout;*.txt
```

**`Psyerns_ChefZ_COT_Comp` has no `include.txt`.** Add one with the same
content before packing it, or Addon Builder's default filter may drop its
script files and its `stringtable.csv`, and the COT spawn categories will
simply not exist.

### 2.4 Signing

There are no keys in the repository. If your server runs with
`verifySignatures = 2` you must create your own key pair and sign all twelve
PBOs:

```
DSCreateKey Psyerns_ChefZ
DSSignFile Psyerns_ChefZ.biprivatekey <path to each .pbo>
```

Put `Psyerns_ChefZ.bikey` into the server's `keys\` directory and into
`@Psyerns_ChefZ_Core\keys\` so clients receive it.

### 2.5 Missing mod metadata

There is no `mod.cpp` anywhere in the repository. Without it the DayZ Launcher
shows the mod folder name instead of a title and no picture. This does not stop
a server from starting, but it should be added before any public release.

## 3. Load order

Do **not** rely on the order of `-mod=` alone. DayZ resolves the addon graph
through `requiredAddons[]`, and ChefZ's graph is fully specified. Still, put the
main mod before the comp mods on the command line so the launcher's own
dependency check has nothing to complain about:

```
@Psyerns_ChefZ_Core;@Psyerns_ChefZ_COT_Comp;@Psyerns_ChefZ_Terje_Skills_Comp;@Psyerns_ChefZ_Terje_Medicine_Comp
```

The foreign mods (`@Terje...`, `@CommunityOnlineTools`) must come **before**
their respective comp mod.

Inside ChefZ, load order of the *data* is a separate mechanism and is not
affected by the command line. Each addon declares `loadOrder` in its `CfgChefZ`
node; the config manager reads records in that order.

| Slice | `loadOrder` |
|---|---|
| `ChefZ_Registry` (shared vocabulary) | 150 |
| `ChefZ_Meat` | 200 |
| `ChefZ_Ingredients` (salt / dairy / herbs slices) | 205, 220, 260 |
| `ChefZ_Farming` | 210, 215 |
| `ChefZ_Processing` | 220, 230, 260 |
| `ChefZ_Baking` | 230 |
| `ChefZ_Preservation` | 280 |
| `ChefZ_Cooking` | 300, 310, 330 |

See [Delta Protocol](Delta-Protocol) for what the shared registry at 150 does.

## 4. `-mod` versus `-serverMod`

ChefZ must be loaded with **`-mod`**, on both the server and the client.

- `-mod=` — the mod is part of the client/server contract. Clients must have
  it, and the client loads the same PBOs. This is what ChefZ needs.
- `-serverMod=` — server-side only, never sent to clients.

ChefZ cannot be a `-serverMod`, for three reasons that are all structural:

1. It adds item classes to `CfgVehicles`. A client that does not have them
   cannot render them.
2. The client loads ChefZ config data itself: rank 1 (`CfgChefZ*` in
   `config.cpp`) and rank 2 (JSON in the PBO). Only the rank-3 operator overlay
   is server-only and is synced to clients as a delta.
3. Handcraft recipe slots are reserved from `RegisterRecipies()` on **both**
   sides, at the same point in the modded-class chain, and their count comes
   from `CfgChefZ handcraftRecipeSlots`. If the client does not read the same
   count, crafting recipe IDs drift apart. The mod detects and refuses that
   case, but the result is crafting that silently does nothing.

The **comp mods** follow the same rule. `Psyerns_ChefZ_COT_Comp` registers COT
spawn categories, which are a client-side UI; `Psyerns_ChefZ_Terje_Skills_Comp`
adds a perk to Terje's skill config, which the client reads. Both go in `-mod`.

Example server start line:

```
DayZServer_x64.exe -config=serverDZ.cfg -profiles=profiles -port=2302 ^
  "-mod=@CommunityOnlineTools;@TerjeCore;@TerjeSkills;@TerjeMedicine;@Psyerns_ChefZ_Core;@Psyerns_ChefZ_COT_Comp;@Psyerns_ChefZ_Terje_Skills_Comp;@Psyerns_ChefZ_Terje_Medicine_Comp" ^
  -cpuCount=4 -dologs -adminlog -netlog -freezecheck
```

## 5. `-profiles` is required

ChefZ writes to `$profile:ChefZ`:

```
$profile:ChefZ\Core.json        operator settings (rank 3), created on first start
$profile:ChefZ\README.txt       created on first start
$profile:ChefZ\Overlay\*.json   further operator overlays
$profile:ChefZ\Logs\            log file and load report
```

The directories and the two templates are created on the **first server start**
and are **never overwritten** afterwards. If `-profiles=` is missing or the
directory is read-only, ChefZ still runs on ranks 1 and 2, but it is not tunable
without rebuilding the PBOs, and the RPT carries:

```
[ChefZ][WARN][CONFIG] Unter $profile:ChefZ kann nicht geschrieben werden - das Overlay entfaellt.
```

The client never creates or reads `$profile:ChefZ` — there, `$profile:` is the
player's own directory and an overlay would be a player-owned file deciding
server rules.

## 6. First start: what a healthy RPT looks like

Search the server RPT for `[ChefZ]`. You should see, in this order:

```
[ChefZ][CORE] Selbsttest S2: n/n Gruppen ok
... (one self-test line per subsystem)
[ChefZ][CONFIG] slices=<n> files=<n> records=<n> ok=<n> rejected=0 patched=<n> health=OK in <n>ms
[ChefZ][CORE] Config SERVER  health=OK  records=...  rezepte=...  aktiv=1
[ChefZ][CORE] Aussenkante SERVER  abonnenten=...  faehigkeitsanbieter=...  fortschrittsempfaenger=...  modus=asAuthored
[ChefZ][CORE] Handwerk  rezepte=<n>  abgewiesen=0  plaetze=<n>  ab Rezept-ID <n>  kennsumme=<n>
```

`health=OK` and `rejected=0` is the goal. `health=DEGRADED` means errors below
the safe-mode threshold; `health=SAFE_MODE` means ChefZ has emptied all its
registries and vanilla cooking runs unchanged. All of this is explained in
[Configuration](Configuration) and [Troubleshooting](Troubleshooting).

The `Handwerk` line and the `Config`/`Aussenkante` lines are printed
**regardless of log level** — they exist to be compared between the client RPT
and the server RPT.

## 7. Known noise in the current state

`ChefZ_Core/config.cpp` still ships a temporary smoke test
(`ChefZ_Core/Tests/V_A_PboJsonSmoke/`). It prints a block per start:

```
[ChefZ][V-A] ===== Rauchtest PBO-JSON  seite=SERVER =====
...
[ChefZ][V-A] ===== Ende  seite=SERVER =====
```

This is a measurement artefact, not an error. It answers the still-open
question of whether a JSON file inside a PBO is readable at runtime. It is meant
to be removed once the answer is recorded. Until then, expect two extra RPT
blocks per start (one server, one client).

## 8. Next

- [Configuration](Configuration) — every setting, defaults, and how to override
  them without touching shipped files
- [Troubleshooting](Troubleshooting) — symptom → cause → check → fix
- [Validation](Validation) — run the static checks before you pack
- [Known Limitations](Known-Limitations) — what is unbuilt, unmeasured, or
  unverified
