# ChefZ - Audit Garstufen-Optik (Gebacken/Gekocht/Getrocknet/Verbrannt/Verfault), 27.10.2026

Erzeugt mit `py -3 tools/chefz-assets/stage_texture_audit.py --md <datei>` (liest alle
`ChefZ_Core/Addons/*/config.cpp` plus Vanilla `DZ/gear/food/config.cpp`, loest Vererbung auf,
prueft p3d/paa/rvmat in BEIDEN Baeumen, MLOD-Selektionen und model.cfg-sections).
178 essbare ChefZ-Klassen (scope 2, Edible_Base), davon 100 mit FoodStages.
Belege aus den Logs: `TestServer/profiles/client-130/DayZDiag_x64_2026-10-27_13-08-30.RPT`,
`TestServer/profiles/server-130/DayZDiag_x64_2026-10-27_13-07-45.RPT`.

## 1. Meerbarbe-Filet (ChefZ_RedMulletFillet) ohne 3D

* Klasse `ChefZ_Fishing/config.cpp:919`, `model = "\ChefZ\ChefZ_Sealife\fishfilet\models\redmulletfillet.p3d"`.
* Datei liegt in beiden Baeumen (byte-gleich, 155036 B) und im deployten `ChefZ_Sealife.pbo`
  (Praefix `ChefZ\ChefZ_Sealife`, Eintrag `fishfilet\models\redmulletfillet.p3d`).
* MLOD v257, LOD 0/1/2/3 + Shadow + Geometry + Memory + LandContact + View/Fire, Selektion `cs_raw`
  in allen Resolution-LODs, alle Punkte/Flaechen in `cs_raw`. Keine NaN, Normalen ok, UV 0..1,
  Groesse 9 x 9 x 20 cm. Strukturell identisch mit allen anderen 28 Filets (gleiches Hub-Template).
* Texturen RedMulletFillet_{co,baked,boiled,dried,burnt}: DXT1 1024, alle 9 Mips dekodierbar, kein
  Alpha, die UV-Inseln liegen auf Fleisch (per Dekodierung geprueft). In beiden Baeumen identisch.
* RPT: Client und Server laden das Modell ohne Warnung ("No model config used"), kein
  "Cannot load texture"/"Cannot open object" fuer ChefZ-Dateien.
* **Ergebnis: kein fehlendes oder kaputtes Asset, kein Pfad- oder Praefixfehler.** Nichts
  unterscheidet das Meerbarben-Filet von den Filets, die im selben Test sichtbar waren (der Server
  lud 13:07-13:40 BluefinTuna-, Char-, Conger-, JapaneseSardine-, RedSnapper- und SeaBassFillet;
  gemeldet wurde nur die Meerbarbe). Eine Ursache auf Datei- oder Config-Ebene ist nicht
  nachweisbar. Allen Hub-Filets gemeinsam: der Memory-Punkt `invView` liegt exakt auf `ce_center`
  (Hub-Meldung "Memory Points aus dem Template angelegt ... Positionen bitte pruefen"). Falls die
  Inventar-/Vorschaukamera `invView` nutzt, ist die Blickrichtung entartet - das traefe aber alle
  Filets. Naechster Schritt in-game: Meerbarben- und Wolfsbarsch-Filet per COT nebeneinander in die
  Welt UND ins Inventar legen und vergleichen; erst danach am Mesh ansetzen.
* Die ZIPs in `E:\DayZ Modding\ChefZ_Fillet_Referenzbilder` werden nicht gebraucht: es fehlt keine p3d.

## 2. Raeucherwurst (ChefZ_SmokedSausage) ohne 3D

* Klasse `ChefZ_Preservation/config.cpp:943`, `model = "\ChefZ\ChefZ_Food\models\sausage_smoked.p3d"`,
  `itemSize {2,1}` (daher der 1x2-Platz).
* p3d in beiden Baeumen und in der deployten PBO, MLOD, Selektion `camo` (alle 872 Punkte und 1258
  Flaechen), Textur im Modell `chefz\chefz_food\data\sausage_smoked_co.paa` (vorhanden, DXT1 512, Mips ok).
* Die model.cfg-Aenderung ist syntaktisch und inhaltlich in Ordnung: `class Default` ist in derselben
  Datei definiert, kein Skelett, Klassenname `sausage_smoked` = Dateiname, `camo` existiert im Mesh.
  RPT ab 12:57: "Model config used: model.cfg", ohne Warnung. Dieselbe Form tragen seit 12:57 auch
  RawSausage, RawVenisonSausage, Beef-/Pork-/VenisonLeg und MincedBoar - die lagen im Test
  13:08-13:40 in der Welt bzw. wurden zur Raeucherwurst verarbeitet und wurden NICHT als unsichtbar
  gemeldet. Die Kombination model.cfg + hiddenSelectionsMaterials ist damit nicht die Ursache.
* **Ergebnis: auch hier kein Datei-, Pfad- oder model.cfg-Fehler.** Das Mesh ist gleich gebaut wie
  sausage_raw/cooked/dry (gleiche LOD-Liste, gleiche Properties, Mass 100). Einziger Unterschied im
  Laufzeitweg: die Raeucherwurst entsteht mit Zustand SMOKED und wird seit heute sofort auf die
  Vanilla-Stufe projiziert (`ChefZ_ItemStateComponent.c`, neuer Block "Die Projektion laeuft
  trotzdem"); Dried = visual {0,3,3} = `sausage_smoked_co.paa` + `meat_steak_dried.rvmat`.
  In-game pruefen: Raeucherwurst per COT spawnen (Stufe Raw) und eine aus dem Raeucherofen
  vergleichen - ist nur die geraeucherte unsichtbar, liegt es an Zustand/Stufe, nicht am Asset.

## 3. Garstufen-Optik fehlt "fast vollstaendig" - Ursache gefunden

**Bei den MLOD-Modellen wirken hiddenSelectionsTextures/-Materials nur, wenn eine model.cfg die
Selektion als section fuehrt.** Das RPT zeigt es direkt: jedes Filet, jede Krabbe und jeder
Tintenfisch wurde mit "No model config used" geladen; ChefZ_Sealife hatte keine einzige model.cfg.

| Gruppe | Klassen | Zustand vor heute | Zustand jetzt |
|---|---|---|---|
| Filets (Sealife/fishfilet) | 29 | 5 eigene Stufen-Texturen + 6 Materialien, Indizes korrekt, aber **keine model.cfg** -> wirkungslos | model.cfg angelegt (beide Baeume) |
| Krabben/Hummer/Garnele (crab) | 9 | 5 eigene Texturen, Material default.rvmat, **keine model.cfg** | model.cfg angelegt |
| Tintenfisch/Krake (squid) | 4 (+4 Varianten-p3d) | wie Krabben | model.cfg angelegt |
| Fleisch/Wurst ChefZ_Meat (Food-Modelle) | 22 | Texturen vorhanden, model.cfg-Eintraege erst seit 12:57 | ok ab Build 12:57 |
| Preservation auf eigenem Wurst-Mesh | 2 | wie Meat | ok |
| Preservation auf Vanilla `meat_steak.p3d` | 6 | Vanilla-Steaktexturen, ODOL - wirkte schon immer | ok (Fisch traegt Steak-Optik) |
| Backwaren, Gemuese/Obst, Ei, Kaese, Butter, Saucen, Mehl | 22 | **keine** hiddenSelections/Texturen | siehe "Needs textures" |
| Gerichte (ServedDish), ganze Fische, Kraeuter | 56 | keine FoodStages - keine Stufen vorgesehen | unveraendert |

Damit hatte vor heute praktisch nur die Vanilla-Steak-Gruppe sichtbare Stufen - genau das
"fast vollstaendig" aus dem Bericht.

Weitere Befunde (nicht geaendert):
* `ChefZ_Butter : Lard` erbt Lards `hiddenSelections {"cs_raw","cs_dried"}` + Lard-Texturen; das
  Butter-Mesh hat nur `camo` -> wirkungslos (sonst saehe Butter wie Schmalz aus).
* `ChefZ_Cheese : BoxCerealCrunchin` erbt `camoGround` + Muesli-Kartontextur; das Mesh hat nur `camo`.
* `ChefZ_RawPasta`, `ChefZ_BoneBroth`, `ChefZ_Cheese`: hiddenSelections ohne hiddenSelectionsMaterials
  bei vorhandenen FoodStages -> `FoodStage.UpdateVisualsEx` ruft `config_materials.Get(0)` auf einem
  leeren Array (FoodStage.c:598).
* ChefZ_Meat-Klassen haben keine FoodStage `Dried` - die vorhandenen `*_dried_co.paa` (leg_*,
  dicedmeat, sausage_raw_*) werden nie gezeigt. Laut Kommentar Absicht (Trocknen nur ueber die
  Preservation-Klassen), daher nicht geaendert.
* Krabben/Tintenfische: alle Stufen nutzen Material 0 (`dz\data\data\default.rvmat`); Rotten zeigt
  deshalb die Rohtextur ohne Faeulnis-Overlay.
* Die ganzen Fische (31 Klassen, `camo`, eine Textur, keine Stufen) haben weiterhin keine model.cfg -
  ohne Stufen folgenlos.
* Im Lieferbaum `ChefZ/ChefZ_Food` fehlen 19 Gericht-Texturen und 10 Gericht-p3d, die nur in
  `ChefZ_Core/Addons/ChefZ_Food` liegen (Gerichte ohne Stufen; das Packen ist nicht betroffen,
  sync-assets loescht nichts).
* `ChefZ_Flour` (ChefZ_Processing) hat Stufen ohne Texturen - die Processing-Config war tabu.

## Geaenderte Dateien

* NEU `ChefZ_Core/Addons/ChefZ_Sealife/fishfilet/models/model.cfg` und `ChefZ/ChefZ_Sealife/fishfilet/models/model.cfg` (29 Klassen, section `cs_raw`)
* NEU `ChefZ_Core/Addons/ChefZ_Sealife/crab/models/model.cfg` und `ChefZ/ChefZ_Sealife/crab/models/model.cfg` (9 Klassen, `camo`)
* NEU `ChefZ_Core/Addons/ChefZ_Sealife/squid/models/model.cfg` und `ChefZ/ChefZ_Sealife/squid/models/model.cfg` (8 Klassen, `camo`)
* NEU `tools/chefz-assets/stage_texture_audit.py` (dieses Audit, wiederholbar)
* `ChefZ_Sealife/include.txt` nimmt `*.cfg` bereits mit.

Nach dem naechsten Build im Client-RPT pruefen: auf `Loading LODShape '...fishfilet\models\codfillet.p3d'`
muss "Model config used: model.cfg" folgen. Dann ein Filet braten: die Textur muss wechseln.

## Needs textures (19 Klassen)

Je Klasse fehlen die Stufen-Texturen; die Basis-Textur liegt in beiden Baeumen. Dateinamen nach dem
bestehenden Schema `<basis>_baked_co.paa`, `_boiled_co.paa`, `_dried_co.paa`, `_burnt_co.paa`
(gleiche UV wie die Basis, also die Basis-Textur als Vorlage umfaerben). Nach der Generierung je
Klasse noetig: `hiddenSelections[] = {"camo"}`, 5 Texturen (raw, baked, boiled, dried, burnt),
6 Materialien (z. B. 6x `dz\data\data\default.rvmat` wie bei den Krabben, oder Vanilla-Analoga),
visual_properties Baked {0,1,1} Boiled {0,2,2} Dried {0,3,3} Burned {0,4,4} Rotten {0,0,5}, UND ein
model.cfg-Eintrag `class <p3d-name>:Default { sections[]={"camo"}; };` im Modellordner (beide Baeume).

| # | Klasse | Addon (config) | Basis-Textur | p3d (Selektion camo) | Stufen der Klasse -> benoetigte Texturen | model.cfg |
|---|---|---|---|---|---|---|
| 1 | ChefZ_Bread | ChefZ_Baking | ChefZ\ChefZ_Food\data\bread_co.paa | ChefZ_Food\models\bread.p3d | Baked, Burned | Food: Klasse `bread` fehlt |
| 2 | ChefZ_Dough | ChefZ_Baking | ChefZ\ChefZ_Food\data\dough_co.paa | ChefZ_Food\models\dough.p3d | Baked, Boiled, Burned | `dough` fehlt |
| 3 | ChefZ_DriedPasta | ChefZ_Baking | ChefZ\ChefZ_Food\data\dried_pasta_co.paa | ChefZ_Food\models\dried_pasta.p3d | Baked, Burned | `dried_pasta` fehlt |
| 4 | ChefZ_Flatbread | ChefZ_Baking | ChefZ\ChefZ_Food\data\flatbread_co.paa | ChefZ_Food\models\flatbread.p3d | Baked, Burned | `flatbread` fehlt |
| 5 | ChefZ_RawPasta | ChefZ_Baking | ChefZ\ChefZ_Food\data\lasagne_pasta_co.paa | ChefZ_Food\models\lasagne_pasta.p3d | Baked, Burned (+ Materialien fehlen) | `lasagne_pasta` fehlt |
| 6 | ChefZ_BoneBroth | ChefZ_Cooking | ChefZ\ChefZ_Food\data\bone_broth_co.paa | ChefZ_Food\models\bone_broth.p3d | Baked, Boiled, Burned (+ Materialien fehlen) | `bone_broth` fehlt |
| 7 | ChefZ_TomatoSauce | ChefZ_Cooking | ChefZ\ChefZ_Items\data\tomato_sauce_co.paa | ChefZ_Items\models\tomato_sauce.p3d | Baked, Boiled, Burned | ChefZ_Items hat KEINE model.cfg |
| 8 | ChefZ_Blackberry | ChefZ_Farming | ChefZ\ChefZ_Plants\data\blackberry_co.paa | ChefZ_Plants\models\blackberry.p3d | Baked, Boiled, Burned | Plants: Klasse fehlt |
| 9 | ChefZ_Blueberry | ChefZ_Farming | ChefZ\ChefZ_Plants\data\blueberry_co.paa | ChefZ_Plants\models\blueberry.p3d | Baked, Boiled, Burned | fehlt |
| 10 | ChefZ_Cabbage | ChefZ_Farming | ChefZ\ChefZ_Plants\data\cabbage_co.paa | ChefZ_Plants\models\cabbage.p3d | Baked, Boiled, Burned | fehlt |
| 11 | ChefZ_Carrot | ChefZ_Farming | ChefZ\ChefZ_Plants\data\carrot_co.paa | ChefZ_Plants\models\carrot.p3d | Baked, Boiled, Burned | fehlt |
| 12 | ChefZ_Corn | ChefZ_Farming | ChefZ\ChefZ_Plants\data\corn_cob_co.paa | ChefZ_Plants\models\corn_cob.p3d | Baked, Boiled, Burned | fehlt |
| 13 | ChefZ_Garlic | ChefZ_Farming | ChefZ\ChefZ_Plants\data\garlic_co.paa | ChefZ_Plants\models\garlic.p3d | Baked, Boiled, Burned | fehlt |
| 14 | ChefZ_Onion | ChefZ_Farming | ChefZ\ChefZ_Plants\data\redonion_co.paa | ChefZ_Plants\models\redonion.p3d | Baked, Boiled, Burned | fehlt |
| 15 | ChefZ_Raspberry | ChefZ_Farming | ChefZ\ChefZ_Plants\data\raspberry_co.paa | ChefZ_Plants\models\raspberry.p3d | Baked, Boiled, Burned | fehlt |
| 16 | ChefZ_Strawberry | ChefZ_Farming | ChefZ\ChefZ_Plants\data\strawberry_co.paa | ChefZ_Plants\models\strawberry.p3d | Baked, Boiled, Burned | fehlt |
| 17 | ChefZ_Butter | ChefZ_Ingredients | ChefZ\ChefZ_Food\data\butter_co.paa | ChefZ_Food\models\butter.p3d | Baked, Boiled, Burned (Lard-Erbe ueberschreiben: hiddenSelections camo) | `butter` fehlt |
| 18 | ChefZ_Cheese | ChefZ_Ingredients | ChefZ\ChefZ_Food\data\cheese_co.paa | ChefZ_Food\models\cheese.p3d | Baked, Boiled, Burned (camoGround-Erbe ueberschreiben, Materialien fehlen) | `cheese` fehlt |
| 19 | ChefZ_Egg | ChefZ_Ingredients | ChefZ\ChefZ_Food\data\egg_co.paa | ChefZ_Food\models\egg.p3d | Baked, Boiled, Burned | `egg` fehlt |

Hinweis: die Plants-model.cfg existiert nur in `ChefZ_Core/Addons/ChefZ_Plants/models/` (nicht im
Lieferbaum) - beim Ergaenzen in beide Baeume legen. "Dried" steht bei diesen Klassen nicht in den
FoodStages; die _dried_co-Textur ist daher optional (Index 3 darf auf die Basis zeigen).
Nicht aufgenommen (Vanilla-Behaeltermodell, keine sichtbare Speise): CreamSauce, MushroomCreamSauce,
CheeseCurd, DriedBerries (Marmalade.p3d), PaprikaPowder (PowderedMilk.p3d), Wheat (Rice.p3d);
ChefZ_Flour (Processing-Config tabu).

## Volle Tabelle

Spalten: Stufen = visual_properties {Selektion,Textur,Material} in der Reihenfolge Raw Baked Boiled
Dried Burned Rotten ("-" = Stufe fehlt). Dateien = model/Texturen/Materialien in Core und ChefZ/.
section = ob die Selektion als model.cfg-section gilt (Stand NACH den neuen model.cfg). Eigene
Stufen-Optik = Stufen, deren Textur sich inhaltlich von der Rohtextur unterscheidet.

| Klasse | Addon | Modell | Sel. | Stufen R/Ba/Bo/Dr/Bu/Ro (visual_properties) | Tex/Mat | Dateien | section | eigene Stufen-Optik | Probleme |
|---|---|---|---|---|---|---|---|---|---|
| ChefZ_Bread | Baking | ChefZ_Food/models/bread.p3d | - | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Dough | Baking | ChefZ_Food/models/dough.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedPasta | Baking | ChefZ_Food/models/dried_pasta.p3d | - | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Flatbread | Baking | ChefZ_Food/models/flatbread.p3d | - | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_RawPasta | Baking | ChefZ_Food/models/lasagne_pasta.p3d | camo | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 1/0 | ok | model.cfg ohne Klasse | keine | hiddenSelectionsMaterials leer -> FoodStage.UpdateVisualsEx Get(0) auf leerem Array; model.cfg ohne Klasse 'lasagne_pasta' -> keine section |
| ChefZ_BeanSausagePlate | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\beansausageplate_co.paa |
| ChefZ_BloodSausagePlate | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\bloodsausageplate_co.paa |
| ChefZ_BoneBroth | Cooking | ChefZ_Food/models/bone_broth.p3d | camo | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 1/0 | ok | model.cfg ohne Klasse | keine | hiddenSelectionsMaterials leer -> FoodStage.UpdateVisualsEx Get(0) auf leerem Array; model.cfg ohne Klasse 'bone_broth' -> keine section |
| ChefZ_BoneBrothSoupBowl | Cooking | ChefZ_Food/models/soup_bonebroth.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'soup_bonebroth' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_CheeseFlatbread | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\cheeseflatbread_co.paa |
| ChefZ_ChernarusChiliBowl | Cooking | ChefZ_Food/models/soup_chernaruschili.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'soup_chernaruschili' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_CreamMushroomPasta | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\creammushroompasta_co.paa |
| ChefZ_CreamSauce | Cooking | /dz/gear/food/Marmalade.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_CrispyFriedCalamari | Cooking | ChefZ_Food/models/crispy_fried_calamari.p3d | camo | keine FoodStages | 1/0 | ok | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'crispy_fried_calamari' -> keine section |
| ChefZ_FarmersBreakfast | Cooking | ChefZ_Food/models/farmersbreakfast.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'farmersbreakfast' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_FishPotatoPlate | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\fishpotatoplate_co.paa |
| ChefZ_FishermanStewBowl | Cooking | ChefZ_Food/models/stew_fisherman.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'stew_fisherman' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_GrilledStuffedCalamari | Cooking | ChefZ_Food/models/grilled_stuffed_calamari.p3d | camo | keine FoodStages | 1/0 | ok | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'grilled_stuffed_calamari' -> keine section |
| ChefZ_HoneyBreadPlate | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\honeybreadplate_co.paa |
| ChefZ_HunterPasta | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\hunterpasta_co.paa |
| ChefZ_HunterPlate | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\hunterplate_co.paa |
| ChefZ_HunterStewBowl | Cooking | ChefZ_Food/models/stew_hunter.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'stew_hunter' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_MacAndCheese | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\macandcheese_co.paa |
| ChefZ_MeatDumplings | Cooking | ChefZ_Food/models/meatdumplings.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'meatdumplings' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_MilkRice | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\milkrice_co.paa |
| ChefZ_MushroomCreamSauce | Cooking | /dz/gear/food/Marmalade.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_MushroomPan | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\mushroompan_co.paa |
| ChefZ_PotatoPancakes | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\potatopancakes_co.paa |
| ChefZ_PumpkinSoupBowl | Cooking | ChefZ_Food/models/soup.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'soup' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_SausageBreadPlate | Cooking | ChefZ_Food/models/sausage_breadplate.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'sausage_breadplate' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_SausagePasta | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\sausagepasta_co.paa |
| ChefZ_SausagePotatoes | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\sausagepotatoes_co.paa |
| ChefZ_ScrambledEggSausage | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\scrambledeggsausage_co.paa |
| ChefZ_ShellfishBisqueBowl | Cooking | ChefZ_Food/models/soup.p3d | camo | keine FoodStages | 1/0 | 2 fehlen ChefZ/ | model.cfg ohne Klasse | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\shellfish_bisque_co.paa; model.cfg ohne Klasse 'soup' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_SmallFishPan | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\smallfishpan_co.paa |
| ChefZ_SurvivorSpaghetti | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\survivorspaghetti_co.paa |
| ChefZ_TacticalBreakfast | Cooking | ChefZ_Food/models/Panfood_Base.p3d | camo | keine FoodStages | 1/0 | 1 fehlen ChefZ/ | ok | keine | Textur fehlt in ChefZ/: ChefZ\ChefZ_Food\data\tacticalbreakfast_co.paa |
| ChefZ_TomatoSauce | Cooking | ChefZ_Items/models/tomato_sauce.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_VegetableSoupBowl | Cooking | ChefZ_Food/models/soup_vegetables.p3d | camo | keine FoodStages | 0/0 | 1 fehlen ChefZ/ | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'soup_vegetables' -> keine section; p3d fehlt in ChefZ/ (Lieferbaum) |
| ChefZ_Blackberry | Farming | ChefZ_Plants/models/blackberry.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Blueberry | Farming | ChefZ_Plants/models/blueberry.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Cabbage | Farming | ChefZ_Plants/models/cabbage.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Carrot | Farming | ChefZ_Plants/models/carrot.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Chili | Farming | ChefZ_Plants/models/chili.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Corn | Farming | ChefZ_Plants/models/corn_cob.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Garlic | Farming | ChefZ_Plants/models/garlic.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Onion | Farming | ChefZ_Plants/models/redonion.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Parsley | Farming | ChefZ_Plants/models/parsley.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_PepperBerries | Farming | /dz/gear/food/Sambucus_nigra.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Raspberry | Farming | ChefZ_Plants/models/raspberry.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Rosemary | Farming | ChefZ_Plants/models/rosmary.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Strawberry | Farming | ChefZ_Plants/models/strawberry.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Thyme | Farming | /dz/gear/cultivation/plant_material.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Wheat | Farming | /dz/gear/food/Rice.p3d | - | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_WildGarlic | Farming | ChefZ_Food/models/wild_garlic.p3d | camo | keine FoodStages | 1/0 | ok | model.cfg ohne Klasse | keine | model.cfg ohne Klasse 'wild_garlic' -> keine section |
| ChefZ_BlueMarlin | Fishing | ChefZ_Sealife/fish/models/bluemarlin.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_BlueMarlinFillet | Fishing | ChefZ_Sealife/fishfilet/models/bluemarlinfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_BlueSentinelCrab | Fishing | ChefZ_Sealife/crab/models/bluesentinelcrab.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_BluefinTuna | Fishing | ChefZ_Sealife/fish/models/bluefintuna.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_BluefinTunaFillet | Fishing | ChefZ_Sealife/fishfilet/models/tunafillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Bonito | Fishing | ChefZ_Sealife/fish/models/bonito.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_BonitoFillet | Fishing | ChefZ_Sealife/fishfilet/models/bonitofillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Char | Fishing | ChefZ_Sealife/fish/models/char.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_CharFillet | Fishing | ChefZ_Sealife/fishfilet/models/charfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_CoastalCrab | Fishing | ChefZ_Sealife/crab/models/coastalcrab.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Cod | Fishing | ChefZ_Sealife/fish/models/cod.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_CodFillet | Fishing | ChefZ_Sealife/fishfilet/models/codfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Conger | Fishing | ChefZ_Sealife/fish/models/conger.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_CongerFillet | Fishing | ChefZ_Sealife/fishfilet/models/congerfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_CrimsonKingCrab | Fishing | ChefZ_Sealife/crab/models/crimsonkingcrab.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_CrimsonSquid | Fishing | ChefZ_Sealife/squid/models/crimsonsquid.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Eel | Fishing | ChefZ_Sealife/fish/models/eel.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_EelFillet | Fishing | ChefZ_Sealife/fishfilet/models/eelfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_GiantGrouper | Fishing | ChefZ_Sealife/fish/models/giantgrouper.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_GiantGrouperFillet | Fishing | ChefZ_Sealife/fishfilet/models/grouperfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_GiantTrevally | Fishing | ChefZ_Sealife/fish/models/gianttrevally.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_GiantTrevallyFillet | Fishing | ChefZ_Sealife/fishfilet/models/gianttrevallyfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_GiltheadBream | Fishing | ChefZ_Sealife/fish/models/giltheadbream.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_GiltheadBreamFillet | Fishing | ChefZ_Sealife/fishfilet/models/giltheadbreamfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Halibut | Fishing | ChefZ_Sealife/fish/models/halibut.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_HalibutFillet | Fishing | ChefZ_Sealife/fishfilet/models/halibutfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_HorseMackerel | Fishing | ChefZ_Sealife/fish/models/horsemackerel.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_HorseMackerelFillet | Fishing | ChefZ_Sealife/fishfilet/models/horsemackerelfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_JapaneseSardine | Fishing | ChefZ_Sealife/fish/models/japanesesardine.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_JapaneseSardineFillet | Fishing | ChefZ_Sealife/fishfilet/models/japanesesardinefillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Langoustine | Fishing | ChefZ_Sealife/crab/models/langoustine.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_MahiMahi | Fishing | ChefZ_Sealife/fish/models/mahimahi.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_MahiMahiFillet | Fishing | ChefZ_Sealife/fishfilet/models/mahimahifillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_MidnightLobster | Fishing | ChefZ_Sealife/crab/models/midnightlobster.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Muskellunge | Fishing | ChefZ_Sealife/fish/models/muskellunge.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_MuskellungeFillet | Fishing | ChefZ_Sealife/fishfilet/models/muskellungefillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_NorthernCrimsonSquid | Fishing | ChefZ_Sealife/squid/models/northerncrimsonsquid.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Octopus | Fishing | ChefZ_Sealife/squid/models/octopus.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Perch | Fishing | ChefZ_Sealife/fish/models/perch.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_PerchFillet | Fishing | ChefZ_Sealife/fishfilet/models/perchfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Plaice | Fishing | ChefZ_Sealife/fish/models/plaice.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_PlaiceFillet | Fishing | ChefZ_Sealife/fishfilet/models/plaicefillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_RainbowTrout | Fishing | ChefZ_Sealife/fish/models/rainbowtrout.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_RainbowTroutFillet | Fishing | ChefZ_Sealife/fishfilet/models/rainbowtroutfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_RedMullet | Fishing | ChefZ_Sealife/fish/models/redmullet.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_RedMulletFillet | Fishing | ChefZ_Sealife/fishfilet/models/redmulletfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_RedSnapper | Fishing | ChefZ_Sealife/fish/models/redsnapper.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_RedSnapperFillet | Fishing | ChefZ_Sealife/fishfilet/models/redsnapperfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Rockfish | Fishing | ChefZ_Sealife/fish/models/rockfish.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_RockfishFillet | Fishing | ChefZ_Sealife/fishfilet/models/rockfishfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Sailfish | Fishing | ChefZ_Sealife/fish/models/sailfish.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SailfishFillet | Fishing | ChefZ_Sealife/fishfilet/models/sailfishfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Salmon | Fishing | ChefZ_Sealife/fish/models/salmon.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SalmonFillet | Fishing | ChefZ_Sealife/fishfilet/models/salmonfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_SeaBass | Fishing | ChefZ_Sealife/fish/models/seabass.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SeaBassFillet | Fishing | ChefZ_Sealife/fishfilet/models/seabassfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_SeaEel | Fishing | ChefZ_Sealife/fish/models/seaeel.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_Snakehead | Fishing | ChefZ_Sealife/fish/models/snakehead.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SnakeheadFillet | Fishing | ChefZ_Sealife/fishfilet/models/snakeheadfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_SnowCrab | Fishing | ChefZ_Sealife/crab/models/snowcrab.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_SockeyeSalmon | Fishing | ChefZ_Sealife/fish/models/sockeyesalmon.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SockeyeSalmonFillet | Fishing | ChefZ_Sealife/fishfilet/models/sockeyesalmonfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Sole | Fishing | ChefZ_Sealife/fish/models/sole.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_SoleFillet | Fishing | ChefZ_Sealife/fishfilet/models/solefillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_SpeckledSquid | Fishing | ChefZ_Sealife/squid/models/speckledsquid.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_SpinyLobster | Fishing | ChefZ_Sealife/crab/models/spinylobster.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_SpinyOrangeCrab | Fishing | ChefZ_Sealife/crab/models/spinyorangecrab.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Squid | Fishing | ChefZ_Sealife/squid/models/squid.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_StripedTigerPrawn | Fishing | ChefZ_Sealife/crab/models/stripedtigerprawn.p3d | camo | {0,0,0} {0,1,0} {0,2,0} {0,3,0} {0,4,0} {0,0,0} | 5/1 | ok | ok | Baked,Boiled,Dried,Burned | - |
| ChefZ_Tarpon | Fishing | ChefZ_Sealife/fish/models/tarpon.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_TarponFillet | Fishing | ChefZ_Sealife/fishfilet/models/tarponfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_YellowfinTuna | Fishing | ChefZ_Sealife/fish/models/yellowfintuna.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_YellowfinTunaFillet | Fishing | ChefZ_Sealife/fishfilet/models/tunafillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_Yellowtail | Fishing | ChefZ_Sealife/fish/models/yellowtail.p3d | camo | keine FoodStages | 1/0 | ok | KEINE model.cfg | keine | keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht |
| ChefZ_YellowtailFillet | Fishing | ChefZ_Sealife/fishfilet/models/yellowtailfillet.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,4,5} | 5/6 | ok | ok | Baked,Boiled,Dried,Burned,Rotten | - |
| ChefZ_BlackPepper | Ingredients | /dz/gear/food/PowderedMilk.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Butter | Ingredients | ChefZ_Food/models/butter.p3d | cs_raw,cs_dried | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {-1,-1,5} | 5/6 | ok | model.cfg ohne Klasse | Baked,Boiled,Burned | Selektion 'cs_dried' nicht im p3d (LOD0: camo); Selektion 'cs_raw' nicht im p3d (LOD0: camo); model.cfg ohne Klasse 'butter' -> keine section |
| ChefZ_Cheese | Ingredients | ChefZ_Food/models/cheese.p3d | camoGround | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 1/0 | ok | model.cfg ohne Klasse | keine | Selektion 'camoGround' nicht im p3d (LOD0: camo); hiddenSelectionsMaterials leer -> FoodStage.UpdateVisualsEx Get(0) auf leerem Array; model.cfg ohne Klasse 'cheese' -> keine section |
| ChefZ_CheeseCurd | Ingredients | /dz/gear/food/Marmalade.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Cream | Ingredients | ChefZ_Items/models/cream.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedBerries | Ingredients | /dz/gear/food/Marmalade.p3d | - | {0,0,0} {0,0,0} {0,0,0} {0,0,0} {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedPaprika | Ingredients | /dz/gear/consumables/birch_bark.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedParsley | Ingredients | /dz/gear/consumables/birch_bark.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedPeppercorns | Ingredients | /dz/gear/food/Rice.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedRosemary | Ingredients | /dz/gear/consumables/birch_bark.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedThyme | Ingredients | /dz/gear/consumables/birch_bark.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_DriedWildGarlic | Ingredients | /dz/gear/consumables/birch_bark.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_Egg | Ingredients | ChefZ_Food/models/egg.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_HerbMix | Ingredients | /dz/gear/food/PowderedMilk.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_HunterSeasoning | Ingredients | /dz/gear/food/PowderedMilk.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_MushroomCulture | Ingredients | /dz/gear/food/Rice.p3d | - | keine FoodStages | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_PaprikaPowder | Ingredients | /dz/gear/food/PowderedMilk.p3d | - | {0,0,0} {0,0,0} {0,0,0} - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |
| ChefZ_BeefLeg | Meat | ChefZ_Food/models/leg_beef.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_BoarSausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_CookedSausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_DicedMeat | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_HunterSausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedBear | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedBoar | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedChicken | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedMeat | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedPork | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_MincedVenison | Meat | ChefZ_Food/models/dicedmeat.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_PorkLeg | Meat | ChefZ_Food/models/leg_pork.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_PorkSausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawBoarSausage | Meat | ChefZ_Food/models/sausage_raw_boar.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawHunterSausage | Meat | ChefZ_Food/models/sausage_raw_hunter.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawPorkSausage | Meat | ChefZ_Food/models/sausage_raw_pork.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawSausage | Meat | ChefZ_Food/models/sausage_raw.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawSpicySausage | Meat | ChefZ_Food/models/sausage_raw_spicy.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_RawVenisonSausage | Meat | ChefZ_Food/models/sausage_raw_venison.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_SpicySausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_VenisonLeg | Meat | ChefZ_Food/models/leg_venison.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_VenisonSausage | Meat | ChefZ_Food/models/sausage_cooked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} - {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_DriedFish | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Burned | - |
| ChefZ_DriedMeat | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Burned | - |
| ChefZ_DrySausage | Preservation | ChefZ_Food/models/sausage_dry.p3d | camo | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_SaltedFish | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Dried,Burned | - |
| ChefZ_SaltedMeat | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Dried,Burned | - |
| ChefZ_SmokedFish | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Burned | - |
| ChefZ_SmokedMeat | Preservation | /dz/gear/food/meat_steak.p3d | cs_raw | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | vanilla ODOL | Baked,Boiled,Burned | - |
| ChefZ_SmokedSausage | Preservation | ChefZ_Food/models/sausage_smoked.p3d | camo | {0,0,0} {0,1,1} {0,2,2} {0,3,3} {0,4,4} {0,0,5} | 5/6 | ok | ok | Baked,Boiled,Burned | - |
| ChefZ_Flour | Processing | ChefZ_Items/models/flour.p3d | - | {0,0,0} {0,0,0} - - {0,0,0} {0,0,0} | 0/0 | ok | keine hiddenSelections | keine | - |

## Stand nach Umsetzung (27.10.2026, nachmittags)

Erneuter Lauf: `py -3 tools/chefz-assets/stage_texture_audit.py` (178 Klassen, 100 mit FoodStages,
Exit 0). **Alle 19 Klassen aus "Needs textures" haben jetzt eigene Stufen-Optik**: hiddenSelections
`{"camo"}` (im MLOD vorhanden, in der model.cfg als section gefuehrt), 5 Texturen, 6 Materialien,
visual_properties Raw {0,0,0} Baked {0,1,1} Boiled {0,2,2} Dried {0,3,3} Burned {0,4,4} Rotten {0,0,5}
fuer die Stufen, die die Klasse hat. Audit-Probleme dieser Klassen: 0.

| Klasse(n) | Texturen (neu, beide Baeume) | Materialien (Vanilla, aus DZ/gear/food/config.cpp) | eigene Optik |
|---|---|---|---|
| Bread, Dough, DriedPasta, Flatbread, RawPasta | ChefZ_Food/data/{bread,dough,dried_pasta,flatbread,lasagne_pasta}_{baked,boiled,dried,burnt}_co.paa | potato_prepared/baked/boiled/dried/burnt + meat_steak_rotten | Baked, (Boiled), Burned |
| BoneBroth, TomatoSauce | ChefZ_Food/data/bone_broth_*, ChefZ_Items/data/tomato_sauce_* | tomato_raw..rotten | Baked, Boiled, Burned |
| Blackberry, Blueberry, Raspberry, Strawberry | ChefZ_Plants/data/<beere>_* | sambucus_nigra_raw..rotten | Baked, Boiled, Burned |
| Cabbage | ChefZ_Plants/data/cabbage_* | pepper_green_raw..rotten | Baked, Boiled, Burned |
| Carrot, Corn, Garlic, Onion | ChefZ_Plants/data/{carrot,corn_cob,garlic,redonion}_* | zucchini_raw..rotten | Baked, Boiled, Burned |
| Butter | ChefZ_Food/data/butter_* | lard_raw..burnt + meat_steak_rotten | Baked, Boiled, Burned |
| Cheese | ChefZ_Food/data/cheese_* | potato_* + meat_steak_rotten | Baked, Boiled, Burned |
| Egg | ChefZ_Food/data/egg_* | apple_raw..rotten | Baked, Boiled, Burned |

Texturen: Baked/Dried/Burned per Higgsfield `flux_3_image` (Basistextur als Referenz, gleiches
UV-Layout), Boiled als PIL-Toenung der Basis (blasser, grauer). Alle auf die Groesse der Basis
skaliert (256/512/1024), ImageToPAA. Dried-Texturen liegen auch fuer Klassen ohne Dried-Stufe bei
(Index 3), damit die Arrays einheitlich sind.

model.cfg (beide Baeume): `ChefZ_Food/models/model.cfg` +9 Klassen (bread, dough, dried_pasta,
flatbread, lasagne_pasta, bone_broth, butter, cheese, egg); `ChefZ_Plants/models/model.cfg` +9
Klassen (blackberry, blueberry, cabbage, carrot, corn_cob, garlic, redonion, raspberry, strawberry),
im Lieferbaum NEU (Kopie der Core-Datei); `ChefZ_Items/models/model.cfg` NEU in beiden Baeumen
(tomato_sauce).

Die "Weiteren Befunde" oben sind erledigt:
* Butter: eigene hiddenSelections `camo` statt Lards cs_raw/cs_dried; Rotten {0,0,5} statt {-1,-1,5}.
* Cheese: `camo` statt geerbtem camoGround; Materialien gesetzt.
* RawPasta, BoneBroth, Cheese: 6 Materialien - kein Get() mehr auf leerem Array (FoodStage.c:598).
* ChefZ_Meat: alle 22 Fleisch-/Wurstklassen und ChefZ_MeatItemBase haben jetzt `class Dried`
  (visual {0,3,3}, cooking {70,120,80} wie Vanillas Steak-Dried, Naehrwerte aus Raw/Baked abgeleitet).
  KEIN Drying-Uebergang ergaenzt: Vanillas Steaks haben ihn (MeatStageTransitions), ChefZ_Meat
  laesst ihn laut Kopfkommentar absichtlich weg (Trocknen nur an ChefZ-Stationen); die bestehenden
  Uebergaenge Raw->Baked/Boiled sind unveraendert. Die gekochten Wuerste (6 Klassen auf
  sausage_cooked.p3d) hatten an Index 3 die Rohtextur - neu `sausage_cooked_dried_co.paa`.
* Krabben/Tintenfische (ChefZ_Seafood_Base): 6 Materialien (5x default.rvmat +
  meat_steak_rotten.rvmat), Rotten {0,0,5} - Faeulnis-Overlay statt Rohoptik.

Was bleibt (bewusst nicht geaendert):
* Rotten zeigt ueberall die Rohtextur ("wie roh") - so gewollt, die Faeulnis kommt ueber Material 5
  wie bei Vanillas Steak.
* Preservation-Produkte (DriedFish/-Meat, SmokedFish/-Meat/-Sausage, DrySausage): Dried = Rohtextur,
  weil "roh" dort schon das getrocknete/geraeucherte Produkt ist.
* Vanilla-Behaeltermodelle ohne sichtbare Speise: CreamSauce, MushroomCreamSauce, CheeseCurd,
  DriedBerries (Marmalade.p3d), PaprikaPowder (PowderedMilk.p3d), Wheat (Rice.p3d) - keine Texturen.
* ChefZ_Flour: Stufen ohne Texturen; ChefZ_Processing/config.cpp ist tabu, flour.p3d nicht bearbeitet.
* Erst nach dem naechsten Build pruefbar: im Client-RPT muss fuer bread.p3d, carrot.p3d,
  tomato_sauce.p3d usw. "Model config used: model.cfg" stehen; dann im COT-Vorschaufenster je Stufe
  die Textur wechseln.
