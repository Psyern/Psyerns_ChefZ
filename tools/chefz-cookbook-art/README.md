# chefz-cookbook-art

Makes the illustrated pages of the in-game cookbook (`ChefZ_Cookbook_UI`).
One page per dish: a worn field-notebook page in DayZ style with four numbered
panels and **no text**.

```
 1  ingredients on a plank        2  everything in the pan / pot
 3  the vessel over a campfire    4  the finished dish
```

The words — title, amounts, device, time, temperature, water, the four step
lines — are set by the UI from the recipe registry and the stringtable. A page
therefore never goes stale when a recipe is rebalanced, and it reads in every
language the stringtable has.

## Pipeline

```bash
node   tools/chefz-cookbook-art/prompts.mjs   # recipes -> ChefZ_Docs/Kochbuch/prompts.json
#      generate one 1:1 image per prompt (Higgsfield, model gpt_image_2_5,
#      quality high, resolution 1k = 1024 x 1024), save as
#      ChefZ_Docs/Kochbuch/source/<OutputClass>.png
python tools/chefz-cookbook-art/build.py      # PNG -> _co.paa + config.cpp block
```

`build.py` needs Pillow and the DayZ Tools (`ImageToPAA.exe`). It exits 1 if a
page is not DXT1 1024 x 1024 or the markers in the config are missing.

## Where the prompts come from

Not from a hand-written list: `prompts.mjs` reads the recipes through
`buildModel()` of `tools/chefz-recipecards`, which uses the validators' parser.
One prompt per **result class** (`outputs[0].cls`) — the group and broth
variants of a dish share its page. The base recipe (not `…Group`, not
`…Broth`) drives the picture.

The four panels follow the mechanic, not the cookbook cliché: ChefZ has no
cutting and no turning in flour. The first pilot (26.10.2026) showed both and
was wrong in-game for exactly that reason.

The style block and the rules block are the same for every page. Change them
and the whole book has to be regenerated, or it looks like it came from two
hands.

## Art direction overrides

Two small tables in `prompts.mjs`, for where the data alone does not make a
good picture:

| Table | Use |
|---|---|
| `ART` | per result class: replace the ingredient text, panel 2 or panel 4 (bread and flatbread lie on the plank, not in a bowl) |
| `WORDS` | rename an ingredient the model would print as a label (`Tactical Bacon Can Opened` → `an opened can of bacon`) |

Recipes with a single ingredient (the six sausages) get an automatic "only this
one item, nothing else" — without it the model adds onions and garlic and cuts
the result up.

## Adding a dish

1. Recipe JSON as usual — the book shows the dish with text and a "no drawing
   yet" hint even without a picture.
2. `node tools/chefz-cookbook-art/prompts.mjs`, generate the new prompt's image,
   save it as `source/<OutputClass>.png`.
3. `python tools/chefz-cookbook-art/build.py` — writes the `.paa` and the
   `CfgChefZCookbookPages` entry. No script change.

## Why PAA, not EDDS

The first build wrote Enfusion EDDS (BGRA8 + LZ4). The notebook paper is noise
to LZ4: 4.8 MB per page, 206 MB for the book. DXT1 PAA is 0.7 MB per page,
28 MB for 43 pages. `ImageWidget.LoadImageFile` loads PAA — LBmaster_Core does
the same with its logo (`LBAppearanceConfig.c:22`).

Power-of-two sizes only: a non-POT texture renders as a white box without any
error.

## Files

| Path | In git |
|---|---|
| `tools/chefz-cookbook-art/` | yes |
| `ChefZ_Core/Addons/ChefZ_Cookbook_UI/data/pages/*_co.paa` | yes — the shipped pages |
| `ChefZ_Docs/Kochbuch/` (prompts, source PNGs, pilots, job ids) | no — `ChefZ_Docs/` is ignored |
