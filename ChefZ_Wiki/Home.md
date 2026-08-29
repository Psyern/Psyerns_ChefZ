# Psyerns ChefZ

A cooking and food-production overhaul for DayZ.

ChefZ turns eating from a pickup into a production chain: grow and gather, process
at a station, season, cook, preserve, serve. It adds farming, herbs, milling,
butchery, sausage-making, dairy, drying, smoking, salting and beekeeping, and
47 recipes that turn the results into actual meals.

**It extends vanilla cooking. It never takes it over.** If no ChefZ recipe matches
what is in the pot, DayZ cooks the way it always did. That rule runs both ways: a
ChefZ recipe that could be satisfied entirely with vanilla ingredients would hijack
vanilla cooking, so the build refuses to contain one. See [Architecture](Architecture).

> ### Read this before running it
>
> **ChefZ compiles and boots, and does not yet keep a server running.** On
> 28.08.2026 every script module compiled clean, the server bound its port and all
> addons of the time registered — then the process died in the mission's `OnInit`
> chain, with the core in safe mode and its registries empty. Nothing has been
> signed or binarised, and no gate checklist has been executed in a live game.
> [Known Limitations](Known-Limitations) is the full inventory, and it is the right
> page to start on if you are considering a live server.

## Start here

| I want to… | Page |
|---|---|
| Put this on a server | [Installation](Installation) |
| Change how it behaves | [Configuration](Configuration) |
| Fix something that is wrong | [Troubleshooting](Troubleshooting) |
| Understand how it is built | [Architecture](Architecture) |
| Add a dish or an ingredient | [Adding Content](Adding-Content) |
| Look up what exists | [Recipe Reference](Recipe-Reference) |

## What is in it

| | |
|---|---:|
| Addons | 12 — ten with content, two assets only |
| Optional compatibility mods | 3 |
| Recipes | 47 |
| Ingredient records | 112 — 60 ChefZ classes, 52 vanilla items mapped in |
| Processing transforms | 61 |
| Processing stations | 11 |
| Ingredient categories | 41 |
| Food tags | 19 |
| Script files | 171 in the mod, 22 in the comp mods |
| Translated strings | 401 keys × 13 languages |

## How it is organised

The core is a rule machine with no vocabulary of its own. It knows what a recipe is,
what a category is, what a processing step is — but not that "Hunter Stew" or
"venison" exist. Those come from the content modules as data.

The practical consequence: **adding a dish never requires touching core code.** A new
recipe is a JSON record and a config class. That is the property the whole design is
built around, and [Adding Content](Adding-Content) walks through it.

```
ChefZ_Core          systems, no content
ChefZ_Registry      the merged vocabulary from all modules
ChefZ_Ingredients   base ingredients, intermediates, spices
ChefZ_Farming       found plants and herbs, beekeeping
ChefZ_Processing    stations, tools, processing steps
ChefZ_Meat          minced meat, sausages, meat products
ChefZ_Preservation  salting, drying, smoking
ChefZ_Baking        dough, bread, flatbread, pasta
ChefZ_Cooking       plates, bowls, stews, breakfasts, sauces
ChefZ_Cookbook      recipe knowledge and RPC - no UI yet
ChefZ_Devices       models and textures only - hive, beekeeper
ChefZ_Items         models and textures only - frames, jar, carrot, smoker
```

Three optional mods hook ChefZ into other systems. **ChefZ runs fully without all
three** — the dependency only ever points one way, and the core contains no
reference to any of them.

- [Terje Skills](Terje-Compatibility) — survival XP, the herbalist perk, recipe locks
- [Terje Medicine](Terje-Compatibility) — herbal teas, immunity, food risk
- [Community Online Tools](COT-Compatibility) — ChefZ items in the admin spawn menu

## Compatibility

Built and checked against DayZ 1.29. Class names are checked against Terje,
Expansion, Community Framework, Community Online Tools and Dabs Framework — 16,352
foreign class names — with one gap noted in
[Known Limitations](Known-Limitations).

## Contributing

Content modules never write the shared registries directly; they hand in a delta and
a single integrator merges it. This is what makes parallel work on the mod possible
without losing edits. See [Delta Protocol](Delta-Protocol).

Before opening a pull request, run the validation:

```bash
node tools/chefz-validate/index.mjs
```

Nineteen checkers, zero errors expected. [Validation](Validation) explains what each
one covers — and, just as usefully, what none of them can see.
