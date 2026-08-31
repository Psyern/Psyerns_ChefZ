# chefz-recipecards

Turns the recipes of the mod into DayZ-style infographic pages for the wiki.
Node, no dependencies. One command, all pages.

```bash
node tools/chefz-recipecards/index.mjs           # SVG
node tools/chefz-recipecards/index.mjs --png     # SVG + PNG
npm run generate:recipes                         # dasselbe, mit PNG
```

Output: `ChefZ_Wiki/images/recipes/recipes-NN.svg` (and `.png`), plus the page
`ChefZ_Wiki/Recipe-Cards.md`, which is rewritten on every run.

## Where the recipes come from

**Not from a hand-written list.** The recipes are read from the mod itself —
`Addons/*/Config/Recipes/*.json` and the `CfgChefZRecipes` nodes — through
`tools/chefz-validate/chefzdata.mjs`, the same parser the validators use.

That matters more than it looks. A second, hand-kept `recipes.json` would have
been out of date the day it was written, and the brief forbids hard-coded
recipes for exactly that reason. The way it stands, a new dish appears on a card
as soon as it exists, and a recipe the validator cannot read does not silently
turn into a pretty picture.

## What a card shows

```
              RECIPE NAME · VARIANT
   [ ingredient grid ]   →  🔥  →   [ result, larger ]
   FRYINGPAN · 65°C · 3 MIN · EMPTY
```

| Part | Source |
|---|---|
| Title | `displayName` of `outputs[0].cls`, resolved through the stringtable |
| Variant | only when two recipes share a name — the differing part of their ids |
| Cells | one per **minimum** required item; a slot asking 1–3 draws one cell and notes `1-3` |
| Labels | `ANY` category/tag · `OR` anyOf · `ALL` allOf · `EXCEPT` allOf with `not` · `OPT` optional |
| Result | `outputs[0]`, with its quantity in the corner |
| Footer | device, minimum temperature, cook time, returned container |

The grid is **computed, not fixed**. Recipes run from one cell (Bread: dough) to
eighteen (Chernarus Chili as a group portion); `fitGrid()` picks the column count
that yields the largest cell that still fits. A card cannot overflow, whatever a
future recipe asks for.

## Item images

`item-images.json` maps a key to a picture:

```json
{ "ChefZ_Cheese": "assets/items/cheese.png", "cat:SALT": "assets/items/salt.png" }
```

Keys are stable identifiers, never display text:

* `ChefZ_Corn` — a concrete class (`match.cls`)
* `cat:ROOT_VEGETABLE` — a category slot; any member may stand for it
* `tag:CHEFZ_HERB` — a tag slot

**The repository ships no item images.** The 52 `.paa` files are model textures,
not inventory icons, and no browser opens a `.paa`.

So a cell falls back through three stages:

1. the real picture from `item-images.json`
2. a **vector glyph** from `icons.mjs` — a carrot, a fish, a bowl
3. a dashed red box, only if no family matches either

The glyphs exist because a page of empty boxes is not an infographic; a drawn
carrot says more in one cell than the word `ROOT`. They do **not** buy silence:
every glyph-drawn key is still counted and listed as *"a vector symbol stands in,
it is NOT an item photo"*. Today that is all 96 keys, and none falls through to
stage 3.

`icons.mjs` holds about forty drawings and a family rule, not one picture per
class — a new dish that asks for carrots gets its symbol without a line of work.
Two things it learned the hard way:

* The **form sits at the end** of a class name. `ChefZ_BoneBrothSoupBowl` is a
  bowl, not a bone, and `ChefZ_CheeseFlatbread` is bread, not a cheese wedge. The
  suffix pass therefore runs before the ingredient rules; the first draft drew
  both wrong.
* The display text is read **last, and only up to the first exclusion**. A slot
  reading `Dairy −Butter −Cream` is dairy — matching on the excluded word painted
  three butter blocks onto the cheese card.

Add PNGs under `assets/items/`, enter them here, and they take precedence on the
next run.

## PNG

SVG is written natively. PNG is rasterised with headless Chrome or Edge if one is
installed; set `CHEFZ_CHROME` to point at another binary. Without a browser the
run says so and keeps the SVGs — it does not fail.

## Options

```
--sort <name|id|slots>      order of the recipes            (default: name)
--cols <n> --rows <n>       grid per page                   (default: 4x3)
--per-page <n>              cards per page, overrides cols*rows
--width <px> --height <px>  page size                       (default: 1920x1080)
--out <dir>                 where the graphics go
--wiki <file>               which markdown file is rewritten
--png                       also rasterise PNG
--strict                    missing item images become an error (exit 1)
--force                     overwrite a wiki file this tool did not write
```

Everything visual lives in `style.mjs` — colours, card metrics, label palette.
The renderer contains no number of its own.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | pages written; missing images are reported but not fatal |
| 1 | invalid recipe data (a card could not be built), or `--strict` with gaps |
| 2 | the run could not start — bad option, or a wiki file that is not ours |

Missing images are deliberately *not* fatal: today they are the normal state, and
a tool that is always red stops being read. `--strict` is there for a build that
wants the opposite.

## The guard on the wiki file

The first run wrote to `ChefZ_Wiki/Recipes.md` — an existing page about the
recipe engine, linked from twelve other pages. It was restored from git and the
default target moved to `Recipe-Cards.md`. Since then every generated page opens
with

```html
<!-- generated by tools/chefz-recipecards - do not edit by hand -->
```

and the tool refuses to touch a file that does not carry that marker. `--force`
exists, and should stay unused.

## Files

| File | Job |
|---|---|
| `index.mjs` | CLI, pagination, PNG, wiki markdown, reports |
| `model.mjs` | mod records → render model (names, cells, labels, variants) |
| `card.mjs` | one recipe card as SVG |
| `page.mjs` | one page: title, grid of cards |
| `style.mjs` | every colour and every measurement |
| `items.mjs` | class → image, and the bookkeeping of what is missing |
| `icons.mjs` | the vector glyphs, the family rule and the legend names |
| `item-images.json` | the mapping itself |
