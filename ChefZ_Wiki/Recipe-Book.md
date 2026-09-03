# Recipe Book

Every recipe ChefZ ships, written the short way: **X + Y = Z**.

This is the player's page. It says what goes in and what comes out, and nothing
else. For the same recipes with quality tiers, policy flags and matching rules, see
[Recipe-Reference](Recipe-Reference); for how the engine picks one, see
[Recipes](Recipes).

Everything here was read out of the shipped JSON and `config.cpp`, not out of the
planning documents.

**Related:** [From-Meat-to-Sausage](From-Meat-to-Sausage) ·
[Recipe-Reference](Recipe-Reference) · [Processing-Stations](Processing-Stations) ·
[Production-Chains](Production-Chains) · [Quality-and-Nutrition](Quality-and-Nutrition)

## How to read a formula

```
2–4× *fish*  +  2–4× *root vegetable*  +  1–2× Carrot   =   Fisherman's Stew (4 helpings)
   optional   1–2× *herb* +2  ·  1× *salt* (6 g) +1  ·  1–2× *spice* +1
   Pot or Cauldron · boil until Boiled
```

| Part | Meaning |
|---|---|
| `2–4×` | How many items the slot takes. A bare `1×` means exactly one — not "one or more" |
| *italic* | A **group**: a category or tag, not a class. Any member satisfies it. The groups are listed in the next section |
| Plain name | One exact item, and only that item |
| `(6 g)` | Only that much is drawn, the rest of the item stays. Everything else is consumed whole |
| `optional` | Leave it out and the recipe still works — it just comes out worse |
| `+2` | Grade points that slot adds. Points decide the quality tier, which decides yield and shelf life |
| `(4 helpings)` | The dish is one item you eat down. 100 units is one helping, one bite is a quarter of that. The empty bowl or plate comes back with the last bite |

**The rule behind all of it:** if no ChefZ recipe matches what is in the pot,
**vanilla cooking runs and nothing is lost**. A recipe that does not fire is not an
error — it is the normal case. See [Recipes](Recipes#1-the-one-rule-that-matters-most).

## What the group names mean

| Group | What counts |
|---|---|
| *meat* | Every raw and cooked meat in the mod, sausages and mince included. The three primal cuts are **not** in it — a Beef, Pork or Venison Leg is category `PRIMAL_CUT` and has to be cut into steaks first |
| *minced meat* | Diced Meat · Minced Meat · Minced Pork · Minced Venison · Minced Boar · Minced Chicken · Minced Bear |
| *wild meat* | Deer, Boar and Rabbit steak · Minced Venison and Minced Boar · the raw and cooked Venison, Boar and Hunter sausages. **Wolf and Bear are not in it** — they are category `PREDATOR_MEAT`, and so is Minced Bear |
| *sausage* | All fourteen sausage classes — raw, cooked, dry and smoked |
| *casing* | Guts · Small Guts |
| *fish* | Carp, Mackerel, Steelhead Trout, Walleye Pollock fillet · Sardines · Bitterlings · Salted, Dried and Smoked Fish |
| *canned fish* | Opened Crab, Sardines and Tuna cans |
| *canned meat* | Brisket Spread · Lunchmeat · Pajka · Pate · Opened Pork Can |
| *root vegetable* | Potato · Carrot · Onion · Garlic |
| *leaf vegetable* | Cabbage |
| *vegetable* | All of the above plus Corn, Tomato, Green Bell Pepper, Sliced Pumpkin, Zucchini and opened Baked Beans |
| *tomato* | Tomato |
| *beans* | Opened Baked Beans |
| *mushroom* | The seven edible mushrooms: Agaricus, Auricularia, Boletus, Craterellus, Lactarius, Macrolepiota, Pleurotus |
| *fruit* | Apple · Pear · Plum |
| *canned fruit* | Opened Peaches |
| *herb* | Fresh **or** dried Parsley, Thyme, Rosemary, Wild Garlic · Herb Mix |
| *fresh herb* | Only the four fresh ones |
| *dried herb* | The four dried ones · Herb Mix |
| *spice* | Salt · Black Pepper · Dried Peppercorns · Dried Paprika · Paprika Powder · Pepper Berries · Chili · Herb Mix · Hunter Seasoning |
| *salt* | Salt |
| *culture* | Mushroom Culture — and only that. It is filed under `SPICE` but has no spice tag, so no recipe that asks for a spice can take it |
| *dairy* | Powdered Milk · Cream · Butter · Cheese · Cheese Curd |
| *cream* / *butter* | Cream / Butter |
| *egg* | Egg |
| *bread* | Bread · Flatbread |
| *dough* / *flour* / *pasta* | Dough / Flour / Fresh and Dried Pasta |
| *broth* | Bone Broth |
| *sauce* | Tomato Sauce · Cream Sauce · Mushroom Cream Sauce |
| *tomato sauce* / *cream sauce* | Tomato Sauce / Cream Sauce and Mushroom Cream Sauce |
| *fat* | Lard |
| *bone* | Bone |
| *sweetener* | Honey |

---

# Part 1 — Preparations

Things you make **before** you cook. None of these happen in cookware; they happen
at a station or in your hands. Full station detail on
[Processing-Stations](Processing-Stations).

## Grain and dough

```
1× Wheat                              =  Flour            Grain Mill, 25 s
1–5× Corn                             =  Flour            Grain Mill, 25 s
1× Flour (250 g)  +  Water (150 ml)   =  Dough            by hand, 8 s
1× Dough                              =  Fresh Pasta      by hand + rolling tool, 10 s
1× Fresh Pasta                        =  Dried Pasta      Drying Rack, 30 min
1× Metal Plate                        =  Pasta Machine    by hand + metalwork tool, 25 s
```

Wheat mills at a ratio of 0.78 — a full ear gives 78 % of its quantity as flour.
Corn is flat-rated at **120 g of flour per cob**, up to five cobs per run.

The rolling tool is tool group `ROLLING_PIN`, whose members are the **Pasta Machine**
and vanilla's **Meat Tenderizer**. There is no actual rolling pin in the mod.

## Herbs, spices and seasoning

```
1× Fresh Parsley       =  Dried Parsley        Drying Rack,  8 min
1× Fresh Thyme         =  Dried Thyme          Drying Rack,  8 min
1× Fresh Wild Garlic   =  Dried Wild Garlic    Drying Rack,  8 min
1× Fresh Rosemary      =  Dried Rosemary       Drying Rack, 10 min
1× Green Bell Pepper   =  Dried Paprika        Drying Rack, 15 min
1× Pepper Berries      =  Dried Peppercorns    Drying Rack, 15 min

1× Dried Peppercorns   =  Black Pepper         Mortar, 20 s
1× Dried Paprika       =  Paprika Powder       Mortar, 20 s

1× Dried Thyme  +  1× Dried Parsley  +  1× Dried Rosemary   =  Herb Mix   Mortar, 25 s

1× Black Pepper  +  1× Paprika Powder  +  1× Dried Thyme
                 +  1× Dried Wild Garlic  +  1× *spice*     =  Hunter Seasoning   Mortar, 35 s
```

**Hunter Seasoning is the best single ingredient in the mod.** It is worth +2 grade
points in Hunter Stew (all three variants) and in Chernarus Chili — more than salt,
spice and herb put together in most recipes.

## Salt

```
1× container of Salt Water, at least 80 % full   =  Raw Salt   boiled, 15 min
1× Raw Salt                                      =  Salt       dried, 20 min
```

Brine boils down at a ratio of 0.04 and raw salt dries at 0.67. A bag of salt holds
200 g, and most recipes draw 3–12 g of it.

## Milk, butter and cheese

```
2× Powdered Milk   =  Cream     Butter Churn, 60 s
2× Cream           =  Butter    Butter Churn, 60 s

1× rotten mushroom =  Mushroom Culture    Mortar, 20 s

3× *dairy*  +  1× *culture*   =  Cheese Curd     Pot or Cauldron, boil 300 s at 60 °C
1× Cheese Curd                =  Cheese          Cheese Press, 5 min
```

Four milk makes one butter. The churn restarts itself after each run and tries
butter first, so a load of milk walks through both stages on its own.

The cheese chain is the newest in the mod and the only one that starts with
something rotten: any of the seven edible mushrooms at vanilla stage `Rotten`,
ground in the mortar, becomes the culture. Poisonous and hallucinogenic mushrooms
are not declared as ingredients and cannot be used.

The `*dairy*` slot for the curd excludes Butter, Cream, Cheese and Cheese Curd
itself — in practice it means **3× Powdered Milk**.

## Honey

```
4× Wooden Plank  +  10× Nail    =  Beehive Kit          by hand, 25 s
1× Beehive Kit                  =  Beehive              by hand + hand tool, 30 s
2× Beehive Kit                  =  Double Beehive       by hand, 30 s
1× Wooden Plank  +  2× Nail     =  Empty Comb Frame     by hand, 12 s
1× Wooden Plank  +  4× Nail     =  Uncapping Fork       by hand, 15 s
1× Opened Tuna Can              =  Bee Smoker           by hand + hand tool, 20 s
1× Bee Smoker (half empty or less)  +  Bark (2)  =  Bee Smoker (full)   by hand, 6 s

1× Full Comb Frame              =  Uncapped Comb Frame  by hand + Uncapping Fork, 18 s
1× Uncapped Comb Frame  +  1× Empty Jar  =  Honey       Honey Extractor, 90 s
```

One uncapped frame yields **four jars of honey**, then retires into an Empty Comb
Frame — the frame survives the harvest and goes back into the hive.

## Preserving meat and fish

```
1× raw meat (not sausage)  +  Salt (20 g)   =  Salted Meat    by hand, 6 s
1× raw fish                +  Salt (20 g)   =  Salted Fish    by hand, 6 s

1× Salted Meat   =  Dried Meat       Drying Rack, 60 min
1× Salted Fish   =  Dried Fish       Drying Rack, 45 min
1× raw fish      =  Smoked Fish      Smoker,       5 min
1× raw sausage   =  Smoked Sausage   Smoker,       5 min
1× raw sausage   =  Dry Sausage      Drying Rack, 90 min

2× Rose Hip      =  Dried Berries    Drying Rack,  7 min
2× Elderberry    =  Dried Berries    Drying Rack,  7 min
```

Spoilage multipliers by state: `DRIED` ×0.15 · `SMOKED` ×0.25 · `SALTED` ×0.50 ·
`COOKED` ×0.80.

> **Salted Meat reaches Smoked Meat since 31.08.2026.** The transform existed in
> the data but named no process and was rejected at boot; it now names
> `PROCESS_SMOKE` and the smoker. See [Known-Limitations](Known-Limitations).

## Plates, bowls, boxes and cans

```
1× Firewood                  =  Empty Plate     by hand + knife, 20 s
1× Firewood                  =  Empty Bowl      by hand + knife, 25 s
1× Birch or Oak Bark         =  Empty Bowl      by hand + knife or axe, 30 s
2× Paper                     =  Empty Food Box  by hand, 12 s
1× Metal Plate               =  10× Empty Can   by hand + hacksaw, 45 s
```

## Meat preparation

The whole meat chain, from the animal to the sausage, has its own page:
**[From-Meat-to-Sausage](From-Meat-to-Sausage)**. In short:

```
1× Beef / Pork / Venison Leg          =  2× steak + bone or lard    by hand + knife, 4 s
1× any raw meat                       =  Diced Meat                 by hand + knife, 4 s
1× raw steak                          =  Minced Meat (6 kinds)      Meat Grinder, 30 s
1× *minced meat* + spice + *casing*   =  Raw Sausage (6 kinds)      Meat Grinder, 15 s
```

---

# Part 2 — The dishes

Everything below happens **in cookware** over a fire: a Frying Pan, a Cooking Pot,
a Cauldron or an indoor oven.

Three ways a dish finishes:

- **`ON_STAGE`** — done when vanilla's own food stage flips to `Baked` or `Boiled`. No timer.
- **`TIMED`** — done after a fixed number of seconds, once the vessel is above a minimum temperature.
- **`INSTANT`** — done the moment the ingredients are together. No heat at all.

## Bowl dishes

Five dishes, twelve recipes. Each has a base version, some have a **Broth** version
that swaps the vessel's water for Bone Broth, and each has a **Group** version that
needs a Cauldron and feeds twelve.

### Hunter Stew

```
2–3× *minced meat*  +  2–4× *root vegetable*  +  2–4× *mushroom*   =  Hunter Stew (4 helpings)
   optional  1–2× *herb* +2 · 1× *salt* (6 g) +1 · 1–2× *spice* +1
   Pot or Cauldron · boil until Boiled · needs 200 ml water in the vessel

1–2× *broth*  +  2–3× *minced meat*  +  2–4× *root vegetable*
              +  2–4× *mushroom*                                   =  Hunter Stew (5 helpings)
   optional  1–2× *herb* +2 · 1× *salt* (6 g) +1 · 1–2× *spice* +1
   Pot or Cauldron · boil or bake · the broth is the liquid

4–6× *minced meat*  +  4–6× *root vegetable*  +  2–4× *tomato*     =  Hunter Stew (12 helpings)
   optional  2–4× *mushroom* +1 · 1–3× *herb* +2 · 1× *salt* (12 g) +1 · 1–2× *spice* +1
   Cauldron · boil until Boiled · needs 600 ml water
```

Thyme is worth +1, Hunter Seasoning +2, herbs above 0.8 freshness another +1.

**The meat must be minced or diced.** A whole raw steak in the pot does not make a
stew — it cooks the vanilla way. That is the difference between a stew and a steak
in a pot.

### Fisherman's Stew

```
2–4× *fish*  +  2–4× *root vegetable*  +  1–2× Carrot              =  Fisherman's Stew (4 helpings)
   optional  1–2× *herb* +2 · 1× *salt* (6 g) +1 · 1–2× *spice* +1
   Pot or Cauldron · boil until Boiled · needs 200 ml water

1–2× *broth*  +  2–4× *fish*  +  2–4× *root vegetable*  +  1–2× Carrot
                                                                   =  Fisherman's Stew (5 helpings)
   optional  1–2× *herb* +2 · 1× *salt* (6 g) +1 · 1–2× *spice* +1

4–6× *fish*  +  4–6× *root vegetable*  +  2–3× Carrot              =  Fisherman's Stew (12 helpings)
   optional  1–3× *herb* +2 · 1× *salt* (12 g) +1 · 1–2× *spice* +1
   Cauldron only
```

The carrot has its own slot on purpose: potatoes alone would otherwise fill the
whole root-vegetable requirement. Parsley is worth +1, fish above 0.85 freshness +1.

### Vegetable Soup

```
3–5× *root vegetable*  +  1–2× *leaf vegetable*                    =  Vegetable Soup (4 helpings)
   optional  1–2× Corn +1 · 1–2× *herb* +2 · 1× *salt* (5 g) +1 · 1–2× *spice* +1
   Pot or Cauldron · boil until Boiled · needs 250 ml water

6–9× *root vegetable*  +  2–3× *leaf vegetable*                    =  Vegetable Soup (12 helpings)
   optional  1–3× Corn +1 · 1–3× *herb* +2 · 1× *salt* (10 g) +1 · 1–2× *spice* +1
   Cauldron only · needs 700 ml water
```

The cheapest dish in the mod and the cheapest group meal — no meat, no station, no
prepared ingredient. Garlic is worth +1.

### Bone Broth Soup

```
2–3× *broth*  +  3–5× *vegetable*  +  1× *herb*                    =  Bone Broth Soup (4 helpings)
   optional  1× *salt* (5 g) +1 · 1–2× *spice* +1 · 1–2× *herb* +2
   Pot or Cauldron · boil or bake

4–6× *broth*  +  6–9× *vegetable*  +  2–4× *herb*                  =  Bone Broth Soup (12 helpings)
   optional  1× *salt* (10 g) +1 · 1–3× *spice* +1
   Cauldron only
```

The herb is **not** optional here: without it the dish would be indistinguishable
from reheated broth.

### Chernarus Chili

```
2–4× *meat*  +  1–2× *beans*  +  2–4× *tomato*
             +  1–2× Green Bell Pepper  +  1–2× Paprika Powder     =  Chernarus Chili (4 helpings)
   optional  1–2× Corn +1 · 1× *salt* (6 g) +1 · 1–2× *spice* +1 · 1–2× *herb* +1
   Pot or Cauldron · 300 s at 60 °C

4–6× *meat*  +  2–3× *beans*  +  4–6× *tomato*
             +  2–3× Green Bell Pepper  +  2–3× Paprika Powder     =  Chernarus Chili (12 helpings)
   optional  1–3× Corn +1 · 1× *salt* (12 g) +1 · 1–3× *spice* +1 · 1–3× *herb* +1
   Cauldron only
```

No water needed — the beans and tomatoes bring it. Any meat works, including whole
steaks. Wild meat is worth +1, black pepper +1, Hunter Seasoning +2.

## Pasta and plates

Ten dishes, all `TIMED`, all two helpings.

```
1–3× *pasta*  +  1–2× *tomato sauce*                               =  Survivor Spaghetti
   optional  1× *salt* (5 g) +2 · 1–2× *spice* +1 · 1–2× *herb* +1
   Pot or Cauldron · 180 s at 60 °C

1–3× *pasta*  +  1–3× *sausage*  +  1× *fat* or *butter*           =  Sausage Pasta
   optional  1× *salt* (5 g) +2 · 1–2× *spice* +1 · 1–2× *herb* +1 · 1× *sauce* +2 · 1–3× *mushroom* +1
   Frying Pan or Pot · 200 s at 60 °C

1–3× *pasta*  +  1–3× *wild meat* (no sausage)  +  1–4× *mushroom*
              +  1–2× *herb*                                       =  Hunter Pasta
   optional  1× *cream* or *cream sauce* +2 · 1× *salt* (5 g) +2 · 1–2× *spice* +1
   Frying Pan, Pot or Cauldron · 240 s at 70 °C

1–3× *pasta*  +  2–5× *mushroom*  +  1–2× *cream*  +  1–2× *herb*  =  Creamy Mushroom Pasta
   optional  1× *salt* (5 g) +2 · 1× *butter* +1 · 1–2× *spice* +1
   Frying Pan, Pot or Cauldron · 210 s at 60 °C

1–3× *pasta*  +  2–3× *dairy* (no butter)                          =  Chernarus Mac and Cheese
   optional  1× *butter* +1 · 1× *salt* (5 g) +2 · 1–2× *spice* +1 · 1–2× *herb* +1
   Pot or Cauldron · 200 s at 60 °C

1–4× Potato  +  1–3× *sausage*  +  1× *fat* or *butter*            =  Sausage and Potatoes
   optional  1× *salt* (5 g) +2 · 1–2× *herb* +1 · 1–2× *spice* +1 · 1–2× *root vegetable* +1
   Frying Pan or Pot · 220 s at 70 °C

1–3× *wild meat* (no sausage)  +  1–3× Potato  +  1–4× *mushroom*
                               +  1–2× *herb*                      =  Hunter Plate
   optional  1× *salt* (5 g) +2 · 1× *fat* or *butter* +1 · 1–2× *spice* +1
   Frying Pan, Pot or Cauldron · 260 s at 70 °C

1–3× *sausage*  +  1–3× Potato  +  1–2× *root vegetable*           =  Blood Sausage Plate
   optional  1× *salt* (5 g) +2 · 1–2× *herb* +1 · 1–2× *spice* +1
   Frying Pan or Pot · 220 s at 70 °C

1–3× *fish* or *canned fish*  +  1–4× Potato  +  1–2× *herb*       =  Fish and Potatoes
   optional  1× *salt* (5 g) +2 · 1× *fat* or *butter* +1 · 1–2× *spice* +1
   Frying Pan or Pot · 200 s at 65 °C

1–2× *beans*  +  1–3× *sausage*  +  1–2× *root vegetable*          =  Beans and Sausage
   optional  1× *salt* (5 g) +2 · 1–2× *spice* +1 · 1–2× *herb* +1
   Frying Pan, Pot or Cauldron · 180 s at 65 °C
```

## Pan and breakfast

Ten more dishes. Two of them need no fire at all.

```
1× Tactical Bacon (opened or sealed)  +  1–2× *egg*  +  1× *bread*  =  Tactical Bacon Breakfast (2 helpings)
   optional  1× *tomato* +1 · *salt* (4 g) +1 · 0–2× *spice*/*herb* +1
   Frying Pan or Pot · bake until Baked · a second egg is worth +1 more

2–4× *egg*  +  1× *dairy*  +  1–2× *sausage*                        =  Scrambled Eggs with Sausage (2 helpings)
   optional  *salt* (4 g) +1 · 0–2× *spice*/*herb* +1
   Frying Pan or Pot · bake · a premium sausage is worth +2

2–4× Potato  +  1–2× *egg*  +  1–2× *sausage*  +  1–2× Onion        =  Farmer's Breakfast (3 helpings)
   optional  1× Corn +1 · 1× *fat* or *butter* +1 · *salt* (5 g) +1 · 0–2× *spice*/*herb* +1
   Frying Pan, Pot or Cauldron · bake

1–2× *dough*  +  1–2× Cheese                                        =  Cheese Flatbread (2 helpings)
   optional  *salt* (4 g) +1 · 1–2× *fresh*/*dried herb* +1 · 0–2× *spice* +1
   Frying Pan or Pot · bake · a second cheese is worth +1, rosemary +1

3–6× *mushroom*  +  1× *butter*                                     =  Mushroom Pan (2 helpings)
   optional  *salt* (4 g) +1 · 1–2× *fresh*/*dried herb* +1 · 1–2× *root vegetable* +1 · 0–2× *spice* +1
   Frying Pan or Pot · bake · parsley +1, garlic +1, fresh mushrooms +1

2–4× Potato  +  Flour (100 g)  +  1–2× *egg*  +  1× *fat* or *butter*  =  Potato Pancakes (2 helpings)
   optional  *salt* (4 g) +1 · 1× Onion +1 · 0–2× *spice*/*herb* +1
   Frying Pan or Pot · bake

1–2× *dough*  +  1–2× *minced meat*  +  1× Onion                    =  Meat Dumplings (3 helpings)
   optional  *salt* (5 g) +1 · 1–2× *spice* +1 · 0–2× *fresh*/*dried herb* +1
   Pot, Cauldron or Frying Pan · boil or bake

1× Rice  +  1–2× *dairy* (no butter)  +  1× *butter*                =  Milk Rice (2 helpings)
   optional  1× Honey +2 · *salt* (3 g) +1
   Pot or Cauldron · 240 s at 60 °C

1–2× *bread*  +  1–2× *sausage*  +  1× Cheese                       =  Sausage and Bread Plate (2 helpings)
   optional  1× *butter* +1 · 1× *canned meat* +1 · 0–2× *spice*/*herb* +1
   INSTANT — no heat needed · premium sausage +2, preserved sausage +1

1–2× *bread*  +  1× Honey                                           =  Honey Bread Platter (2 helpings)
   optional  1× *butter* +2 · 0–1× *spice*/*herb* +1
   INSTANT — no heat needed
```

The two `INSTANT` plates are the only recipes in the mod that produce a finished
dish without a fire. Assemble the parts in a pan or pot and they are done.

## Dishes from vanilla produce

```
3–5× Sliced Pumpkin  +  1× *butter*                                 =  Pumpkin Soup (3 helpings)
   optional  1–3× *root vegetable* +1 · 1× *cream* +2 · 1–2× *herb* +2 · 1× *salt* (6 g) +1 · 1–2× *spice* +1
   Pot or Cauldron · boil until Boiled

4–6× Sardines or Bitterlings  +  1× *fat* or *butter*  +  1–2× Garlic  =  Fried Small Fish (2 helpings)
   optional  1–2× *herb* +3 · 1× *salt* (5 g) +2 · 1–2× *spice* +1
   Frying Pan only · bake until Baked

4–6× *fruit* (not preserved)  +  1–2× Dried Berries                 =  Fruit Compote (3 helpings)
   optional  1× *sweetener* +3 · 1× *canned fruit* +1 · 1× *cream* +1 · 1–2× *spice* +1
   Pot or Cauldron · boil until Boiled
```

Fried Small Fish has the most generous herb bonus in the mod at +3, and Fruit
Compote the most generous sweetener bonus, also +3.

## Sauces and broth

These are **ingredients**, not meals. Each run fills one 100-unit jar.

```
3–6× *tomato*  +  Salt (8 g)                        =  Tomato Sauce
   optional  1–2× *root vegetable* +2 · 1–2× *herb* +2
   Pot or Cauldron · 180 s at 60 °C

1× *cream*  +  1× *butter*  +  Salt (5 g)           =  Cream Sauce
   Pot, Cauldron or Frying Pan · 120 s at 50 °C

3–6× *mushroom*  +  1× *cream*  +  1–2× *herb*  +  Salt (5 g)  =  Mushroom Cream Sauce
   Pot, Cauldron or Frying Pan · 150 s at 50 °C

2–4× *bone*  +  2–4× *root vegetable*  +  1–2× *herb*   =  Bone Broth
   Pot or Cauldron · 420 s at 80 °C
```

Salt is **required** in all three sauces, not optional. Bone Broth is the longest
cook in the mod at seven minutes and the hottest at 80 °C — and it is what turns the
base stews into their five-helping Broth versions.

## Bread

```
1× Dough   =  Bread       Cooking Pot or indoor oven · bake until Baked
1× Dough   =  Flatbread   Frying Pan · bake until Baked
```

Same input, different vessel. The pot and the oven give a loaf; the pan gives a
flatbread, which is what the Cheese Flatbread recipe wants.

## Sausage

```
1× Raw Sausage          =  Cooked Sausage
1× Raw Pork Sausage     =  Pork Sausage
1× Raw Venison Sausage  =  Venison Sausage
1× Raw Boar Sausage     =  Boar Sausage
1× Raw Hunter Sausage   =  Hunter Sausage
1× Raw Spicy Sausage    =  Spicy Sausage
```

Frying Pan, Pot or Cauldron, baked or boiled. Full chain on
[From-Meat-to-Sausage](From-Meat-to-Sausage).

## Cheese

```
3× *dairy* (no butter, cream or cheese)  +  1× *culture*  =  Cheese Curd
   Pot or Cauldron · 300 s at 60 °C
```

Then press it: `1× Cheese Curd = Cheese` at the Cheese Press, five minutes. The
curd is the slowest thing that happens in a cauldron, and that is deliberate — the
cheese chain is a project, not something you do between fights.

---

## Counts

| Group | Recipes | File |
|---|---|---|
| Bowl dishes | 12 | `ChefZ_Cooking/Config/Recipes/BowlDishes.json` |
| Pasta and plates | 10 | `ChefZ_Cooking/Config/Recipes/Dishes_A.json` |
| Pan and breakfast | 10 | `ChefZ_Cooking/Config/Recipes/DishesB.json` |
| Vanilla produce | 3 | `ChefZ_Cooking/Config/Recipes/DishesVanilla.json` |
| Sauces and broth | 4 | `ChefZ_Cooking/Config/Recipes/Sauces.json` |
| Cheese | 1 | `ChefZ_Cooking/Config/Recipes/Cheese.json` |
| Sausage | 6 | `ChefZ_Meat/Config/Recipes/Sausage.json` |
| Bread | 2 | `ChefZ_Baking/Config/GrainRecipes.json` |
| **Cooking recipes** | **48** | |
| Transforms (stations and handcraft) | 62 | across 7 modules |

## See also

- [From-Meat-to-Sausage](From-Meat-to-Sausage) — one chain end to end
- [Recipe-Reference](Recipe-Reference) — the same recipes with quality, policy and timing columns
- [Recipes](Recipes) — how a recipe is written and how the engine picks one
- [Processing-Stations](Processing-Stations) — the fifteen stations and what each one does
- [Production-Chains](Production-Chains) — where every raw ingredient comes from
- [Quality-and-Nutrition](Quality-and-Nutrition) — what grade points buy you
