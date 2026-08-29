# V-B — Abnahme der Entscheidungen OF-01, OF-02, OF-03, OF-05

Vorarbeit aus `19 §2`. Kein Code. Diese Datei ist das **Abnahmeprotokoll**: sie hält
fest, was entschieden ist, woran es belegt ist, was daraus für welchen
Implementierungsschritt zwingend folgt und was eine spätere Umkehr kostet.

**Verbindlichkeit.** Ab hier gilt: wer in S6, S9, S10, S12, S13 oder im Content von
M2/M3 anders baut, als hier steht, baut falsch — nicht neu.

| ID | Frage | Entschieden | Wirkt auf |
|---|---|---|---|
| OF-01 | Food States | **D — Hybrid, Klassentausch als V1-Normalfall** | S9, S11, `06`, `11` |
| OF-02 | Persistenzmechanik | **A — eigener `OnStoreSave`-Block, kein CF** | S9, `06 §6` |
| OF-03 | `extraItems`-Default | **A — `forbid`, `extraItemsAllowedIf` als Ventil** | S6, `08 E2` |
| OF-05 | Qualität vs. Nährwert | **B — Ausbeute; A (eigene Klasse) nur punktuell** | S12, S13, S15, M2/M3-Content |

Alle vier folgen der Empfehlung aus `OFFENE_ENTSCHEIDUNGEN.md`. Nichts davon wurde
gedreht. Was hinzukommt, sind **vier Auflagen** aus einer erneuten, eigenhändigen
Lesung der Vanilla-Quellen (§6) — sie schärfen die Entscheidungen, sie kippen keine.

## 0. Wie die Belege entstanden sind

Jede Fundstelle in diesem Protokoll wurde für die Abnahme **selbst nachgeschlagen**,
nicht aus `01_Vanilla_Befunde.md` übernommen. Quelle:
`Mod Repositories\scripts - 1.29`, Pfade relativ zu dessen Wurzel.

Wo die Lesung den Befund bestätigt, steht das Kürzel aus `01` dabei. Wo sie mehr
gefunden hat, als in `01` steht, ist es als **Neu** markiert und hat eine Auflage
zur Folge.

## 1. OF-01 — Food States: Hybrid, Klassentausch als V1-Normalfall

### Entscheidung

Option **D**. ChefZ-Zustand lebt ausschließlich auf ChefZ-eigenen Klassen
(`ChefZ_Edible_Base : Edible_Base`, `ChefZ_Item_Base : ItemBase`). Der
**Klassentausch** ist der V1-Normalfall (`ChefZ_TransformDef.resultClass`), die
Zustandsvariable die Ausnahme (`setState`). **Kein `modded class Edible_Base`,
kein `modded enum FoodStageType`.**

### Belege

| Fundstelle | Aussage |
|---|---|
| `4_World/DayZ/Classes/FoodStage/FoodStage.c:1-12` | `enum FoodStageType { NONE=0, RAW=1, BAKED, BOILED, DRIED, BURNED, ROTTEN, COUNT }` — `COUNT` trägt den Kommentar `//for net sync purposes` |
| `4_World/DayZ/Entities/ItemBase/Edible_Base.c:31` | `RegisterNetSyncVariableInt("m_FoodStage.m_FoodStageType", FoodStageType.NONE, FoodStageType.COUNT)` — die Netzsync-**Breite** hängt an `COUNT` |
| `FoodStage.c:113` und `FoodStage.c:164` | zwei statische Map-Aufbauten iterieren `for (int i = 1; i < FoodStageType.COUNT; ++i)` über `visual_properties`, `nutrition_properties` und `FoodStageTransitions` |
| `4_World/DayZ/Classes/PlayerStomach.c:128` | **Neu:** `const int ACCEPTABLE_FOODSTAGE_MAX = FoodStageType.COUNT - 1;` — eine **dritte** Abhängigkeit von `COUNT`, im Verzehrpfad |
| `4_World/DayZ/Entities/ItemBase.c:2654-2658` | `HasFoodStage()` liest `CfgVehicles <Klasse> Food FoodStages` — ob ein Item überhaupt FoodStages hat, ist eine **Klassen**eigenschaft |

Bestätigt `01` V4 und V15.

### Was das entscheidet

`FoodStageType` zu erweitern ändert die Sync-Bitbreite **jedes** Nahrungsmittels im
Spiel, invalidiert zwei prozessweite statische Maps und verschiebt zusätzlich die
Plausibilitätsgrenze im Magen (`PlayerStomach.c:401-417`: ein Foodstage außerhalb
`0..ACCEPTABLE_FOODSTAGE_MAX` wird **stillschweigend** auf `RAW` zurückgesetzt).
Option B ist damit dreifach ausgeschlossen, nicht nur einfach.

Weil `HasFoodStage()` an der Klasse hängt und `01` V6 (§4 hier) die Nährwerte an die
Klasse bindet, ist der Klassentausch nicht nur zulässig, sondern der einzige Weg,
auf dem geräucherte Wurst andere Nährwerte haben kann als rohe.

### Folgen, verbindlich

1. **S9** baut `ChefZ_Edible_Base` und `ChefZ_Item_Base` als neue Klassen. Die
   geschlossene Liste aus `00 §5` bleibt bei **zwei** Einträgen (`Cooking`,
   `PluginRecipesManagerBase`). Wächst sie, ist nicht die Liste falsch.
2. **S9** baut `ChefZ_ItemTransform.Swap()` als vollwertigen Pfad, nicht als
   Nebenweg. `Edible_Base.TransferFoodStage()` wird benutzt, nicht nachgebaut.
3. Der Core hat **zu keinem der beiden Wege eine Meinung**. `resultClass` und
   `setState` sind gleichrangige Felder desselben `ChefZ_TransformDef`.
4. Die Projektionsregel `06 §3` ist Pflicht: Variable → `defaultState` der Klasse →
   Vanilla-FoodStage → `INVALID`. Ohne Stufe 2 wäre der Normalfall zustandslos.

### Bekannte Grenze

Ein **Vanilla**-Item kann keinen ChefZ-Zustand tragen. Gesalzenes Vanilla-Fleisch ist
ein Klassentausch nach einer ChefZ-Klasse. Das ist Absicht (Production Map §43), es
ist der Preis für null Kollisionsfläche, und es gehört in den Gate-1-Report.

### Umkehrkosten

Pro Kette ein Datenumbau (`resultClass` ↔ `setState`) plus Migration bestehender
Items. Genau dafür ist der Hybrid da. Würde man dagegen `modded class Edible_Base`
brauchen, wäre das eine Revision von Invariante I6 — eine Entwurfsänderung, keine
Datenänderung.

## 2. OF-02 — Persistenz: eigener `OnStoreSave`-Block

### Entscheidung

Option **A**. Eigener Block mit `MAGIC` + `VERSION`, geschrieben von
`ChefZ_ItemStateComponent` (`06 §4.3`). **Keine Abhängigkeit zum Community
Framework**, weder hier noch beim Logging (`18 E8`).

### Belege

| Fundstelle | Aussage |
|---|---|
| `4_World/DayZ/Entities/ItemBase.c:3120` / `:3221` | die zu erweiternden Signaturen: `override bool OnStoreLoad(ParamsReadContext ctx, int version)` und `override void OnStoreSave(ParamsWriteContext ctx)` |
| `ItemBase.c:3221-3244` | **Neu:** Vanilla schreibt selbst einen **variabel breiten** Block — ein führendes `ctx.Write(true/false)`, je nachdem, ob das Item an einem Spieler hängt, danach bedingt `raib.OnStoreSave(ctx)`. Der führende Bool ist wörtlich kommentiert: `// Keep track of if we should actually read this in or not` |
| `4_World/DayZ/Entities/ItemBase/Edible_Base.c:308-320` | `Edible_Base.OnStoreSave` ruft `super` **zuerst**, schreibt den FoodStage-Block nur `if (GetFoodStage())`, danach `m_DecayTimer` und `m_LastDecayStage` |
| `Edible_Base.c:322-350` | `OnStoreLoad` bricht bei `!super.OnStoreLoad(...)` ab und liest die Decay-Felder erst `if (version >= 115)` — Vanillas eigenes Versionsgate |

### Was das entscheidet

Der selbstbeschreibende Marker vor einem optionalen Block ist **Vanillas eigenes
Muster**, nicht eine ChefZ-Erfindung. `MAGIC` + `VERSION` ist derselbe Gedanke mit
mehr Bits.

Zwei Punkte, die die Entscheidung tragen und in `OFFENE_ENTSCHEIDUNGEN.md` nur
angedeutet sind:

1. **Der Erstinstallationsfall existiert für ChefZ gar nicht.** Der Block steht nur
   auf ChefZ-eigenen Klassen (OF-01, OF-12). Ein Item einer ChefZ-Klasse kann nicht
   aus der Zeit vor ChefZ stammen. Was `MAGIC` wirklich absichert, ist der
   Versionssprung und ein fremder Mod, der von `ChefZ_Edible_Base` **ableitet**.
2. **`HasFoodStage()` ist klassenstabil** (`ItemBase.c:2656`). Vanillas bedingter
   FoodStage-Block variiert also nicht zwischen zwei Starts derselben Klasse. Der
   Strom vor dem ChefZ-Block ist damit für eine gegebene Klasse fest.

### Folgen, verbindlich

1. **Der ChefZ-Block wird immer geschrieben, in fester Breite.** `06 §6` ist an
   dieser Stelle nicht verhandelbar. Ein bedingter Block verschöbe den Lesestrom
   jedes gespeicherten Items dieser Klasse, sobald sich zwischen zwei Serverstarts
   irgendetwas am Content ändert — und zwischen M2 und M3 ändert er sich laufend.
2. **`super` zuerst, immer.** `OnStoreSave`: `super.OnStoreSave(ctx)` als erste
   Anweisung. `OnStoreLoad`: `if (!super.OnStoreLoad(ctx, version)) return false;`
   als erste Anweisung.
3. **Ehrlich zu benennen, und im Gate-1-Report zu nennen:** ein `MAGIC`-Fehlschlag
   kann den Strom **nicht wieder ausrichten**. Er verhindert nur, dass ChefZ fremde
   Bytes als eigene deutet. Die Ausrichtung selbst wird durch die feste Breite
   gesichert, nicht durch `MAGIC`. Wer das umdreht, hat den Mechanismus nicht
   verstanden.
4. `VERSION` gehört in denselben Block und wird bei **jeder** Feldänderung erhöht.
   Store-Version neuer als der Core → Rest überspringen, Defaults, `WARN`
   (`06 §7`).
5. Persistiert wird `Name.Hash()`, nie der Sync-Ordinal (`03 E2`). Das ist zugleich
   die Vorbedingung für ein späteres OF-16-„ja".

### Umkehrkosten

Ein Wechsel auf CF-ModStorage ist eine Migration bestehender Spielstände **und** eine
harte Fremdabhängigkeit, die entscheidet, welche Server den Mod überhaupt fahren
können. Deshalb wird sie hier bewusst und einmalig ausgeschlossen.

## 3. OF-03 — `extraItems`-Default: `forbid`

### Entscheidung

Option **A**. `ChefZ_RecipePolicy.extraItems` hat den Default `"forbid"`.
`policy.extraItemsAllowedIf` ist das Ventil. Ein Rezept, das tolerant sein soll,
schreibt `"ignore"` hin — sichtbar, im Review prüfbar.

### Belege

| Fundstelle | Aussage |
|---|---|
| `4_World/DayZ/Classes/Cooking/Cooking.c:117-156` | `CookWithEquipment` kennt **kein Rezept**. Es behandelt das Gefäß und danach **jedes einzelne Cargo-Item** für sich (`ProcessItemToCook` in einer Schleife über `cargo.GetItemCount()`) |
| `Cooking.c:24`, `:94-102` | `PARAM_BURN_DAMAGE_COEF = 0.05`; jedes Cargo-Item ohne `IsCookware()` nimmt bei Überhitzung Schaden |
| `Cooking.c:430-455` | **Neu, folgenreich:** bei trockenem Kochgeschirr entscheidet **Lard im Cargo** über die Kochmethode: mit Lard `BAKING` mit `TIME_WITH_SUPPORT_MATERIAL_COEF`, ohne Lard `BAKING` mit `TIME_WITHOUT_SUPPORT_MATERIAL_COEF` |
| `Cooking.c:246-264`, `:519-527` | ohne Lard zieht jeder Stage-Wechsel `COOKING_FOOD_QUANTITY_DECREASE_AMOUNT_NONE = 25` von der Quantity ab; mit Lard wird stattdessen die Lard-Quantity gesenkt |

### Was das entscheidet

Weil Vanilla jedes Cargo-Item einzeln behandelt, ist ein fremdes Item im Topf **kein
Rauschen**, sondern ein Gegenstand, an dem Vanilla gerade arbeitet. Ein Rezept, das
bei `ignore` matcht und den Topfinhalt verbraucht, nimmt dem Spieler ein Item weg,
das er sichtbar gerade gart. `forbid` fällt stattdessen auf den Vanilla-Pfad zurück —
Invariante I2, wörtlich.

### Auflage 1 (neu, verbindlich für M2/M3)

**Jedes ChefZ-Rezept mit `methods: ["BAKING"]` in trockenem Kochgeschirr muss `Lard`
über `policy.extraItemsAllowedIf` dulden.**

Begründung: `Cooking.c:445` — Lard im Cargo ist in Vanilla **die Bedingung** für die
gute Backvariante. Ein Spieler, der Lard dazulegt, tut genau das Richtige. Mit
`forbid` und ohne Ventil bestraft ChefZ ihn dafür, indem das Rezept nicht mehr
matcht. Das ist kein Grenzfall, sondern der Normalfall des Pfannenkochens.

Der Default wird deswegen **nicht** gedreht: eine globale Toleranzliste in
`Core.json` wäre ein stilles Aufweichen von `forbid` für alle Rezepte. Die Duldung
gehört an das Rezept, das sie braucht.

**Review-Prüfpunkt für M2/M3:** Rezept mit `BAKING` und ohne `extraItemsAllowedIf` →
Rückfrage. Kandidat für einen späteren `chefzsym`-Nachbarprüfer.

### Auflage 2 (Addendum zu `02 §5.4`)

`ChefZ_CoreSettingsDef` bekommt **ein** Feld:

```c
string defaultExtraItems = "forbid";   // "forbid" | "ignore" | "consume"
```

Es gilt für Rezepte, die `policy.extraItems` nicht setzen. Damit ist die Umkehr
tatsächlich das, was OF-03 verspricht: eine Konfigurationszeile, keine
Codeänderung. Der Seed steht in `Config/Core.json`.

**S2 implementiert dieses Feld mit, S6 liest es.** Ein Rezept-eigener Wert schlägt
den Default immer.

### Folgen, verbindlich

1. **S6** setzt `extraItems` auf den Wert aus `Core.json`, wenn das Rezept schweigt.
2. **S5/S6** werten Restitems gegen `extraItemsAllowedIf` aus, **bevor** `forbid`
   greift (`07 §4` Schritt 4).
3. Das Gefäß selbst ist nie ein Restitem (`01` V13, `05`).
4. `wPolicyForbid = 0.50` in der Spezifitätsrechnung (`09 §3`) bezieht sich auf den
   **effektiven** Wert nach Default-Auflösung, nicht auf das Vorhandensein des
   Feldes. Sonst hinge die Rezeptpriorität davon ab, ob ein Autor einen Default
   ausgeschrieben hat.

### Umkehrkosten

Eine Zeile in `Core.json`. Genau deshalb darf die Entscheidung streng ausfallen.

## 4. OF-05 — Qualität wirkt auf Ausbeute, nicht auf den Bissen

### Entscheidung

Option **B als Hauptkanal**, Option **A punktuell** über `OutputDef.variants` für
einzelne Signature-Gerichte. **C und D sind verworfen** und stehen auf der
Verbotsliste (`00 §6`).

Das ist die teuerste der vier Entscheidungen: sie bestimmt, wie Content-Agenten in
M2 und M3 **jedes** Gericht auslegen.

### Belege

| Fundstelle | Aussage |
|---|---|
| `4_World/DayZ/Classes/PlayerStomach.c:401` | `void AddToStomach(string class_name, float amount, int food_stage = 0, int agents = 0, float temperature = 0)` — **kein** Item-Parameter |
| `PlayerStomach.c:408` | `profile = Edible_Base.GetNutritionalProfile(null, class_name, food_stage);` — `item` ist wörtlich `null` |
| `4_World/DayZ/Entities/ItemBase/Edible_Base.c:513-526` | `static NutritionalProfile GetNutritionalProfile(ItemBase item, string classname = "", int food_stage = 0)` — acht Getter, alle `static` |
| `Edible_Base.c:407-420` (und die Geschwister bis `:490`) | jeder Getter: `if (food_item && food_item.GetFoodStage())` … sonst `FoodStage.Get*(null, food_stage, classname)` … sonst `cfgVehicles <Klasse> Nutrition <feld>`. Mit `item == null` bleibt **nur** Klasse × Foodstage |
| `PlayerStomach.c:1-20` | `class StomachItem` hält `m_Profile`, `m_Amount`, `m_FoodStage`, `m_ClassName`, `m_Agents`, `m_Temperature` — **kein Feld für Instanzdaten** |
| `4_World/DayZ/Entities/ManBase/PlayerBase.c:7395` | der Verzehrpfad: `m_PlayerStomach.AddToStomach(data.m_Source.GetType(), data.m_Amount, foodStageType, data.m_Agents, temperature)` — vom Item geht **der Typname** über, sonst nichts |
| `PlayerStomach.c:209-250` | `InitData()` registriert nur `CfgVehicles`-Kinder mit `scope != 0` **und** `class Nutrition` **oder** `class Food`; `AddToStomach` bricht bei `GetIDFromClassname(...) == -1` **ohne Meldung** ab |

Bestätigt `01` V6 und V7 vollständig. Bestätigt zugleich OF-20: die Getter sind
`static`, und selbst ein wirksames `override static` erreichte den Verzehrpfad nicht,
weil `item` dort `null` ist.

### Was das entscheidet

Es gibt engine-seitig **keinen** Weg, den Nährwert pro Bissen an einer Instanz
festzumachen. `PlayerBase.c:7395` zeigt den vollständigen Verlust: Klasse, Menge,
Foodstage, Agents, Temperatur — mehr überquert die Grenze nicht.

Was übrig bleibt, ist die **Menge** (`data.m_Amount`). Sie geht ungefiltert in
`AddToStomach` ein und skaliert den Nährwert linear. Deshalb ist die Ausbeute nicht
ein Ersatz mit Beigeschmack, sondern **der Hebel, den die Engine anbietet**.

### Auflage 3 (neu, verbindlich für S12/S15 und für M2/M3)

**Träger der Ausbeute ist `ChefZ_Portions`, nicht die Vanilla-`quantity`.**

`12 §2` schreibt „mehr Portionen bzw. mehr Quantity". Die zweite Hälfte ist
brüchig, und zwar belegbar:

- `Cooking.c:222-227`: solange ein Item überhitzt im Gefäß liegt, zieht **jeder
  Tick** `25` von der Quantity ab.
- `Cooking.c:246-264`: jeder Stage-Wechsel ohne Lard zieht nochmals `25` ab.

Ein fertiges Gericht, das im heißen Topf liegen bleibt, verliert also genau den
Bonus, den `yieldMultiplier` ihm gegeben hat — und der Spieler sieht nur, dass
sein PREMIUM-Eintopf schrumpft. `ChefZ_Portions` ist eine ChefZ-eigene
Sync-Variable (`06 §4.3`, `0..31`) und von Vanillas Quantity-Abzügen nicht
betroffen.

Daraus:

1. `ChefZ_QualityTierDef.portionBonus` und `yieldMultiplier` wirken **primär** auf
   die Portionszahl (`15`).
2. Quantity-basierte Ausbeute (`OutputDef.quantityMode`) bleibt erlaubt, ist aber
   für Ergebnisse gedacht, die das Kochgerät sofort verlassen. Für Gerichte, die
   im Gefäß entstehen und dort liegen bleiben können, ist sie **nicht** der
   Balancinghebel.
3. `13`s Startaudit rechnet die Sollwerte gegen `CfgVehicles` — er misst damit die
   Klasse, nicht die Instanz. Das ist konsistent mit dieser Entscheidung und der
   einzige Ort, an dem der Nährwert überhaupt geprüft werden kann.

### Auflage 4 (harte Content-Regel, aus V7)

**Jede essbare ChefZ-Ergebnisklasse braucht `class Nutrition` oder `class Food` und
`scope != 0`.** Fehlt das, wird das Gericht gegessen, verschwindet und sättigt
nichts — ohne Fehlermeldung, ohne Log, ohne Hinweis (`PlayerStomach.c:209-250` plus
der stille `return` in `AddToStomach`).

Abgefangen an drei Stellen (`13 §3`): `chefznut.mjs` statisch, Startaudit beim Boot,
Rezeptabweisung in der Recipe Engine. Der Validator ist die billigste der drei und
ist Auflage zu OF-18 — er gehört gebaut, bevor M2 Gerichte schreibt.

### Folgen für M2/M3, verbindlich

1. **Ein Gericht ist eine Klasse.** Nicht vier. Qualitätsstufen sind Ausbeute,
   Haltbarkeit, Anzeige, Tags und Effekt-IDs — keine Klassenvervierfachung.
2. `OutputDef.variants` ist die **Ausnahme** für einzelne Signature-Gerichte. Als
   Faustzahl: eine Handvoll im ganzen V1-Umfang, jede einzeln begründet.
3. Kein Content-Modul und kein Comp-Mod addiert Energie oder Wasser in `OnConsume`
   (`13 E3`). Verstöße sind ein Abbruchgrund im Review.

### Umkehrkosten

Von B auf A zu wechseln vervierfacht jede Gerichteklasse — Modell, Nutrition-Block,
Stringtable, Loot-Eintrag je Stufe. Von A auf B zurück wäre ein Rückbau von rund 75
Klassen. Genau deshalb fällt die Entscheidung vor M2 und nicht in M3.

## 5. Was diese vier Entscheidungen zusammen bewirken

Sie greifen ineinander, und das ist kein Zufall:

```text
OF-01  Zustand nur auf ChefZ-Klassen
   |      => kein modded class Edible_Base
   |      => OF-12 (Sync nur auf ChefZ-Klassen) ist damit mitentschieden
   v
OF-02  eigener Persistenzblock auf genau diesen Klassen
   |      => OF-19 (Kollisionsrisiko) verliert seinen Gegenspieler:
   |         es gibt niemanden, mit dem man kollidieren koennte
   v
OF-05  Qualitaet wirkt ueber Portionen -- eine ChefZ-eigene, persistierte,
   |   gesyncte Variable auf genau diesen Klassen
   v
OF-03  und wenn nichts davon passt, faellt alles auf Vanilla zurueck
```

Die gemeinsame Linie: **ChefZ schreibt nur auf das, was ihm gehört.** Zustand,
Qualität, Frische, Portionen, Persistenz — alles vier lebt auf ChefZ-Klassen, und
der Rückfallpfad ist überall unverändertes Vanilla.

## 6. Auflagen dieser Abnahme — Sammelliste

| # | Auflage | Wer | Wann |
|---|---|---|---|
| 1 | `BAKING`-Rezepte dulden `Lard` über `extraItemsAllowedIf` | Content M2/M3 | vor dem ersten Pfannenrezept |
| 2 | `ChefZ_CoreSettingsDef.defaultExtraItems` (Addendum zu `02 §5.4`) | S2 baut, S6 liest | S2 |
| 3 | Ausbeute reitet auf `ChefZ_Portions`, nicht auf `quantity` | S12, S15, Content | S12 |
| 4 | Essbare Ergebnisklassen brauchen `Nutrition`/`Food` und `scope != 0`; `chefznut.mjs` vor M2 | Werkzeug + Content | vor M2 |

## 7. Bekannte Grenzen, in den Gate-1-Report zu übernehmen

1. Ein **Vanilla**-Item kann keinen ChefZ-Zustand tragen (OF-01).
2. Ein `MAGIC`-Fehlschlag **erkennt** eine Fehlausrichtung, er **behebt** sie nicht.
   Die Ausrichtung sichert die feste Blockbreite (OF-02).
3. `TIMED`-Kochfortschritt überlebt keinen Serverneustart, Stationsfortschritt schon
   (OF-14). Bewusste Asymmetrie: ChefZ speichert nur auf eigenen Klassen.
4. Qualität kann den Nährwert **pro Bissen** nie verändern. Wer eine Balancing-Idee
   hat, die das voraussetzt, hat eine Idee, die DayZ nicht zulässt (OF-05).

## 8. Was diese Abnahme im Addon hinterlässt

| Datei | Zweck |
|---|---|
| `Decisions/V_B_Entscheidungen.md` | dieses Protokoll |
| `Config/Core.json` | Seed für `ChefZ_CoreSettingsDef` (`02 §5.4`) — Feldnamen und Defaults wörtlich aus dem Entwurf, **plus** `defaultExtraItems: "forbid"` aus Auflage 2 |

Sonst nichts. V-B ist laut `19 §2` ausdrücklich **kein Code**; `ChefZ_Log`,
`ChefZ_ItemStateComponent`, `ChefZ_Edible_Base` und die Recipe Engine entstehen in
S1, S9 und S6 — nicht hier.

**Bewusst nicht geschrieben:** ein `CfgChefZ`-Manifesteintrag für `Config/Core.json`.
Die Pfad**wurzel** ist inzwischen entschieden (B4, `02 §4.1`): sie ist das PBO-Präfix
und damit der Ordnername, also `ChefZ_Core/Config/…`. Offen bleibt nur, ob eine
JSON-Datei aus einem Mod-PBO überhaupt lesbar ist und in welcher Schreibweise
(Trennzeichen, Groß-/Kleinschreibung) — das entscheidet die **Messung** aus V-A,
deren Protokollblock in `Tests/V_A_PboJsonSmoke/README.md §5` noch leer ist. Einen
Pfad zu raten, den der Loader später still verwirft, wäre die schlechteste aller
Varianten.
Der Eintrag entsteht in S2, mit dem Messergebnis in der Hand. Fällt V-A negativ aus,
wird derselbe Datensatz stattdessen zu einem `CfgChefZSeed`-Klassenbaum — die Werte
in `Core.json` bleiben davon unberührt, nur ihre Quelle wechselt (`02 E7`, OF-10).

## 9. Abnahmevermerk

```text
V-B  Entscheidungen OF-01, OF-02, OF-03, OF-05
  Datum:            ____________
  Abgenommen von:   ____________
  OF-01  [ ] D Hybrid, Klassentausch als V1-Normalfall
  OF-02  [ ] A eigener OnStoreSave-Block, kein CF
  OF-03  [ ] A forbid, extraItemsAllowedIf als Ventil
  OF-05  [ ] B Ausbeute; A punktuell ueber OutputDef.variants
  Auflagen 1-4 aus §6 zur Kenntnis genommen:   [ ]
  Grenzen 1-4 aus §7 in den Gate-1-Report:     [ ]
  Abweichungen (falls eine Entscheidung anders faellt, hier begruenden):
```

Solange dieser Vermerk leer ist, sind **S6** und **S9** formal blockiert (`19 §2`).
Die Entscheidungen selbst sind damit nicht offen — sie sind getroffen und begründet;
offen ist nur die Gegenzeichnung.
