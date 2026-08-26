# ChefZ × Terje – Kompatibilitätsanalyse & technische Notizen

## Ziel

Diese Notizen fassen die für **ChefZ** relevanten Erkenntnisse aus **TerjeCore**, **TerjeSkills** und **TerjeMedicine** zusammen.

Ziel ist eine saubere Integration, bei der:

- **ChefZ_Core unabhängig von Terje bleibt**
- Terje nur über optionale Compatibility-Module angebunden wird
- vorhandene Terje-Systeme genutzt statt dupliziert werden
- eigene ChefZ-Perks ergänzt werden können
- XP, Skills, Medizin und Food-Effekte sauber getrennt bleiben

---

# 1. Wichtigste Erkenntnis

ChefZ sollte **Terje nicht nachbauen**, sondern sich an die vorhandenen Systeme anhängen.

Terje besitzt bereits dynamische Systeme für:

- Skills
- XP
- Perks
- Skill Modifier
- Ernährung
- Lebensmittelvergiftung
- Immunität
- medizinische Consumable-Effekte
- Crafting Conditions
- Persistenz
- Skill-/Perk-UI

Die Terje-Skill-Registry liest Skills, Perks und Modifier dynamisch aus `CfgTerjeSkills`.

Dadurch ist es möglich, dass ein externes Modul wie `ChefZ_TerjeSkills` eigene Perks in bestehende Terje-Skills einfügt.

---

# 2. Eigene ChefZ-Perks sind möglich

Terje selbst nutzt dieses Prinzip bereits.

Beispiel:

`TerjeRadiation` erweitert den bestehenden Skill `Immunity` um zusätzliche Perks wie:

- Radiation Resist
- Radiation Regeneration

ChefZ kann dasselbe Prinzip verwenden.

Beispiel:

```text
ChefZ_TerjeSkills
        ↓
Terje Survival
        ↓
ChefZ Herbalist
```

Terje-Dateien müssen dafür nicht direkt verändert werden.

---

# 3. Für ChefZ relevante Terje-Skills

| Skill | ID | ChefZ-Relevanz |
|---|---|---|
| Survival | `surv` | Kochen, Kräuter, Konservierung, Bushcraft |
| Metabolism | `mtblsm` | Essen, Kalorien, Hydration |
| Hunting | `hunt` | Tiere zerlegen, Fleischgewinnung |
| Fishing | `fish` | Fischfang und Filetieren |
| Immunity | `immunity` | verdorbenes Essen, Vergiftung, gesundes Essen |
| Medicine | `med` | Heilkräuter, medizinische Tees |
| Strength | `strng` | nur indirekt |
| Athletics | `athlc` | kaum ChefZ-relevant |
| Stealth | `stlth` | nicht ChefZ-relevant |

---

# 4. Survival als Hauptskill für ChefZ

Für V1 sollte ChefZ **keinen eigenen Cooking Skill** bekommen.

Terje behandelt Kochen bereits als Survival-Aktivität.

ChefZ-Aktivitäten, die Survival zugeordnet werden sollten:

```text
Kochen
Konservieren
Kräuter sammeln
Salz herstellen
Räuchern
Trocknen
Teig herstellen
Wurst herstellen
Gewürze verarbeiten
```

---

# 5. Bestehende Survival-Perks

Terje Survival besitzt bereits unter anderem:

- Starting Fire
- Cold Resistance
- Rough Feet
- Rough Hands
- Ancestral Technologies
- Maintaining Fire
- Durable Equipment
- Bushcraft
- Expert
- Stashes
- Survival Instinct
- Mushroom Premonition

Besonders interessant für ChefZ:

## Mushroom Premonition

Dieser Perk hebt Pilze visuell hervor.

Das ist eine sehr gute technische Vorlage für ChefZ-Kräuter.

---

# 6. Neuer ChefZ-Perk: Kräuterkundiger / Herbalist

Empfohlener Name:

**Deutsch:** Kräuterkundiger  
**Englisch:** Herbalist

Skill:

```text
Survival
ID: surv
```

Perk-ID:

```text
chefzherb
```

Die ID sollte bewusst mit `chefz` beginnen, um Konflikte mit zukünftigen Terje-Perks zu vermeiden.

---

# 7. Herbalist – vorgeschlagene 5 Stufen

| Stufe | Survival Level | Effekt |
|---:|---:|---|
| I | 5 | Kräuter in unmittelbarer Nähe hervorheben |
| II | 10 | +10 % Ernteausbeute |
| III | 15 | größere Erkennungsreichweite, +20 % Ausbeute |
| IV | 25 | größere Erkennungsreichweite, +30 % Ausbeute |
| V | 35 | maximale Reichweite, +50 % Ausbeute |

Vorgeschlagene Perk-Point-Kosten:

```text
1 / 1 / 1 / 2 / 2
```

Vorgeschlagene Werte:

```text
0.00
0.10
0.20
0.30
0.50
```

Diese Werte könnten den Yield-Bonus repräsentieren.

Die Highlight-Reichweite kann separat anhand des Perk-Levels bestimmt werden.

---

# 8. Beispielstruktur des Herbalist-Perks

```cpp
class CfgTerjeSkills
{
    class SkillsBase;

    class Survival: SkillsBase
    {
        class Perks
        {
            class ChefZHerbalist
            {
                id = "chefzherb";
                enabled = 1;

                displayName = "#STR_CHEFZ_PERK_HERBALIST";
                description = "#STR_CHEFZ_PERK_HERBALIST_DESC";

                stagesCount = 5;

                requiredSkillLevels[] =
                {
                    5,
                    10,
                    15,
                    25,
                    35
                };

                requiredPerkPoints[] =
                {
                    1,
                    1,
                    1,
                    2,
                    2
                };

                values[] =
                {
                    0.0,
                    0.1,
                    0.2,
                    0.3,
                    0.5
                };

                disabledIcon = "...";
                enabledIcon = "...";
            };
        };
    };
};
```

Terje unterstützt bis zu 10 Perk-Stufen.

Wichtig:

`stagesCount`, `requiredSkillLevels`, `requiredPerkPoints` und `values[]` müssen dieselbe Anzahl Einträge besitzen.

---

# 9. Kräuter-Kategorisierung in ChefZ

ChefZ sollte nicht jede Kräuterklasse einzeln im Compatibility-Modul prüfen.

Stattdessen sollte ChefZ ein internes Tag-System besitzen.

Beispiel:

```text
CHEFZ_HERB
```

Mögliche ChefZ-Kräuter:

```text
ChefZ_Parsley
ChefZ_Dill
ChefZ_Thyme
ChefZ_Rosemary
ChefZ_WildGarlic
```

Logik:

```text
Item has CHEFZ_HERB
        +
Player has chefzherb
        ↓
Apply Herbalist behaviour
```

Dadurch bleiben spätere Erweiterungen einfach.

---

# 10. Kräuter sammeln und Survival XP

Für Kräuterernte wurde kein bereits fertiger Terje-Hook gefunden.

Das sollte daher `ChefZ_TerjeSkills` übernehmen.

Vorgeschlagene XP-Werte:

| Aktion | Survival XP |
|---|---:|
| gewöhnliches Kraut ernten | 2 |
| seltenes Kraut ernten | 4–5 |
| Pfeffer ernten | 5 |
| Kräuter trocknen | 3 |
| Gewürzmischung herstellen | 5 |
| medizinische Kräutermischung herstellen | 8 |

Wichtig:

XP nur nach tatsächlich erfolgreicher Aktion.

Nicht:

```text
Action gestartet
→ XP
```

Sondern:

```text
Harvest Completed
→ Result Item erzeugt
→ XP
```

---

# 11. Metabolism muss kaum speziell integriert werden

Terje hängt direkt in `PlayerStomach`.

Beim Essen werden:

- Energie
- Wasser
- Nutritional Profile

ausgewertet.

Dadurch erhalten normale ChefZ-Gerichte automatisch Metabolism-XP, wenn sie korrekt als DayZ-Food definiert sind.

Beispiel:

```text
ChefZ_SurvivorSpaghetti
ChefZ_HunterPlate
ChefZ_FishPotatoPlate
```

Ablauf:

```text
ChefZ Food
↓
PlayerStomach
↓
Terje Metabolism
↓
XP
```

## Wichtig

ChefZ darf beim Essen **nicht zusätzlich Metabolism XP vergeben**.

Sonst entsteht doppelte XP-Vergabe.

---

# 12. Bestehende Metabolism-Perks

Terje Metabolism besitzt bereits passende Perks:

- Increased Calorie
- Increased Hydration
- Energy Saving
- Water Saving
- Energy Control
- Hydration Control
- Resist Hunger
- Wild Meat Lover

Diese sollten ChefZ-Food automatisch beeinflussen, sofern die Gerichte normale Nutritional Profiles verwenden.

---

# 13. Wild Meat Lover

Der bestehende Terje-Perk `wmlover` entfernt unter bestimmten Bedingungen Salmonella.

ChefZ sollte daher keinen zusätzlichen parallelen Wildfleisch-Perk bauen, der denselben Zweck erfüllt.

Keine doppelte Mechanik wie:

```text
Hunter Meal
→ Immun gegen Salmonella
```

---

# 14. Hunting – klare Systemgrenze

Terje Hunting ist bereits sehr umfangreich.

Vorhandene Perks umfassen unter anderem:

- Meat Hunter
- Quick Skinning
- Master Knife
- Trap Expert
- Pelt Master
- Experienced Hunter
- Knowledge Anatomy

Terje berücksichtigt beim Tierzerlegen bereits:

- Tier-spezifische XP
- Messer-Modifikatoren
- Messerabnutzung
- Fleischmenge
- Knochen
- Pelze
- Skinning-Geschwindigkeit
- Perks

Daher:

```text
TERJE
Animal
↓
Butchering
↓
Meat
```

Danach:

```text
CHEFZ
Meat
↓
Minced Meat
↓
Sausage
↓
Smoking
↓
Dish
```

---

# 15. Keine Hunting XP für Wurstherstellung

Wurstherstellung sollte kein Hunting XP geben.

Grund:

Ein Spieler könnte Fleisch kaufen und damit Hunting leveln, ohne je gejagt zu haben.

Daher:

```text
Tier zerlegen
→ Hunting XP
```

```text
Fleisch weiterverarbeiten
→ Survival XP
```

---

# 16. Fishing – gleiche Trennung

Terje Fishing besitzt bereits unter anderem:

- Master Fillet
- Quick Cleaning
- Straight Arms
- Master Traps
- Skilled Fisherman
- Fisherman Luck
- Reliable Gear
- Worm Hunter
- Craftsman
- Remove Rotten Fish

Terje beeinflusst bereits:

- Fillet-Menge
- Messerabnutzung
- Verarbeitungsgeschwindigkeit
- Fishing XP

Daher:

```text
Catch Fish
→ Terje Fishing

Fillet Fish
→ Terje Fishing

Fish → Smoked Fish
→ ChefZ Survival

Fish → Dinner
→ ChefZ Survival
```

---

# 17. Immunity ist für ChefZ sehr relevant

Terje Medicine fügt den Skill hinzu:

```text
immunity
```

Relevante Perks:

- Poison Resistance
- Intoxication Resistance
- Safe Dinner
- Blood Regeneration
- Wound Healing
- Disease Resistance

Besonders relevant:

```text
svdinner
```

= Safe Dinner

---

# 18. Safe Dinner nicht duplizieren

Safe Dinner reduziert bereits die Wirkung bzw. Übertragung von Food Poisoning.

ChefZ sollte dieses System nutzen und keine zweite parallele Food-Poison-Resistance implementieren.

---

# 19. Food-Risiken in ChefZ

ChefZ sollte echte DayZ/Terje-Risiken erhalten.

Beispiele:

## Verdorbenes Essen

```text
FOOD_POISON
```

## Schlecht gegartes Fleisch

```text
SALMONELLA
```

## Kontaminiertes Wasser

```text
CHOLERA
```

Terje übernimmt anschließend die gesundheitlichen Konsequenzen.

ChefZ benötigt kein eigenes Krankheitssystem.

---

# 20. ChefZ_TerjeMedicine

Dieses Compatibility-Modul kann medizinische Effekte auf ChefZ-Items legen.

Terje unterstützt unter anderem:

- Antibiotic
- Antipoison
- Antibiohazard
- Antidepressant
- Blood Regeneration
- Hemostatic
- Immunity Gain
- Health Regeneration
- Antisepsis
- Adrenalin
- Vaccine Effects
- direkte Health/Blood/Shock/Energy/Water-Werte

---

# 21. Kräutertees

ChefZ selbst könnte nur die Items bereitstellen:

```text
ChefZ_ThymeTea
ChefZ_WildGarlicTea
ChefZ_HerbalTea
```

Ohne Terje-Wirkung.

Erst:

```text
ChefZ_TerjeMedicine
```

fügt medizinische Eigenschaften hinzu.

Beispiel:

```text
ChefZ_HerbalTea
↓
medImmunityGainValue
medImmunityGainTimeSec
```

Dadurch bleibt ChefZ auch ohne Terje voll funktionsfähig.

---

# 22. Beispiel: Immunitäts-Tee

Vanilla Vitamins werden von Terje beispielsweise über Immunity-Gain-Parameter erweitert.

ChefZ könnte dieselbe Mechanik in deutlich schwächerer Form verwenden.

Beispiel:

```text
Wild Garlic Tea

Immunity Gain:
klein

Duration:
kurz
```

Wichtig:

Kräutertees sollen Medikamente **nicht ersetzen**.

Sie sollten eher:

- präventiv
- unterstützend
- leicht regenerativ

wirken.

---

# 23. Medicine Skill und Pharmacologist

Terje Medicine besitzt den Skill:

```text
med
```

und unter anderem den Perk:

```text
pharmac
```

Pharmacologist kann medizinische Consumable-Effekte beeinflussen.

Dadurch könnten ChefZ-Kräutertees automatisch mit diesem Terje-Perk interagieren, wenn sie als Terje-Medicine-Consumables definiert werden.

---

# 24. XP über Consumables

Terje Consumables können grundsätzlich Skill-XP vergeben.

Technisch ist dies möglich über Werte wie:

```text
<skillId>SkillExpAddToSelf
<skillId>SkillExpAddToTarget
```

Für normales ChefZ-Essen sollte diese Funktion jedoch **nicht verwendet werden**.

Grund:

```text
Essen kaufen
↓
essen
↓
Skill farmen
```

Metabolism deckt Food-XP bereits sinnvoll ab.

---

# 25. Terje Skills API für ChefZ

ChefZ_TerjeSkills sollte die vorhandene Terje-Skills-API verwenden.

Relevante Zugriffe:

```text
GetSkillLevel()
GetSkillExperience()
AddSkillExperience()

GetPerkLevel()
GetPerkValue()
IsPerkRegistered()
```

Nicht direkt auf interne Profile zugreifen.

Empfohlener Weg:

```text
ChefZ
↓
Terje Skills Accessor
↓
Terje Profile
```

---

# 26. Vorgeschlagene ChefZ-XP-Matrix

| ChefZ-Aktion | Terje Skill | XP |
|---|---|---:|
| Kraut ernten | Survival | 2–5 |
| seltenes Kraut ernten | Survival | 5 |
| Kräuter trocknen | Survival | 3 |
| Pfeffer trocknen | Survival | 3 |
| Gewürz mahlen | Survival | 2 |
| Salz aus Salzwasser herstellen | Survival | 5 |
| Mehl mahlen | Survival | 3 |
| Teig herstellen | Survival | 3 |
| Nudeln herstellen | Survival | 5 |
| einfache Mahlzeit | Survival | 3 |
| komplexe Mahlzeit | Survival | 8 |
| Premium-Gericht | Survival | 15 |
| Wurst herstellen | Survival | 5 |
| Räucherwurst | Survival | 8 |
| Trockenfleisch | Survival | 5 |
| Fisch räuchern | Survival | 5 |
| Tier zerlegen | Terje Hunting | keine ChefZ-XP |
| Fisch filetieren | Terje Fishing | keine ChefZ-XP |
| Essen | Terje Metabolism automatisch | keine ChefZ-XP |

Diese Werte sind Balancing-Vorschläge.

---

# 27. Anti-XP-Farming

XP sollte ausschließlich nach abgeschlossener Produktion vergeben werden.

Beispiel:

```text
Noodles + Sausage
→ Sausage Pasta
→ +5 Survival XP
```

Nicht:

```text
Ingredient inserted
→ XP

Ingredient removed
Ingredient inserted
→ XP
```

## Batch-Verarbeitung

Bei Batch-Produktion sollte nicht automatisch volle XP pro Stück vergeben werden.

Beispiel:

```text
10x Sausage
```

Nicht:

```text
10 × volle XP
```

Sondern beispielsweise:

```text
Base XP
+ kleiner Mengenbonus
```

---

# 28. Möglicher weiterer Perk: Preserver / Vorratsexperte

Später sinnvoll:

```text
chefzpreserve
```

Skill:

```text
Survival
```

Mögliche Effekte:

- schnelleres Trocknen
- schnelleres Räuchern
- geringerer Salzverbrauch
- höhere Haltbarkeit
- bessere Produktausbeute

Dieser Perk sollte erst nach Herbalist geplant werden.

---

# 29. Möglicher weiterer Perk: Field Cook / Feldkoch

ID:

```text
chefzcook
```

Skill:

```text
Survival
```

Mögliche Effekte:

- geringere Kochzeit
- geringeres Verbrennungsrisiko
- bessere Dish Quality
- kleine Chance auf zusätzliche Portion
- Premium-Rezepte

Langfristig sinnvoller als ein komplett eigener Cooking Skill.

---

# 30. Eigener Cooking Skill?

Technisch scheint ein vollständig eigener Skill möglich zu sein.

Terje registriert Skills dynamisch aus `CfgTerjeSkills`.

Auch Persistenz und UI arbeiten dynamisch.

Damit wäre theoretisch möglich:

```text
Cooking
ID: chefzcooking
```

Aktuell aber nicht empfohlen.

Grund:

Zu starke Überschneidung mit:

- Survival
- Metabolism
- Hunting
- Fishing

Für V1 sollte Kochen Teil von Survival bleiben.

---

# 31. Terje Custom Crafting

TerjeCore besitzt ein eigenes Custom-Crafting-System.

Profilpfad:

```text
$profile:TerjeSettings\CustomCrafting\Recipes.xml
```

Es unterstützt serverseitige Conditions.

Unter anderem existieren Conditions für:

```text
SkillLevel
SkillPerk
```

Dadurch könnten Rezepte grundsätzlich an Terje-Skills gekoppelt werden.

Beispiel:

```text
Hunter Sausage
requires:
Survival Level 10
```

Oder:

```text
Advanced Herbal Mix
requires:
chefzherb Stage 3
```

---

# 32. ChefZ behält trotzdem seine eigene Recipe Engine

Terje Custom Crafting eignet sich gut für einfache:

```text
Item A + Item B → Item C
```

ChefZ benötigt aber komplexere Kochsysteme.

Beispiel:

```text
Cooking Pot
├ Meat
├ Potato
├ Tomato
├ Mushroom
├ Salt
└ Thyme
```

ChefZ benötigt zusätzlich:

- optionale Zutaten
- Kategorien
- Rezeptprioritäten
- Flüssigkeiten
- Portionsgrößen
- Kochtemperatur
- Kochgeräte
- Seasoned/Premium Quality
- mehrstufige Verarbeitung

Daher bleibt:

```text
ChefZ_Core Recipe Engine
```

bestehen.

Terje wird nur ergänzend eingebunden.

---

# 33. Empfohlene Modulstruktur

```text
ChefZ_Core
│
├── Recipe Engine
├── Categories
├── Processing
├── Food Tags
├── Events
└── Generic APIs

ChefZ_Farming
ChefZ_Ingredients
ChefZ_Processing
ChefZ_Meat
ChefZ_Preservation
ChefZ_Cooking
ChefZ_Baking
ChefZ_UI
│
└── Compatibility
    │
    ├── ChefZ_TerjeSkills
    │   ├── Survival XP
    │   ├── Herbalist
    │   ├── Cooking Perks
    │   └── Skill Conditions
    │
    └── ChefZ_TerjeMedicine
        ├── Herbal Effects
        ├── Food Poisoning
        ├── Immunity
        └── Medicinal Teas
```

---

# 34. Wichtige Architekturregel

```text
ChefZ_Core
```

hat **keine Terje-Abhängigkeit**.

Stattdessen:

```text
ChefZ_Core
↑
ChefZ_TerjeSkills
↓
TerjeSkills
```

und:

```text
ChefZ_Core
↑
ChefZ_TerjeMedicine
↓
TerjeMedicine
```

Terje erweitert ChefZ.

ChefZ benötigt Terje nicht zum Funktionieren.

---

# 35. Aktuelle Perk-Empfehlung für V1

Für die erste Version sollte nur ein eigener Terje-Perk umgesetzt werden:

## Kräuterkundiger / Herbalist

```text
Skill:
Survival

ID:
chefzherb

Stages:
5
```

Mechanik:

```text
Stage I
Herbs erkennen / Highlight

Stage II
+10 % Yield

Stage III
+20 % Yield
größere Detection Range

Stage IV
+30 % Yield
noch größere Range

Stage V
+50 % Yield
maximale Detection Range
```

Technische Vorbilder aus Terje:

- `MushroomPremonition` für Highlighting
- `MeatHunter` für Ressourcen-Ausbeute
- `MasterFillet` für Ressourcen-Ausbeute

Damit passt der Perk stilistisch und technisch sehr gut in das bestehende Terje-System.

---

# 36. Nächster Planungsschritt

Als nächstes sollte `ChefZ_TerjeSkills` vollständig spezifiziert werden.

Dafür jeden ChefZ-Prozess einzeln definieren:

```text
Kräuter
↓
Salz
↓
Mahlen
↓
Fleisch
↓
Wurst
↓
Räuchern
↓
Trocknen
↓
Kochen
↓
Premium-Gerichte
```

Für jeden Prozess festlegen:

- verwendeter Terje-Skill
- XP-Menge
- relevante Perks
- Stärke der Effekte
- Anti-Exploit-Regeln
- serverseitige Config-Werte
- benötigte Hooks
- benötigte ChefZ Events
- benötigte Terje APIs
- optionale Medicine-Kompatibilität

---

# 37. Geplante Compatibility-Module

## ChefZ_TerjeSkills

Aufgaben:

- Survival XP
- Herbalist Perk
- weitere Cooking-/Preservation-Perks
- Skill Requirements
- Skill-/Perk-Abfragen
- Anti-XP-Farming
- optionale Recipe Locks

## ChefZ_TerjeMedicine

Aufgaben:

- medizinische Kräuter
- Kräutertees
- Immunity Gain
- leichte Regeneration
- Food Poisoning Integration
- Salmonella / Cholera / Food Poison Handling
- Interaktion mit Medicine Skill
- Interaktion mit Pharmacologist

---

# Status

## Bestätigt sinnvoll

- ChefZ Core bleibt Terje-unabhängig
- Terje Compatibility wird separat gebaut
- Kochen nutzt Survival
- Essen nutzt automatisch Metabolism
- Tierzerlegung bleibt Hunting
- Fischzerlegung bleibt Fishing
- Food Poisoning nutzt Terje Medicine
- Kräuterkundiger wird neuer Survival-Perk
- eigener Cooking Skill vorerst nicht nötig

## Später genauer planen

- genaue Herbalist-Reichweiten
- Yield-Berechnung
- XP-Balancing
- Cook Perk
- Preservation Perk
- konkrete Kräuter-Medicine-Effekte
- Recipe Locks nach Skill/Perk
- serverseitige Config-Struktur
