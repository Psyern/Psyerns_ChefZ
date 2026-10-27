# ChefZ-Items mit Vanilla-Proxy-Modell (ohne eigene p3d)

Stand 27.10.2026. Erzeugt aus den Configs in `ChefZ_Core/Addons/*/config.cpp` (nur `scope = 2`, Vererbung aufgeloest).
Ein Item steht hier, wenn sein wirksames `model` NICHT unter `\ChefZ\...` liegt.

**31 Items** in 5 Addons.

## ChefZ_Cookbook (1)

| Name (DE) | Klasse | Vanilla-Modell | Modell gesetzt in |
|---|---|---|---|
| Kochbuch | `ChefZ_CookbookItem` | `\dz\gear\books\book_kniga.p3d` | ChefZ_CookbookItem |

## ChefZ_Cooking (4)

| Name (DE) | Klasse | Vanilla-Modell | Modell gesetzt in |
|---|---|---|---|
| Rahmsauce | `ChefZ_CreamSauce` | `\dz\gear\food\Marmalade.p3d` | Marmalade |
| Leere Dose | `ChefZ_EmptyCan` | `\dz\gear\food\food_can_open.p3d` | ChefZ_EmptyCan |
| Leerer Teller | `ChefZ_EmptyPlate` | `\dz\gear\cooking\FryingPan.p3d` | ChefZ_EmptyPlate |
| Pilzrahmsauce | `ChefZ_MushroomCreamSauce` | `\dz\gear\food\Marmalade.p3d` | Marmalade |

## ChefZ_Farming (5)

| Name (DE) | Klasse | Vanilla-Modell | Modell gesetzt in |
|---|---|---|---|
| Bienenstock-Bausatz | `ChefZ_BeehiveKit` | `\DZ\gear\camping\wooden_case.p3d` | ChefZ_BeehiveKit |
| Pfefferbeeren | `ChefZ_PepperBerries` | `\dz\gear\food\Sambucus_nigra.p3d` | ChefZ_PepperBerries |
| Frischer Thymian | `ChefZ_Thyme` | `\dz\gear\cultivation\plant_material.p3d` | ChefZ_FreshHerbBase |
| Weizen | `ChefZ_Wheat` | `\dz\gear\food\Rice.p3d` | ChefZ_Wheat |
| Wilder Thymian | `ChefZ_WildThyme` | `\dz\gear\cultivation\plant_material.p3d` | ChefZ_WildThyme |

## ChefZ_Ingredients (15)

| Name (DE) | Klasse | Vanilla-Modell | Modell gesetzt in |
|---|---|---|---|
| Schwarzer Pfeffer | `ChefZ_BlackPepper` | `\dz\gear\food\PowderedMilk.p3d` | ChefZ_SpiceBase |
| Käsebruch | `ChefZ_CheeseCurd` | `\dz\gear\food\Marmalade.p3d` | Marmalade |
| Getrocknete Beeren | `ChefZ_DriedBerries` | `\dz\gear\food\Marmalade.p3d` | Marmalade |
| Getrocknete Paprika | `ChefZ_DriedPaprika` | `\dz\gear\consumables\birch_bark.p3d` | ChefZ_DriedHerbBase |
| Getrocknete Petersilie | `ChefZ_DriedParsley` | `\dz\gear\consumables\birch_bark.p3d` | ChefZ_DriedHerbBase |
| Getrocknete Pfefferkörner | `ChefZ_DriedPeppercorns` | `\dz\gear\food\Rice.p3d` | ChefZ_DriedPeppercorns |
| Getrockneter Rosmarin | `ChefZ_DriedRosemary` | `\dz\gear\consumables\birch_bark.p3d` | ChefZ_DriedHerbBase |
| Getrockneter Thymian | `ChefZ_DriedThyme` | `\dz\gear\consumables\birch_bark.p3d` | ChefZ_DriedHerbBase |
| Getrockneter Bärlauch | `ChefZ_DriedWildGarlic` | `\dz\gear\consumables\birch_bark.p3d` | ChefZ_DriedHerbBase |
| Kräutermischung | `ChefZ_HerbMix` | `\dz\gear\food\PowderedMilk.p3d` | ChefZ_SpiceBase |
| Hunter Seasoning | `ChefZ_HunterSeasoning` | `\dz\gear\food\PowderedMilk.p3d` | ChefZ_SpiceBase |
| Pilzkultur | `ChefZ_MushroomCulture` | `\dz\gear\food\Rice.p3d` | ChefZ_MushroomCulture |
| Paprikapulver | `ChefZ_PaprikaPowder` | `\dz\gear\food\PowderedMilk.p3d` | ChefZ_SpiceBase |
| Rohsalz | `ChefZ_RawSalt` | `\dz\gear\consumables\garden_lime.p3d` | ChefZ_RawSalt |
| Salz | `ChefZ_Salt` | `\dz\gear\consumables\garden_lime.p3d` | ChefZ_Salt |

## ChefZ_Preservation (6)

| Name (DE) | Klasse | Vanilla-Modell | Modell gesetzt in |
|---|---|---|---|
| Trockenfisch | `ChefZ_DriedFish` | `\dz\gear\food\meat_steak.p3d` | ChefZ_DriedFish |
| Dörrfleisch | `ChefZ_DriedMeat` | `\dz\gear\food\meat_steak.p3d` | ChefZ_DriedMeat |
| Gesalzener Fisch | `ChefZ_SaltedFish` | `\dz\gear\food\meat_steak.p3d` | ChefZ_SaltedFish |
| Gepökeltes Fleisch | `ChefZ_SaltedMeat` | `\dz\gear\food\meat_steak.p3d` | ChefZ_SaltedMeat |
| Räucherfisch | `ChefZ_SmokedFish` | `\dz\gear\food\meat_steak.p3d` | ChefZ_SmokedFish |
| Geräuchertes Fleisch | `ChefZ_SmokedMeat` | `\dz\gear\food\meat_steak.p3d` | ChefZ_SmokedMeat |

## Nach Vanilla-Modell

| Vanilla-Modell | Anzahl Items |
|---|---|
| `\dz\gear\food\meat_steak.p3d` | 6 |
| `\dz\gear\consumables\birch_bark.p3d` | 5 |
| `\dz\gear\food\marmalade.p3d` | 4 |
| `\dz\gear\food\powderedmilk.p3d` | 4 |
| `\dz\gear\food\rice.p3d` | 3 |
| `\dz\gear\cultivation\plant_material.p3d` | 2 |
| `\dz\gear\consumables\garden_lime.p3d` | 2 |
| `\dz\gear\books\book_kniga.p3d` | 1 |
| `\dz\gear\food\food_can_open.p3d` | 1 |
| `\dz\gear\cooking\fryingpan.p3d` | 1 |
| `\dz\gear\camping\wooden_case.p3d` | 1 |
| `\dz\gear\food\sambucus_nigra.p3d` | 1 |
