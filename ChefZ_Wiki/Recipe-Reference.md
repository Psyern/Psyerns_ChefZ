# Recipe Reference

Every cooking recipe ChefZ ships, read straight out of the shipped JSON.

**Totals: 44 recipes** in 6 files across 3 modules.

| Group | File | Module | Recipes |
|---|---|---|---|
| Bowl dishes | `BowlDishes.json` | `ChefZ_Cooking` | 12 |
| Plate dishes A — pasta and plates | `Dishes_A.json` | `ChefZ_Cooking` | 10 |
| Plate dishes B — pan and breakfast | `DishesB.json` | `ChefZ_Cooking` | 10 |
| Sauces and broth | `Sauces.json` | `ChefZ_Cooking` | 4 |
| Sausage — cooking the raw sausages | `Sausage.json` | `ChefZ_Meat` | 6 |
| Bread | `GrainRecipes.json` | `ChefZ_Baking` | 2 |
| **Total** | | | **44** |

> A **recipe** runs inside cookware — Pot, Cauldron, Frying Pan, Oven.
> Everything that happens at a workbench (milling, grinding, drying, smoking,
> stuffing sausage, chopping) is a **transform**, not a recipe. Transforms are
> documented on [Processing-Stations](Processing-Stations).

## How to read the tables

| Column | Meaning |
|---|---|
| **Dish** | The English `displayName` from the module stringtable. For a portioned dish this is the name of the **served portion**, not of the pot. |
| **Recipe ID** | The `id` in the recipe JSON. This is the string that appears in the RPT log. |
| **Cookware** | `contexts[].deviceClasses`. `Cauldron` alone means the recipe is a group recipe. |
| **Required** | Mandatory slots. `3–5×` is the min/max item count. *Italic* names are categories or tags; plain names are exact classes. |
| **Optional** | Optional slots. **+n** is the grade points that slot contributes. |
| **Result** | The output class. For a portioned dish: bulk class → portion count × portion class. |
| **Portions** | Portion count and container category. |

### Quality

Grade points from optional slots are only half of the quality score.
33 of the 44
recipes carry additional `gradeRules` on top of the slot points — typically
+1 for a named herb, +1 for freshness above 0.8, +2 for Hunter Seasoning or a
sausage tagged `CHEFZ_PREMIUM`. The shared tier set `DISH_DEFAULT` is declared
exactly once, in `ChefZ_Cooking/config.cpp`:

| Tier | Rank | Min. score | Yield | Spoilage | Portion bonus |
|---|---|---|---|---|---|
| POOR | 0 | -99 | ×0.75 | ×1.25 | 0 |
| SIMPLE | 1 | 0 | ×1.0 | ×1.0 | 0 |
| PREPARED | 2 | 2 | ×1.1 | ×0.95 | 0 |
| SEASONED | 3 | 4 | ×1.25 | ×0.9 | +1 |
| PREMIUM | 4 | 7 | ×1.5 | — | — |

Details on [Quality-and-Nutrition](Quality-and-Nutrition).

### Policy

32 recipes forbid
`BURNT` and `ROTTEN` inputs and require `minMatchedHealth01` 0.15.
38 of 44
set `extraItems: forbid` — an unexpected item in the vessel makes the recipe
not match, and vanilla cooking runs instead.

## Bowl dishes

Stews and soups. Each one cooks as a **bulk pot** and is then served into bowls; the bowl item is what the player eats. Five dishes, twelve recipes: several have a *Broth* variant that swaps vessel water for Bone Broth, and a *Group* variant that only works in a Cauldron.

**12 recipes** · `ChefZ_Cooking/Config/Recipes/BowlDishes.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Hunter Stew | `RCP_ChefZ_HunterStew` | Pot, Cauldron | 2–3× Diced Meat or Minced Venison or Minced Boar<br>2–4× *ROOT_VEGETABLE*<br>2–4× *MUSHROOM* | 1–2× *CHEFZ_HERB* **+2**<br>1× *SALT* (6 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Hunter Stew (Cookware)<br>→ 4× Hunter Stew | 4 (BOWL) |
| Hunter Stew | `RCP_ChefZ_HunterStewBroth` | Pot, Cauldron | 1–2× *BROTH*<br>2–3× Diced Meat or Minced Venison or Minced Boar<br>2–4× *ROOT_VEGETABLE*<br>2–4× *MUSHROOM* | 1–2× *CHEFZ_HERB* **+2**<br>1× *SALT* (6 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Hunter Stew (Cookware)<br>→ 5× Hunter Stew | 5 (BOWL) |
| Hunter Stew | `RCP_ChefZ_HunterStewGroup` | Cauldron | 4–6× Diced Meat or Minced Venison or Minced Boar<br>4–6× *ROOT_VEGETABLE*<br>2–4× *TOMATO* | 2–4× *MUSHROOM* **+1**<br>1–3× *CHEFZ_HERB* **+2**<br>1× *SALT* (12 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Hunter Stew (Cookware)<br>→ 12× Hunter Stew | 12 (BOWL) |
| Fisherman's Stew | `RCP_ChefZ_FishermanStew` | Pot, Cauldron | 2–4× *FISH*<br>2–4× *ROOT_VEGETABLE*<br>1–2× Carrot or Chopped Carrot | 1–2× *CHEFZ_HERB* **+2**<br>1× *SALT* (6 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Fisherman's Stew (Cookware)<br>→ 4× Fisherman's Stew | 4 (BOWL) |
| Fisherman's Stew | `RCP_ChefZ_FishermanStewBroth` | Pot, Cauldron | 1–2× *BROTH*<br>2–4× *FISH*<br>2–4× *ROOT_VEGETABLE*<br>1–2× Carrot or Chopped Carrot | 1–2× *CHEFZ_HERB* **+2**<br>1× *SALT* (6 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Fisherman's Stew (Cookware)<br>→ 5× Fisherman's Stew | 5 (BOWL) |
| Fisherman's Stew | `RCP_ChefZ_FishermanStewGroup` | Cauldron | 4–6× *FISH*<br>4–6× *ROOT_VEGETABLE*<br>2–3× Carrot or Chopped Carrot | 1–3× *CHEFZ_HERB* **+2**<br>1× *SALT* (12 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Fisherman's Stew (Cookware)<br>→ 12× Fisherman's Stew | 12 (BOWL) |
| Vegetable Soup | `RCP_ChefZ_VegetableSoup` | Pot, Cauldron | 3–5× *ROOT_VEGETABLE*<br>1–2× *LEAF_VEGETABLE* | 1–2× *CHEFZ_HERB* **+2**<br>1× *SALT* (5 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Vegetable Soup (Cookware)<br>→ 4× Vegetable Soup | 4 (BOWL) |
| Vegetable Soup | `RCP_ChefZ_VegetableSoupGroup` | Cauldron | 6–9× *ROOT_VEGETABLE*<br>2–3× *LEAF_VEGETABLE* | 1–3× *CHEFZ_HERB* **+2**<br>1× *SALT* (10 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Vegetable Soup (Cookware)<br>→ 12× Vegetable Soup | 12 (BOWL) |
| Bone Broth Soup | `RCP_ChefZ_BoneBrothSoup` | Pot, Cauldron | 2–3× *BROTH*<br>3–5× *VEGETABLE*<br>1× *CHEFZ_HERB* | 1× *SALT* (5 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+2** | Bone Broth Soup (Cookware)<br>→ 4× Bone Broth Soup | 4 (BOWL) |
| Bone Broth Soup | `RCP_ChefZ_BoneBrothSoupGroup` | Cauldron | 4–6× *BROTH*<br>6–9× *VEGETABLE*<br>2–4× *CHEFZ_HERB* | 1× *SALT* (10 g) **+1**<br>1–3× *CHEFZ_SPICE* **+1** | Bone Broth Soup (Cookware)<br>→ 12× Bone Broth Soup | 12 (BOWL) |
| Chernarus Chili | `RCP_ChefZ_ChernarusChili` | Pot, Cauldron | 2–4× *MEAT*<br>1–2× *BEANS*<br>2–4× *TOMATO*<br>1–2× Paprika or Chopped Paprika or Green Bell Pepper<br>1–2× Paprika Powder | 1× *SALT* (6 g) **+1**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+1** | Chernarus Chili (Cookware)<br>→ 4× Chernarus Chili | 4 (BOWL) |
| Chernarus Chili | `RCP_ChefZ_ChernarusChiliGroup` | Cauldron | 4–6× *MEAT*<br>2–3× *BEANS*<br>4–6× *TOMATO*<br>2–3× Paprika or Chopped Paprika or Green Bell Pepper<br>2–3× Paprika Powder | 1× *SALT* (12 g) **+1**<br>1–3× *CHEFZ_SPICE* **+1**<br>1–3× *CHEFZ_HERB* **+1** | Chernarus Chili (Cookware)<br>→ 12× Chernarus Chili | 12 (BOWL) |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Hunter Stew | `RCP_ChefZ_HunterStew` | BOILING | ON_STAGE | Boiled | — | 200 |
| Hunter Stew | `RCP_ChefZ_HunterStewBroth` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Hunter Stew | `RCP_ChefZ_HunterStewGroup` | BOILING | ON_STAGE | Boiled | — | 600 |
| Fisherman's Stew | `RCP_ChefZ_FishermanStew` | BOILING | ON_STAGE | Boiled | — | 200 |
| Fisherman's Stew | `RCP_ChefZ_FishermanStewBroth` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Fisherman's Stew | `RCP_ChefZ_FishermanStewGroup` | BOILING | ON_STAGE | Boiled | — | 600 |
| Vegetable Soup | `RCP_ChefZ_VegetableSoup` | BOILING | ON_STAGE | Boiled | — | 250 |
| Vegetable Soup | `RCP_ChefZ_VegetableSoupGroup` | BOILING | ON_STAGE | Boiled | — | 700 |
| Bone Broth Soup | `RCP_ChefZ_BoneBrothSoup` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Bone Broth Soup | `RCP_ChefZ_BoneBrothSoupGroup` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Chernarus Chili | `RCP_ChefZ_ChernarusChili` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Chernarus Chili | `RCP_ChefZ_ChernarusChiliGroup` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |

## Plate dishes A — pasta and plates

The `TIMED` half of the plate dishes: they finish after a fixed number of seconds once the vessel is above the minimum temperature, not when a vanilla food stage flips.

**10 recipes** · `ChefZ_Cooking/Config/Recipes/Dishes_A.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Survivor Spaghetti | `RCP_ChefZ_SurvivorSpaghetti` | Pot, Cauldron | 1–3× *PASTA*<br>1–2× *TOMATO_SAUCE* | 1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+1** | Survivor Spaghetti (Cookware)<br>→ 2× Survivor Spaghetti | 2 (PLATE) |
| Sausage Pasta | `RCP_ChefZ_SausagePasta` | FryingPan, Pot | 1–3× *PASTA*<br>1–3× *SAUSAGE*<br>1× *FAT* or *BUTTER* | 1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+1**<br>1× *SAUCE* **+2**<br>1–3× *MUSHROOM* **+1** | Sausage Pasta (Cookware)<br>→ 2× Sausage Pasta | 2 (PLATE) |
| Hunter Pasta | `RCP_ChefZ_HunterPasta` | FryingPan, Pot, Cauldron | 1–3× *PASTA*<br>1–3× *WILD_MEAT* + not *SAUSAGE*<br>1–4× *MUSHROOM*<br>1–2× *CHEFZ_HERB* | 1× *CREAM* or *CREAM_SAUCE* **+2**<br>1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_SPICE* **+1** | Hunter Pasta (Cookware)<br>→ 2× Hunter Pasta | 2 (PLATE) |
| Creamy Mushroom Pasta | `RCP_ChefZ_CreamMushroomPasta` | FryingPan, Pot, Cauldron | 1–3× *PASTA*<br>2–5× *MUSHROOM*<br>1–2× *CREAM*<br>1–2× *CHEFZ_HERB* | 1× *SALT* (5 g) **+2**<br>1× *BUTTER* **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Creamy Mushroom Pasta (Cookware)<br>→ 2× Creamy Mushroom Pasta | 2 (PLATE) |
| Chernarus Mac and Cheese | `RCP_ChefZ_MacAndCheese` | Pot, Cauldron | 1–3× *PASTA*<br>2–3× *DAIRY* + not *BUTTER* | 1× *BUTTER* **+1**<br>1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+1** | Chernarus Mac and Cheese (Cookware)<br>→ 2× Chernarus Mac and Cheese | 2 (PLATE) |
| Sausage and Potatoes | `RCP_ChefZ_SausagePotatoes` | FryingPan, Pot | 1–4× Potato<br>1–3× *SAUSAGE*<br>1× *FAT* or *BUTTER* | 1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_HERB* **+1**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *ROOT_VEGETABLE* **+1** | Sausage and Potatoes (Cookware)<br>→ 2× Sausage and Potatoes | 2 (PLATE) |
| Hunter Plate | `RCP_ChefZ_HunterPlate` | FryingPan, Pot, Cauldron | 1–3× *WILD_MEAT* + not *SAUSAGE*<br>1–3× Potato<br>1–4× *MUSHROOM*<br>1–2× *CHEFZ_HERB* | 1× *SALT* (5 g) **+2**<br>1× *FAT* or *BUTTER* **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Hunter Plate (Cookware)<br>→ 2× Hunter Plate | 2 (PLATE) |
| Blood Sausage Plate | `RCP_ChefZ_BloodSausagePlate` | FryingPan, Pot | 1–3× *CHEFZ_BLOOD_SAUSAGE* or *SAUSAGE*<br>1–3× Potato<br>1–2× *ROOT_VEGETABLE* | 1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_HERB* **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Blood Sausage Plate (Cookware)<br>→ 2× Blood Sausage Plate | 2 (PLATE) |
| Fish and Potatoes | `RCP_ChefZ_FishPotatoPlate` | FryingPan, Pot | 1–3× *FISH*<br>1–4× Potato<br>1–2× *CHEFZ_HERB* | 1× *SALT* (5 g) **+2**<br>1× *FAT* or *BUTTER* **+1**<br>1–2× *CHEFZ_SPICE* **+1** | Fish and Potatoes (Cookware)<br>→ 2× Fish and Potatoes | 2 (PLATE) |
| Beans and Sausage | `RCP_ChefZ_BeanSausagePlate` | FryingPan, Pot, Cauldron | 1–2× *BEANS*<br>1–3× *SAUSAGE*<br>1–2× *ROOT_VEGETABLE* | 1× *SALT* (5 g) **+2**<br>1–2× *CHEFZ_SPICE* **+1**<br>1–2× *CHEFZ_HERB* **+1** | Beans and Sausage (Cookware)<br>→ 2× Beans and Sausage | 2 (PLATE) |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Survivor Spaghetti | `RCP_ChefZ_SurvivorSpaghetti` | BOILING, BAKING | TIMED | 180 s | 60 °C | — |
| Sausage Pasta | `RCP_ChefZ_SausagePasta` | BAKING, BOILING | TIMED | 200 s | 60 °C | — |
| Hunter Pasta | `RCP_ChefZ_HunterPasta` | BAKING, BOILING | TIMED | 240 s | 70 °C | — |
| Creamy Mushroom Pasta | `RCP_ChefZ_CreamMushroomPasta` | BAKING, BOILING | TIMED | 210 s | 60 °C | — |
| Chernarus Mac and Cheese | `RCP_ChefZ_MacAndCheese` | BOILING, BAKING | TIMED | 200 s | 60 °C | — |
| Sausage and Potatoes | `RCP_ChefZ_SausagePotatoes` | BAKING, BOILING | TIMED | 220 s | 70 °C | — |
| Hunter Plate | `RCP_ChefZ_HunterPlate` | BAKING, BOILING | TIMED | 260 s | 70 °C | — |
| Blood Sausage Plate | `RCP_ChefZ_BloodSausagePlate` | BAKING, BOILING | TIMED | 220 s | 70 °C | — |
| Fish and Potatoes | `RCP_ChefZ_FishPotatoPlate` | BAKING, BOILING | TIMED | 200 s | 65 °C | — |
| Beans and Sausage | `RCP_ChefZ_BeanSausagePlate` | BAKING, BOILING | TIMED | 180 s | 65 °C | — |

## Plate dishes B — pan and breakfast

The `ON_STAGE` half of the plate dishes. Two of them, Sausage and Bread Plate and Honey Bread Platter, are `INSTANT` cold plates that need no heat at all.

**10 recipes** · `ChefZ_Cooking/Config/Recipes/DishesB.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Tactical Bacon Breakfast | `RCP_ChefZ_TacticalBreakfast` | FryingPan, Pot | 1× Tactical Bacon (opened) or Tactical Bacon (can)<br>1–2× *EGG*<br>1× *BREAD* | 1× *TOMATO* **+1**<br>*SALT* (4 g) **+1**<br>0–2× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Tactical Bacon Breakfast (Cookware)<br>→ 2× Tactical Bacon Breakfast | 2 (PLATE) |
| Scrambled Eggs with Sausage | `RCP_ChefZ_ScrambledEggSausage` | FryingPan, Pot | 2–4× *EGG*<br>1× *DAIRY*<br>1–2× *SAUSAGE* | *SALT* (4 g) **+1**<br>0–2× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Scrambled Eggs with Sausage (Cookware)<br>→ 2× Scrambled Eggs with Sausage | 2 (PLATE) |
| Farmer's Breakfast | `RCP_ChefZ_FarmersBreakfast` | FryingPan, Pot, Cauldron | 2–4× Sliced Potato or Potato<br>1–2× *EGG*<br>1–2× *SAUSAGE*<br>1–2× Chopped Onion or Onion | 1× *FAT* or *BUTTER* **+1**<br>*SALT* (5 g) **+1**<br>0–2× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Farmer's Breakfast (Cookware)<br>→ 3× Farmer's Breakfast | 3 (PLATE) |
| Cheese Flatbread | `RCP_ChefZ_CheeseFlatbread` | FryingPan, Pot | 1–2× *DOUGH*<br>1–2× Cheese | *SALT* (4 g) **+1**<br>1–2× *HERB* or *DRIED_HERB* **+1**<br>0–2× *SPICE* **+1** | Cheese Flatbread (Cookware)<br>→ 2× Cheese Flatbread | 2 (PLATE) |
| Sausage and Bread Plate | `RCP_ChefZ_SausageBreadPlate` | FryingPan, Pot | 1–2× *BREAD*<br>1–2× *SAUSAGE*<br>1× Cheese | 1× *BUTTER* **+1**<br>0–2× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Sausage and Bread Plate (Cookware)<br>→ 2× Sausage and Bread Plate | 2 (PLATE) |
| Mushroom Pan | `RCP_ChefZ_MushroomPan` | FryingPan, Pot | 3–6× *MUSHROOM*<br>1× *BUTTER* | *SALT* (4 g) **+1**<br>1–2× *HERB* or *DRIED_HERB* **+1**<br>1–2× *ROOT_VEGETABLE* **+1**<br>0–2× *SPICE* **+1** | Mushroom Pan (Cookware)<br>→ 2× Mushroom Pan | 2 (PLATE) |
| Potato Pancakes | `RCP_ChefZ_PotatoPancakes` | FryingPan, Pot | 2–4× Sliced Potato or Potato<br>1× *FLOUR* (100 g)<br>1–2× *EGG*<br>1× *FAT* or *BUTTER* | *SALT* (4 g) **+1**<br>1× Chopped Onion or Onion **+1**<br>0–2× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Potato Pancakes (Cookware)<br>→ 2× Potato Pancakes | 2 (PLATE) |
| Meat Dumplings | `RCP_ChefZ_MeatDumplings` | Pot, Cauldron, FryingPan | 1–2× *DOUGH*<br>1–2× *MINCED_MEAT*<br>1× Chopped Onion or Onion | *SALT* (5 g) **+1**<br>1–2× *SPICE* **+1**<br>0–2× *HERB* or *DRIED_HERB* **+1** | Meat Dumplings (Cookware)<br>→ 3× Meat Dumplings | 3 (PLATE) |
| Milk Rice | `RCP_ChefZ_MilkRice` | Pot, Cauldron | 1× Rice<br>1–2× *DAIRY* + not *BUTTER* | 1× Honey **+2**<br>1× *BUTTER* **+1**<br>*SALT* (3 g) **+1** | Milk Rice (Cookware)<br>→ 2× Milk Rice | 2 (BOWL) |
| Honey Bread Platter | `RCP_ChefZ_HoneyBreadPlate` | FryingPan, Pot | 1–2× *BREAD*<br>1× Honey | 1× *BUTTER* **+2**<br>0–1× *SPICE* or *HERB* or *DRIED_HERB* **+1** | Honey Bread Platter (Cookware)<br>→ 2× Honey Bread Platter | 2 (PLATE) |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Tactical Bacon Breakfast | `RCP_ChefZ_TacticalBreakfast` | BAKING | ON_STAGE | Baked | — | — |
| Scrambled Eggs with Sausage | `RCP_ChefZ_ScrambledEggSausage` | BAKING | ON_STAGE | Baked | — | — |
| Farmer's Breakfast | `RCP_ChefZ_FarmersBreakfast` | BAKING | ON_STAGE | Baked | — | — |
| Cheese Flatbread | `RCP_ChefZ_CheeseFlatbread` | BAKING | ON_STAGE | Baked | — | — |
| Sausage and Bread Plate | `RCP_ChefZ_SausageBreadPlate` | NONE, BAKING | INSTANT | — | — | — |
| Mushroom Pan | `RCP_ChefZ_MushroomPan` | BAKING | ON_STAGE | Baked | — | — |
| Potato Pancakes | `RCP_ChefZ_PotatoPancakes` | BAKING | ON_STAGE | Baked | — | — |
| Meat Dumplings | `RCP_ChefZ_MeatDumplings` | BOILING, BAKING | ON_STAGE | Boiled / Baked | — | — |
| Milk Rice | `RCP_ChefZ_MilkRice` | BOILING | ON_STAGE | Boiled | — | — |
| Honey Bread Platter | `RCP_ChefZ_HoneyBreadPlate` | NONE, BAKING | INSTANT | — | — | — |

## Sauces and broth

Sauces are not meals. They are intermediates that other recipes consume through the categories `TOMATO_SAUCE`, `CREAM_SAUCE`, `SAUCE` and `BROTH`. None of them is portioned, and none of them uses the `DISH_DEFAULT` tier set.

**4 recipes** · `ChefZ_Cooking/Config/Recipes/Sauces.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Tomato Sauce | `RCP_ChefZ_TomatoSauce` | Pot, Cauldron | 3–6× *TOMATO*<br>1× *SALT* (8 g) | 1–2× *ROOT_VEGETABLE* **+2**<br>1–2× *CHEFZ_HERB* **+2** | Tomato Sauce | — |
| Cream Sauce | `RCP_ChefZ_CreamSauce` | Pot, Cauldron, FryingPan | 1× *CREAM*<br>1× *BUTTER*<br>1× *SALT* (5 g) | — | Cream Sauce | — |
| Mushroom Cream Sauce | `RCP_ChefZ_MushroomCreamSauce` | Pot, Cauldron, FryingPan | 3–6× *MUSHROOM*<br>1× *CREAM*<br>1–2× *CHEFZ_HERB*<br>1× *SALT* (5 g) | — | Mushroom Cream Sauce | — |
| Bone Broth | `RCP_ChefZ_BoneBroth` | Pot, Cauldron | 2–4× *BONE*<br>2–4× *ROOT_VEGETABLE*<br>1–2× *CHEFZ_HERB* | — | Bone Broth | — |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Tomato Sauce | `RCP_ChefZ_TomatoSauce` | BOILING | TIMED | 180 s | 60 °C | — |
| Cream Sauce | `RCP_ChefZ_CreamSauce` | BAKING, BOILING | TIMED | 120 s | 50 °C | — |
| Mushroom Cream Sauce | `RCP_ChefZ_MushroomCreamSauce` | BAKING, BOILING | TIMED | 150 s | 50 °C | — |
| Bone Broth | `RCP_ChefZ_BoneBroth` | BOILING | TIMED | 420 s | 80 °C | — |

## Sausage — cooking the raw sausages

These six recipes only turn a raw sausage into its cooked class. The raw sausages themselves are **not** made in cookware — they are stuffed at the [Meat Grinder](Processing-Stations). Each recipe is a single class match with no optional slots, so the grade system does not apply.

**6 recipes** · `ChefZ_Meat/Config/Recipes/Sausage.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Cooked Sausage | `RCP_CookSausage` | FryingPan, Pot, Cauldron | 1+× Raw Sausage | — | Cooked Sausage | — |
| Pork Sausage | `RCP_CookPorkSausage` | FryingPan, Pot, Cauldron | 1+× Raw Pork Sausage | — | Pork Sausage | — |
| Venison Sausage | `RCP_CookVenisonSausage` | FryingPan, Pot, Cauldron | 1+× Raw Venison Sausage | — | Venison Sausage | — |
| Boar Sausage | `RCP_CookBoarSausage` | FryingPan, Pot, Cauldron | 1+× Raw Boar Sausage | — | Boar Sausage | — |
| Hunter Sausage | `RCP_CookHunterSausage` | FryingPan, Pot, Cauldron | 1+× Raw Hunter Sausage | — | Hunter Sausage | — |
| Spicy Sausage | `RCP_CookSpicySausage` | FryingPan, Pot, Cauldron | 1+× Raw Spicy Sausage | — | Spicy Sausage | — |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Cooked Sausage | `RCP_CookSausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |
| Pork Sausage | `RCP_CookPorkSausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |
| Venison Sausage | `RCP_CookVenisonSausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |
| Boar Sausage | `RCP_CookBoarSausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |
| Hunter Sausage | `RCP_CookHunterSausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |
| Spicy Sausage | `RCP_CookSpicySausage` | BAKING, BOILING | ON_STAGE | Baked / Boiled | — | — |

## Bread

Both bread recipes take a dough and bake it. The doughs come from handcraft transforms — see [Production-Chains](Production-Chains).

**2 recipes** · `ChefZ_Baking/Config/GrainRecipes.json`

| Dish | Recipe ID | Cookware | Required | Optional (grade points) | Result | Portions |
|---|---|---|---|---|---|---|
| Bread | `REC_ChefZ_Bread` | Pot, FryingPan, OvenIndoor | 1× Yeast Dough | — | Bread | — |
| Flatbread | `REC_ChefZ_Flatbread` | FryingPan, Pot | 1× Simple Dough | — | Flatbread | — |

### Timing

| Dish | Recipe ID | Method | Completion | Done at | Min. temp. | Liquid used |
|---|---|---|---|---|---|---|
| Bread | `REC_ChefZ_Bread` | BAKING | ON_STAGE | Baked | — | — |
| Flatbread | `REC_ChefZ_Flatbread` | BAKING | ON_STAGE | Baked | — | — |

## Effect IDs

32 of the
44 recipes attach opaque effect IDs to their output. ChefZ Core
never evaluates them; they exist so an effect module can pick them up later.

| Effect ID | Recipes carrying it |
|---|---|
| `CHEFZ_ENERGIZED` | 6 |
| `CHEFZ_HEALTHY_MEAL` | 6 |
| `CHEFZ_HEARTY_MEAL` | 11 |
| `CHEFZ_HUNTERS_MEAL` | 5 |
| `CHEFZ_HYDRATED` | 10 |
| `CHEFZ_WARM_MEAL` | 30 |

## Contradictions and gaps found while compiling this page

| Finding | Detail |
|---|---|
| Salt slots without counts | The eight optional `salt` slots in `DishesB.json` declare neither `minCount` nor `maxCount`. Every other optional salt slot in the mod writes `"minCount": 1, "maxCount": 1`. Affected: `RCP_ChefZ_TacticalBreakfast`, `RCP_ChefZ_ScrambledEggSausage`, `RCP_ChefZ_FarmersBreakfast`, `RCP_ChefZ_CheeseFlatbread`, `RCP_ChefZ_MushroomPan`, `RCP_ChefZ_PotatoPancakes`, `RCP_ChefZ_MeatDumplings`, `RCP_ChefZ_MilkRice`. |
| Sausage slots without `maxCount` | All six recipes in `Sausage.json` declare `"minCount": 1` and no `maxCount`. |
| Paprika bound by class, not category | `RCP_ChefZ_ChernarusChili` and its group variant match `ChefZ_Paprika`, `ChefZ_ChoppedPaprika`, `GreenBellPepper` and `ChefZ_PaprikaPowder` by class, because no `PAPRIKA` category exists and the three ingredient records belong to other modules. The recipe file states this openly. |
| Ingredients with no in-game source | `Rice` and `Honey` (required by Milk Rice and Honey Bread Platter) are vanilla loot. `ChefZ_Milk`, `ChefZ_Egg` and `ChefZ_Yeast` are loot-only ChefZ classes — and the mod ships **no `types.xml`**, so nothing spawns without an admin. See [Known-Limitations](Known-Limitations). |
| Sausage recipes reachable only through a broken chain | The six cooked sausages need raw sausages, which need the Meat Grinder, which currently has no cargo. See [Processing-Stations](Processing-Stations). |

## See also

[Recipes](Recipes) · [Processing-Stations](Processing-Stations) ·
[Production-Chains](Production-Chains) · [Quality-and-Nutrition](Quality-and-Nutrition) ·
[Portions-and-Containers](Portions-and-Containers) · [Food-States](Food-States) ·
[Known-Limitations](Known-Limitations)
