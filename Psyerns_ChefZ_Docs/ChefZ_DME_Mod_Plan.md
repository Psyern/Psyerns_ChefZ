# ChefZ – DME Cooking & Food Production Overhaul

## 1. Projektübersicht

**ChefZ** ist ein umfangreicher Cooking- und Food-Production-Mod für **Deadmans Echo (DME)**.

Die Grundidee orientiert sich an der simplen und sehr gut funktionierenden Logik von CookZ:

- Spieler benutzen Pfannen, Kochtöpfe und Kessel wie gewohnt.
- Befinden sich passende Zutaten im Kochgerät, wird ein Rezept erkannt.
- Die Zutaten werden entfernt.
- Das fertige Gericht wird erzeugt.
- Passt kein Rezept, greift weiterhin das normale DayZ-Cooking.

ChefZ soll dieses Prinzip jedoch deutlich erweitern und daraus ein vollständiges Survival-Food-System machen.

Der Fokus liegt nicht nur auf Rezepten, sondern auf einer zusammenhängenden Kette aus:

**Sammeln → Anbauen → Ernten → Verarbeiten → Würzen → Kochen → Konservieren → Servieren**

ChefZ soll sich klar nach DayZ anfühlen: rustikal, improvisiert, postapokalyptisch und praktisch.

---

# 2. Hauptziele

ChefZ soll:

- deutlich mehr Abwechslung beim Essen bieten
- Jagd und Landwirtschaft aufwerten
- Fischfang sinnvoller machen
- Pflanzen und Kräuter wichtiger machen
- Fleischverarbeitung vertiefen
- eigene 3D-Modelle für Zutaten und Gerichte verwenden
- echte Produktionsketten ermöglichen
- Kochen zu einem eigenen Gameplay-System machen
- Gruppen und Basen einen Grund für Küchen und Lebensmittelproduktion geben
- hochwertige Gerichte gegenüber einfachem gebratenem Fleisch belohnen
- langfristig problemlos um weitere Rezepte erweiterbar bleiben

---

# 3. Eigene 3D-Modelle

ChefZ soll eigene Modelle enthalten.

## 3.1 Neue Rohstoffe und Lebensmittel

Beispiele:

- Nudeln
- rohe Nudeln
- getrocknete Nudeln
- Teig
- Mehl
- Hefe
- Weizen
- Milch
- Sahne
- Butter
- Käse
- Wurst
- rohe Wurst
- Hackfleisch
- Salz
- Rohsalz
- Pfefferkörner
- gemahlener Pfeffer
- Paprika
- getrocknete Paprika
- Paprikapulver
- Kräuter
- getrocknete Kräuter

---

## 3.2 Fertige Gerichte

Fertige ChefZ-Gerichte sollen nicht als generische Lebensmittel erscheinen.

Stattdessen erhält jedes Gericht ein eigenes **serviertes 3D-Modell auf einem Teller oder in einer Schüssel**.

Mögliche Präsentation:

- weißer Keramikteller
- leicht beschädigter Keramikteller
- Emailleteller
- Metallteller
- Feldgeschirr
- tiefer Suppenteller
- rustikale Schüssel

Die Optik soll bewusst nicht nach Restaurant aussehen.

Ziel:

> Das Gericht soll aussehen, als hätte ein Survivor es mit den vorhandenen Mitteln gekocht und gerade serviert.

Optional später:

- voller Teller
- halb gegessener Teller
- fast leerer Teller
- leerer Teller

Leere Teller können anschließend erneut verwendet werden.

---

# 4. Grundarchitektur des Rezeptsystems

ChefZ sollte Zutaten zusätzlich in Kategorien einteilen.

Beispiele:

## MEAT

- Schwein
- Wildschwein
- Hirsch
- Reh
- Ziege
- Schaf
- Rind
- Huhn
- Bär
- Wolf

## FISH

- verschiedene Fischarten
- Fischfilets

## VEGETABLE

- Kartoffel
- Tomate
- Paprika
- Zucchini
- Kürbis
- Karotte
- Kohl
- Zwiebel

## MUSHROOM

- verschiedene essbare Pilze

## CARB

- Reis
- Nudeln
- Brot
- Teig
- Kartoffeln

## DAIRY

- Milch
- Sahne
- Butter
- Käse

## HERBS

- Petersilie
- Dill
- Thymian
- Rosmarin
- Bärlauch

## SPICES

- Salz
- Pfeffer
- Paprikapulver

Dadurch müssen nicht für jede Tierart vollständig getrennte Rezepte programmiert werden.

Beispiel:

```text
1x MEAT
1x VEGETABLE
1x RICE
```

kann ein allgemeines Reisgericht erzeugen.

Bestimmte hochwertige Rezepte können trotzdem konkrete Fleischsorten verlangen.

---

# 5. Kochgeräte

ChefZ soll vorhandene DayZ-Kochgeräte nutzen und ergänzen.

| Gerät | Verwendung |
|---|---|
| Frying Pan | Pfannengerichte, Eier, Wurst, Kartoffeln |
| Cooking Pot | Suppen, Eintöpfe, Nudeln, Reis |
| Cauldron | große Portionen und Gruppenessen |
| Stick | simples Fleisch oder Fisch |
| Fireplace | Grundkochen |
| Gas Stove | mobiles Kochen |
| Oven | Brot, Teig, Aufläufe |
| ChefZ Grill | Grillgerichte |
| Smoker | Räuchern |
| Drying Rack | Kräuter, Fleisch, Fisch, Nudeln trocknen |

---

# 6. Neue Produktionsstationen und Werkzeuge

## 6.1 Schneidebrett

Verwendung:

- Gemüse schneiden
- Fleisch vorbereiten
- Kräuter vorbereiten
- Fisch filetieren

---

## 6.2 Fleischwolf

Verwendung:

```text
Raw Meat
→ Minced Meat
```

Benötigt für:

- Würste
- Frikadellen
- Fleischfüllungen
- bestimmte Teigtaschen

---

## 6.3 Mörser und Stößel

Verwendung:

```text
Dried Peppercorns
→ Black Pepper
```

```text
Dried Paprika
→ Paprika Powder
```

```text
Dried Herbs
→ Herb Mix
```

---

## 6.4 Getreidemühle

```text
Wheat
→ Flour
```

---

## 6.5 Nudelholz

Verwendung:

- Teig ausrollen
- Nudelteig
- Teigtaschen
- Fladenbrot
- eventuell Pizza

---

## 6.6 Räucherschrank

Verwendung:

- Wurst
- Fleisch
- Fisch

Erzeugt länger haltbare Lebensmittel.

---

## 6.7 Trockenrahmen

Verwendung:

- Kräuter
- Fleisch
- Fisch
- Nudeln
- eventuell Paprika

---

## 6.8 Butterfass

```text
Milk / Cream
→ Butter
```

---

## 6.9 Käsepresse

```text
Milk
→ Cheese
```

Kann später mehrere Käsesorten unterstützen.

---

# 7. Landwirtschaft und neue Pflanzen

ChefZ soll Lebensmittel nicht nur aus Loot erzeugen.

Ein Teil der Zutaten soll aktiv angebaut oder gesammelt werden.

---

# 8. Kräutersystem

Auf der Karte sollen Kräuter natürlich wachsen.

Geplant sind zunächst fünf Sorten.

## 8.1 Petersilie

Vorkommen:

- Dörfer
- Gärten
- Felder

Verwendung:

- Suppen
- Kartoffeln
- Fisch
- Pasta
- Rahmgerichte

Seltenheit:

**häufig**

---

## 8.2 Dill

Vorkommen:

- Gewässer
- feuchte Wiesen
- Gärten

Verwendung:

- Fisch
- Kartoffeln
- Suppen
- eingelegte Lebensmittel

Seltenheit:

**häufig bis mittel**

---

## 8.3 Thymian

Vorkommen:

- Wiesen
- Waldränder
- trockene Gebiete

Verwendung:

- Wildfleisch
- Wurst
- Eintöpfe
- Pilzgerichte

Seltenheit:

**mittel**

---

## 8.4 Rosmarin

Vorkommen:

- verlassene Gärten
- Gewächshäuser
- sonnige Bereiche

Verwendung:

- Fleisch
- Kartoffeln
- Brot
- hochwertige Gerichte

Seltenheit:

**selten**

---

## 8.5 Bärlauch

Vorkommen:

- feuchte Wälder
- schattige Waldgebiete

Verwendung:

- Wurst
- Fleisch
- Suppen
- Kräutermischungen

Seltenheit:

**mittel**

---

# 9. Frische und getrocknete Kräuter

Kräuter können frisch verwendet werden.

Zusätzlich:

```text
Fresh Herb
→ Drying Rack
→ Dried Herb
```

Vorteile getrockneter Kräuter:

- deutlich längere Haltbarkeit
- für Gewürzmischungen nutzbar
- für hochwertige Rezepte nutzbar
- leichter lagerbar

---

# 10. Pfeffer

Pfeffer wird als zusätzliches Gewächs integriert.

Da echter Pfeffer klimatisch nicht typisch für Chernarus wäre, sollte er seltener und gezielt vorkommen.

Mögliche Spawnorte:

- Gewächshäuser
- Gärtnereien
- besondere Farmgebäude
- seltene Pflanzenspawns

Produktionskette:

```text
Pepper Plant
→ Pepper Berries
→ Drying
→ Dried Peppercorns
→ Mortar & Pestle
→ Black Pepper
```

Pfeffer ist ein hochwertigeres Gewürz.

---

# 11. Paprika und Paprikapulver

Paprika wird als anbaubares Gemüse integriert.

Direkte Verwendung:

```text
Paprika
→ Raw Food / Cooking Ingredient
```

Gewürzherstellung:

```text
Paprika
→ Drying Rack
→ Dried Paprika
→ Mortar & Pestle
→ Paprika Powder
```

Paprikapulver wird unter anderem benötigt für:

- Würste
- Gulasch
- Chili
- Eintöpfe
- Fleischgerichte
- Gewürzmischungen

---

# 12. Salzherstellung

Salz soll nicht ausschließlich als Loot existieren.

Spieler können Salz selbst aus Salzwasser herstellen.

Grundidee:

```text
Saltwater
→ Cooking Pot
→ Boiling
→ Water evaporates
→ Raw Salt
```

Danach:

```text
Raw Salt
→ Drying / Processing
→ Salt
```

Damit wird die Küste wirtschaftlich interessanter.

Salz wird zu einem zentralen ChefZ-Rohstoff.

Verwendung:

- Würzen
- Wurstherstellung
- Pökeln
- Fisch konservieren
- Fleisch konservieren
- Brot
- Teig
- Pasta
- Suppen
- Eintöpfe
- Gemüsekonservierung

---

# 13. Fleischverarbeitung

ChefZ erweitert Jagd deutlich.

Ein Tier sollte nach Möglichkeit nicht ausschließlich Fleisch liefern.

Langfristige Zielstruktur:

```text
Animal
↓
Butchering
↓
Meat
Fat
Bones
Intestines / Sausage Casing
```

---

## 13.1 Fleisch

Kann verwendet werden als:

- Steak
- Fleischstücke
- Hackfleisch
- Suppenfleisch
- Wurstbasis
- Trockenfleisch
- Räucherfleisch

---

## 13.2 Tierfett

Verwendung:

```text
Animal Fat
→ Cooking Fat
```

Kann für Pfannengerichte genutzt werden.

---

## 13.3 Knochen

```text
Bones + Water + Herbs
→ Bone Broth
```

Knochenbrühe dient als Basis für hochwertige Suppen und Eintöpfe.

---

## 13.4 Därme / Wursthülle

Kann als natürliche Wursthülle verwendet werden.

```text
Intestines
→ Clean / Process
→ Sausage Casing
```

---

# 14. Hackfleisch

```text
Raw Meat
+ Meat Grinder
→ Minced Meat
```

Hackfleisch kann Grundlage sein für:

- Wurst
- Frikadellen
- gefüllte Teigtaschen
- Hackfleischgerichte
- eventuell Fleischsoßen

---

# 15. Wurstsystem

Die Wurstproduktion ist ein eigener ChefZ-Produktionszweig.

Grundkette:

```text
Raw Meat
↓
Meat Grinder
↓
Minced Meat
↓
Seasoning
↓
Sausage Casing
↓
Raw Sausage
```

Danach unterschiedliche Verarbeitung.

```text
Raw Sausage + Pan
→ Fried Sausage
```

```text
Raw Sausage + Cooking Pot
→ Boiled Sausage
```

```text
Raw Sausage + Smoker
→ Smoked Sausage
```

```text
Raw Sausage + Drying Rack
→ Dry Sausage
```

---

# 16. Geplante Wurstsorten

## Simple Sausage

Zutaten:

- beliebiges Fleisch
- Salz
- Wursthülle

---

## Pork Sausage

Zutaten:

- Schweinefleisch
- Salz
- Pfeffer

---

## Venison Sausage

Zutaten:

- Hirsch-/Rehfleisch
- Salz
- Thymian
- Pfeffer

---

## Boar Sausage

Zutaten:

- Wildschwein
- Salz
- Pfeffer
- Bärlauch

---

## Bear Sausage

Zutaten:

- Bärenfleisch
- Salz
- Kräuter

Soll besonders gründlich gegart werden.

---

## Wolf Sausage

Zutaten:

- Wolfsfleisch
- Salz
- Pfeffer

Eher als extremes Survival-Rezept.

---

## Chicken Sausage

Zutaten:

- Geflügel
- Salz
- Kräuter

---

## Mixed Meat Sausage

Zutaten:

- zwei Fleischsorten
- Salz
- Pfeffer

---

## Garlic / Wild Garlic Sausage

Zutaten:

- Fleisch
- Salz
- Bärlauch
- Pfeffer

---

## Herb Sausage

Zutaten:

- Fleisch
- Salz
- Thymian
- Petersilie

---

## Spicy Sausage

Zutaten:

- Fleisch
- Salz
- Paprikapulver
- Pfeffer

---

## Smoked Sausage

Herstellung:

```text
Raw Sausage
→ Smoker
→ Smoked Sausage
```

---

## Dry Sausage

Herstellung:

```text
Raw Sausage
→ Drying Rack
→ Dry Sausage
```

---

## Blood Sausage

Mögliche Zutaten:

- Schwein oder Wildschwein
- Blut
- Mehl oder Getreide
- Salz
- Pfeffer

---

## Hunter Sausage

Hochwertige Wildwurst.

Zutaten:

- Wildfleisch
- Salz
- Thymian
- Bärlauch
- Pfeffer
- eventuell Paprikapulver

---

# 17. Wurst als Weiterverarbeitungsprodukt

Wurst soll nicht nur direkt gegessen werden.

Sie dient zusätzlich als Zutat für:

- Bratwurst mit Kartoffeln
- Wurst-Nudel-Pfanne
- Bauernfrühstück
- Bohnen mit Wurst
- Wurstbrot
- Wurst-Käse-Platte
- Wurstgulasch
- Räucherwursteintopf
- Spicy Sausage Pasta

---

# 18. Weizen-, Mehl- und Teigsystem

ChefZ soll eine eigene Getreidekette besitzen.

```text
Wheat
→ Grain Mill
→ Flour
```

Danach:

```text
Flour + Water
→ Simple Dough
```

oder:

```text
Flour + Water + Yeast
→ Yeast Dough
```

oder:

```text
Flour + Egg + Water
→ Pasta Dough
```

---

# 19. Teigprodukte

Aus Teig können entstehen:

- Brot
- Fladenbrot
- Brötchen
- Kräuterbrot
- Käsebrot
- Nudeln
- Teigtaschen
- eventuell Pizza
- eventuell Kuchen oder süßes Gebäck

---

# 20. Nudelherstellung

Produktionskette:

```text
Flour
+ Egg / Water
→ Pasta Dough
```

```text
Pasta Dough
→ Rolling / Cutting
→ Raw Pasta
```

Optional:

```text
Raw Pasta
→ Drying Rack
→ Dried Pasta
```

Getrocknete Pasta:

- lange haltbar
- gut transportierbar
- wichtiger Vorratsartikel

---

# 21. Milchverarbeitung

Milch wird zu einer wichtigen ChefZ-Ressource.

Mögliche Produktionsketten:

```text
Milk
→ Cream
```

```text
Cream
→ Butter Churn
→ Butter
```

```text
Milk
→ Cheese Processing
→ Cheese
```

Mögliche Produkte:

- Milch
- Sahne
- Butter
- Frischkäse
- Käse

Später sind mehrere Käsesorten möglich.

---

# 22. Eier

Eier wären für ChefZ sehr sinnvoll.

Mögliche Quellen:

- Hühner
- Hühnernester
- Farmgebäude

Verwendung:

- Rührei
- Omelett
- Nudelteig
- Backwaren
- Bauernfrühstück
- Frühstücksteller

---

# 23. Weitere sinnvolle Pflanzen

Neben Kräutern, Pfeffer und Paprika sind folgende Pflanzen besonders sinnvoll:

- Zwiebel
- Knoblauch
- Karotte
- Kohl
- Sonnenblume

Optional später:

- Gurke
- Zuckerrübe

---

# 24. Zwiebel

Sehr häufige Basiszutat für:

- Suppen
- Eintöpfe
- Würste
- Fleisch
- Kartoffelgerichte
- Soßen

---

# 25. Knoblauch

Verwendung:

- Würste
- Marinaden
- Fleisch
- Kräutermischungen
- Suppen

---

# 26. Karotten

Verwendung:

- Suppen
- Eintöpfe
- Gemüsegerichte
- Brühen

---

# 27. Kohl

Verwendung:

- Eintopf
- Suppen
- Sauerkraut
- Beilagen

---

# 28. Sonnenblumen

Mögliche Verarbeitung:

```text
Sunflower
→ Seeds
→ Processing
→ Sunflower Oil
```

Dadurch bekommt ChefZ ein selbst produzierbares Pflanzenöl.

---

# 29. Öl und Fett

Mögliche Kochfette:

- Animal Fat
- Butter
- Sunflower Oil
- eventuell gefundenes Cooking Oil

Diese können je nach Rezept austauschbar oder verpflichtend sein.

---

# 30. Fischverarbeitung

Fisch soll ebenfalls mehrere Verarbeitungsschritte erhalten.

```text
Fish
→ Knife
→ Fish Fillet
```

Weitere Optionen:

```text
Fish + Salt
→ Salted Fish
```

```text
Salted Fish
→ Drying Rack
→ Dried Fish
```

```text
Fish
→ Smoker
→ Smoked Fish
```

Damit wird Fischfang deutlich wertvoller.

---

# 31. Konservierungssystem

ChefZ soll mehrere Methoden der Lebensmittelkonservierung unterstützen.

---

## 31.1 Salzen / Pökeln

```text
Raw Meat + Salt
→ Salted Meat
```

---

## 31.2 Trocknen

```text
Salted Meat
→ Drying Rack
→ Dried Meat
```

---

## 31.3 Räuchern

```text
Meat
→ Smoker
→ Smoked Meat
```

---

## 31.4 Fisch trocknen

```text
Fish + Salt
→ Salted Fish
→ Drying Rack
→ Dried Fish
```

---

## 31.5 Fisch räuchern

```text
Fish
→ Smoker
→ Smoked Fish
```

---

## 31.6 Einlegen

Mögliche Produkte:

- eingelegte Gurken
- eingelegte Paprika
- eingelegte Pilze
- eingelegtes Gemüse

Benötigt:

- Glas
- Salzlake oder Essig

---

## 31.7 Einkochen

Mögliche Produkte:

- Gemüsekonserven
- Suppen
- Eintöpfe
- Fleischgerichte
- Marmelade

---

# 32. Verpackungsmaterialien

In Anlehnung an CookZ können Verpackungen hergestellt werden.

Beispiele:

```text
Metal Sheet + Hacksaw
→ Empty Cans
```

```text
Paper + Paper
→ Cardboard Food Box
```

Zusätzlich für ChefZ:

- Einmachglas
- leere Dose
- Lebensmittelbox
- eventuell Glasflasche

---

# 33. Haltbarkeit

Haltbarkeit soll eine zentrale Gameplay-Rolle bekommen.

| Zustand | Haltbarkeit | Eigenschaften |
|---|---:|---|
| Frisch | niedrig | beste Frische |
| Gekocht | mittel | guter Standard |
| Gesalzen | hoch | konserviert |
| Geräuchert | sehr hoch | guter Reiseproviant |
| Getrocknet | sehr hoch | wenig Wasseranteil |
| Eingekocht | extrem hoch | ideal für Vorräte |

Dadurch gibt es kein einzelnes „bestes“ Lebensmittel.

Beispiel:

Eine Base-Gruppe kocht einen großen Eintopf.

Ein Spieler auf einer langen Reise nimmt dagegen:

- Trockenwurst
- Brot
- Trockenfisch

---

# 34. Brühen und Soßen

ChefZ kann durch Zwischenprodukte noch deutlich mehr Rezeptvielfalt erzeugen.

## Knochenbrühe

```text
Bones
+ Water
+ Herbs
→ Bone Broth
```

---

## Tomatensoße

```text
Tomatoes
→ Cooking
→ Tomato Sauce
```

---

## Rahmsoße

```text
Milk / Cream
+ Butter
→ Cream Sauce
```

---

## Pilzrahmsoße

```text
Mushrooms
+ Cream
+ Parsley
→ Mushroom Cream Sauce
```

---

# 35. Gewürzmischungen

Mehrere Gewürze können kombiniert werden.

Beispiel:

```text
Salt
+ Black Pepper
+ Paprika Powder
+ Thyme
→ Hunter Seasoning
```

Weitere mögliche Mischungen:

- Fish Seasoning
- Meat Seasoning
- Herb Mix
- Spicy Mix

Diese Mischungen können für hochwertige Rezepte benötigt werden.

---

# 36. Marinieren

Fleisch kann vor dem Kochen veredelt werden.

Beispiel:

```text
Raw Meat
+ Oil
+ Salt
+ Herbs
→ Marinated Meat
```

Mariniertes Fleisch kann:

- bessere Nährwerte
- besseren Geschmack
- hochwertigere Gerichte

ermöglichen.

---

# 37. Rezeptklassen

ChefZ-Rezepte werden nach Komplexität gegliedert.

## Basic

- wenig Zutaten
- früh verfügbar
- Survival-Fokus

Beispiele:

- Cooked Rice
- Vegetable Soup
- Meat Soup
- Fried Vegetables
- Baked Potato
- Toast

---

## Standard

- 3–4 Zutaten
- normale Kochgerichte

Beispiele:

- Chicken & Rice
- Hunter Stew
- Fish Stew
- Fried Rice
- Meat & Beans
- Tomato Soup

---

## Advanced

- mehrere Zutaten
- Gewürze
- Verarbeitungsschritte

Beispiele:

- Hunter's Casserole
- Creamy Chicken
- Cheese Potato Bake
- Survival Curry
- Fisherman's Pot
- Meat & Mushroom Pasta

---

## Premium

- seltene Zutaten
- Wildfleisch
- spezielle Gewürze

Beispiele:

- Bear Goulash
- Venison Feast
- Honey Glazed Meat
- Four Cheese Bake
- DME Survivor Feast

---

# 38. 20 geplante Tellergerichte

Die folgenden 20 Gerichte bilden eine gute erste Basis für ChefZ.

---

## 1. Survivor Spaghetti

Zutaten:

- Nudeln
- Tomatensoße
- Salz

Optional:

- Pfeffer
- Petersilie

3D-Modell:

Rustikale Spaghetti mit roter Tomatensoße auf Keramikteller.

---

## 2. Wurst-Nudeln-Pfanne

Zutaten:

- Nudeln
- Wurst
- Öl oder Fett
- Salz

Optional:

- Paprikapulver
- Pfeffer

---

## 3. Jägernudeln

Zutaten:

- Nudeln
- Wildfleisch
- Pilze
- Thymian

Optional:

- Sahne

---

## 4. Rahm-Pilz-Nudeln

Zutaten:

- Nudeln
- Pilze
- Milch oder Sahne
- Petersilie
- Salz

---

## 5. Chernarus Mac & Cheese

Zutaten:

- Nudeln
- Milch
- Käse
- Butter

---

## 6. Kartoffeln mit Bratwurst

Zutaten:

- Kartoffeln
- Wurst
- Fett oder Öl
- Salz

Optional:

- Rosmarin

---

## 7. Jägerteller

Zutaten:

- Wildfleisch
- Kartoffeln
- Pilze
- Thymian

---

## 8. Gebratene Blutwurstplatte

Zutaten:

- Blutwurst
- Kartoffeln
- Zwiebel

---

## 9. Fisch mit Kartoffeln

Zutaten:

- Fischfilet
- Kartoffeln
- Dill
- Salz

---

## 10. Bohnen-Wurst-Teller

Zutaten:

- Baked Beans
- Wurst
- Zwiebel

Optional:

- Paprikapulver

---

## 11. Tactical Bacon Breakfast

Zutaten:

- Tactical Bacon
- Ei
- Brot

Optional:

- Tomate

---

## 12. Rührei mit Wurst

Zutaten:

- Ei
- Milch
- Wurst
- Salz

Optional:

- Petersilie

---

## 13. Bauernfrühstück

Zutaten:

- Kartoffeln
- Ei
- Wurst
- Zwiebel

---

## 14. Fladenbrot mit Käse

Zutaten:

- Teig
- Käse
- Salz

Optional:

- Rosmarin

---

## 15. Wurstbrot-Teller

Zutaten:

- Brot
- Wurst
- Käse

Optional:

- Kräuter

---

## 16. Pilzpfanne

Zutaten:

- Pilze
- Öl oder Butter
- Salz
- Petersilie

---

## 17. Kartoffelpuffer

Zutaten:

- Kartoffeln
- Mehl
- Ei
- Öl
- Salz

---

## 18. Fleisch-Teigtaschen

Zutaten:

- Teig
- Hackfleisch
- Zwiebel
- Salz
- Pfeffer

---

## 19. Milchreis

Zutaten:

- Reis
- Milch

Optional:

- Honig
- Zucker

---

## 20. Honigbrot-Platte

Zutaten:

- Brot
- Honig

Optional:

- Butter

---

# 39. Weitere passende Gerichte für spätere Versionen

Mögliche Erweiterungen:

- Bear Goulash
- Wurstgulasch
- Wildschwein-Eintopf
- Fischsuppe
- Kräuterbrot
- Räucherwurst-Eintopf
- Spicy Sausage Pasta
- Hirschragout
- Kartoffelsuppe
- Kohleintopf
- Sauerkraut mit Wurst
- Pilzsuppe
- Knochenbrühe
- Gemüseauflauf
- Käsekartoffeln
- Omelett
- Fischplatte
- Trockenwurstplatte
- Käse-Wurst-Platte

---

# 40. DME Signature Meals

ChefZ sollte eigene Gerichte speziell für Deadmans Echo besitzen.

Beispiele:

## Deadman's Stew

- Wildfleisch
- Kartoffeln
- Pilze
- Tomate
- Thymian

---

## Chernarus Chili

- Fleisch
- Baked Beans
- Paprika
- Tomate
- Paprikapulver
- Pfeffer

---

## Survivor's Breakfast

- Ei
- Fleisch oder Wurst
- Brot
- Tomate

---

## Fisherman's Pot

- Fisch
- Kartoffeln
- Gemüse
- Wasser
- Dill
- Petersilie

---

## Black Forest Stew

- Wildfleisch
- Pilze
- Kartoffeln
- Thymian

---

## Bear Hunter Goulash

- Bärenfleisch
- Paprika
- Tomate
- Kartoffeln
- Paprikapulver
- Pfeffer

---

# 41. Rezeptqualität

Ein Gericht kann verschiedene Qualitätsstufen besitzen.

## Simple

Nur die wichtigsten Grundzutaten.

---

## Prepared

Vollständiges normales Rezept.

---

## Seasoned

Zusätzliche passende Gewürze oder Kräuter.

---

## Premium

Seltene Zutaten, hochwertige Gewürze oder vollständige Rezeptvariante.

Beispiel:

```text
Noodles + Sausage
→ Simple Sausage Pasta
```

```text
Noodles + Sausage + Oil
→ Sausage Pasta
```

```text
Noodles + Sausage + Oil + Pepper + Paprika
→ Seasoned Sausage Pasta
```

```text
Noodles + Hunter Sausage + Mushrooms + Cream + Herbs
→ Premium Hunter Pasta
```

---

# 42. Gerichtsnutzen

Aufwendige Gerichte sollen gegenüber normal gebratenem Fleisch Vorteile haben.

Mögliche Eigenschaften:

## Suppen

- hoher Wasseranteil
- mittlere Energie

## Eintöpfe

- gute Hydration
- hohe Energie

## Pasta und Reisgerichte

- sehr hohe Energie

## Fleischgerichte

- hohe Sättigung

## Fischgerichte

- ausgewogene Werte

## Gemüsegerichte

- gute Gesundheits-/Vitaminwerte

## Premiumgerichte

- hohe Energie
- hohe Sättigung
- kleine Zusatzboni

---

# 43. Optionale Food-Buffs

ChefZ kann kleine temporäre Survival-Buffs verwenden.

Die Boni sollten bewusst moderat bleiben.

Mögliche Effekte:

## Hearty Meal

Große Fleischgerichte.

Effekt:

- Energie sinkt langsamer

---

## Hydrated

Suppen und Eintöpfe.

Effekt:

- Wasser sinkt langsamer

---

## Warm Meal

Heiße Gerichte.

Effekt:

- verbesserter Wärmeerhalt

---

## Energized

Honig- oder Zuckergerichte.

Effekt:

- kurzfristig bessere Ausdauerregeneration

---

## Healthy Meal

Gemüse- und Kräutergerichte.

Effekt:

- kleiner Immunitätsbonus

---

## Hunter's Meal

Wildgerichte.

Effekt:

- lange Sättigung

---

# 44. Große Gruppenrezepte

Der Cauldron kann große Portionen ermöglichen.

Beispiel:

```text
4x Meat
4x Potato
2x Tomato
Water
Herbs
```

Ergebnis:

```text
8x Hunter Stew Portion
```

Dadurch kann eine Gruppe gemeinsam für mehrere Spieler kochen.

---

# 45. Experimentelles Kochen

Spieler sollen Rezepte nicht zwingend vorher kennen müssen.

Wenn eine gültige Kombination vorhanden ist:

```text
Ingredients
→ Recipe Found
→ Proper Dish
```

Wenn kein exaktes Rezept existiert, aber die Kombination essbar ist:

```text
Ingredients
→ Improvised Stew
```

Für fragwürdige Kombinationen:

```text
Questionable Stew
```

oder:

```text
Mystery Stew
```

Das passt sehr gut zum DayZ-Survival-Stil.

---

# 46. Rezeptbuch und Entdeckung

Langfristig kann ChefZ ein eigenes Rezeptbuch besitzen.

Spieler kennen am Anfang nur Grundrezepte.

Weitere Rezepte werden gefunden.

Beispiele:

- ChefZ Recipe: Hunter Stew
- ChefZ Recipe: Fisherman's Pot
- ChefZ Recipe: Bear Goulash
- ChefZ Recipe: Hunter Sausage
- ChefZ Recipe: Creamy Mushroom Pasta

Interaktion:

```text
Learn Recipe
```

Danach wird das Rezept dauerhaft freigeschaltet.

---

# 47. Rezept-Raritäten

Mögliche Einteilung:

## Common

- einfache Suppen
- Reis
- Grundgerichte

## Uncommon

- Chili
- Pasta
- Eintöpfe
- bessere Würste

## Rare

- Wildgerichte
- Hunter Sausage
- Fisherman's Pot

## Epic

- DME Signature Meals
- hochwertige Jagdgerichte

## Legendary

- Eventgerichte
- Boss-/Questrezepte
- Season-Rezepte

---

# 48. ChefZ Cookbook UI

Spätere Erweiterung:

```text
CHEFZ
────────────────────

Soups        7 / 12
Stews        4 / 15
Meat         8 / 18
Fish         3 / 10
Pasta        6 / 14
Vegetarian   2 / 12
Sausages     5 / 15
Special      0 / 8
```

Ein Rezept zeigt:

- Name
- Zutaten
- optionale Zutaten
- Kochgerät
- benötigte Station
- Kochzeit
- Rezeptstatus
- Qualität
- mögliche Buffs

---

# 49. Kern-Gameplay-Loops

## Landwirtschaft

```text
Seed
↓
Plant
↓
Grow
↓
Harvest
↓
Ingredient
```

---

## Kräuter

```text
Find Herb
↓
Harvest
↓
Fresh Herb
↓
Dry
↓
Dried Herb
↓
Seasoning / Recipe
```

---

## Salz

```text
Saltwater
↓
Boil
↓
Raw Salt
↓
Dry
↓
Salt
```

---

## Wurst

```text
Animal
↓
Butcher
↓
Meat
↓
Grind
↓
Minced Meat
↓
Season
↓
Sausage Casing
↓
Raw Sausage
↓
Cook / Smoke / Dry
```

---

## Brot

```text
Wheat
↓
Mill
↓
Flour
↓
Water + Yeast
↓
Dough
↓
Bake
↓
Bread
```

---

## Pasta

```text
Wheat
↓
Flour
↓
Egg / Water
↓
Pasta Dough
↓
Cut
↓
Raw Pasta
↓
Dry or Cook
```

---

## Fisch

```text
Catch Fish
↓
Fillet
↓
Cook
or
Salt
↓
Dry / Smoke
```

---

# 50. ChefZ Gameplay-Säulen

ChefZ wird in fünf Hauptbereiche gegliedert.

## 1. Landwirtschaft und Sammeln

- Weizen
- Gemüse
- Kräuter
- Pfeffer
- Paprika

## 2. Verarbeitung

- mahlen
- schneiden
- wolfen
- trocknen
- würzen
- Teig herstellen
- Wurst herstellen

## 3. Kochen

- Pfanne
- Topf
- Kessel
- Ofen
- Grill

## 4. Konservieren

- salzen
- pökeln
- trocknen
- räuchern
- einlegen
- einkochen

## 5. Servieren

- fertige Tellergerichte
- Schüsseln
- Portionsgerichte
- wiederverwendbares Geschirr

---

# 51. Empfohlener Umfang für ChefZ V1

ChefZ sollte nicht sofort mit mehreren hundert Rezepten starten.

Eine sinnvolle V1 könnte enthalten:

## Pflanzen

- Weizen
- Paprika
- Pfeffer
- Zwiebel
- Knoblauch
- Karotte
- Kohl

## Kräuter

- Petersilie
- Dill
- Thymian
- Rosmarin
- Bärlauch

## Basiszutaten

- Mehl
- Hefe
- Teig
- Nudeln
- Milch
- Butter
- Käse
- Salz
- Pfeffer
- Paprikapulver
- Hackfleisch
- Wursthülle

## Verarbeitung

- Getreidemühle
- Fleischwolf
- Mörser
- Trockenrahmen
- Räucherschrank

## Wurst

mindestens:

- Simple Sausage
- Pork Sausage
- Venison Sausage
- Boar Sausage
- Hunter Sausage
- Spicy Sausage
- Smoked Sausage
- Dry Sausage

## Tellergerichte

zunächst die 20 definierten Gerichte.

---

# 52. Empfohlene Erweiterungsstufen

## ChefZ V1 – Cooking Core

- Zutaten
- 20 Tellergerichte
- Grundrezepte
- Kräuter
- Salz
- Pfeffer
- Paprikapulver
- Wurst
- Fleischwolf
- Mörser

---

## ChefZ V2 – Production

- Weizenproduktion
- Mehl
- Teig
- Pasta
- Brot
- Milchverarbeitung
- Butter
- Käse
- Räuchern
- Trocknen

---

## ChefZ V3 – Preservation

- Einmachgläser
- Einkochen
- Einlegen
- Pökeln
- komplexere Haltbarkeit
- größere Vorratswirtschaft

---

## ChefZ V4 – Progression

- Rezeptbuch
- Cookbook UI
- seltene Rezepte
- Rezept-Raritäten
- DME Signature Meals
- Quest-/Eventrezepte

---

# 53. Technische Namenskonvention

Eigene Klassen sollten einheitlich benannt werden.

Beispiele:

```text
ChefZ_Wheat
ChefZ_Flour
ChefZ_Yeast
ChefZ_Dough
ChefZ_PastaDough
ChefZ_RawPasta
ChefZ_DriedPasta

ChefZ_Milk
ChefZ_Cream
ChefZ_Butter

ChefZ_RawSalt
ChefZ_Salt

ChefZ_PepperBerries
ChefZ_DriedPeppercorns
ChefZ_BlackPepper

ChefZ_Paprika
ChefZ_DriedPaprika
ChefZ_PaprikaPowder

ChefZ_MincedMeat
ChefZ_SausageCasing
ChefZ_RawSausage
ChefZ_PorkSausage
ChefZ_VenisonSausage
ChefZ_BoarSausage
ChefZ_HunterSausage
ChefZ_SpicySausage
ChefZ_SmokedSausage
ChefZ_DrySausage
```

Gerichte:

```text
ChefZ_SurvivorSpaghetti
ChefZ_SausagePasta
ChefZ_HunterPasta
ChefZ_CreamMushroomPasta
ChefZ_MacAndCheese
ChefZ_SausagePotatoes
ChefZ_HunterPlate
ChefZ_BloodSausagePlate
ChefZ_FishPotatoPlate
ChefZ_BeanSausagePlate
ChefZ_TacticalBreakfast
ChefZ_ScrambledEggSausage
ChefZ_FarmersBreakfast
ChefZ_CheeseFlatbread
ChefZ_SausageBreadPlate
ChefZ_MushroomPan
ChefZ_PotatoPancakes
ChefZ_MeatDumplings
ChefZ_MilkRice
ChefZ_HoneyBreadPlate
```

---

# 54. Designprinzip

ChefZ soll nicht das normale DayZ-Kochen ersetzen.

Es soll es erweitern.

Grundregel:

> Wer nur Fleisch braten möchte, kann weiterhin Fleisch braten.

ChefZ belohnt Spieler, die mehr Aufwand investieren.

Beispiel:

```text
Raw Meat
→ Cooked Meat
```

ist schnell und einfach.

Dagegen:

```text
Animal
→ Butcher
→ Meat
→ Grind
→ Minced Meat
→ Season
→ Sausage
→ Smoke
→ Combine with Potatoes
→ Served Dish
```

ist deutlich aufwendiger, erzeugt aber:

- bessere Nahrung
- längere Haltbarkeit
- bessere Vorräte
- eventuell kleine Buffs
- mehr Handelswert
- mehr Immersion

---

# 55. Gesamtvision

ChefZ entwickelt Lebensmittel in DME von einem einfachen Verbrauchsgegenstand zu einem vollständigen Survival-System.

Der Spieler entscheidet:

- Esse ich mein Fleisch sofort?
- Verarbeite ich es zu Wurst?
- Räuchere ich die Wurst?
- Nutze ich sie später für ein hochwertiges Gericht?
- Sammle ich Kräuter?
- Produziere ich Salz?
- Baue ich Weizen an?
- Stelle ich eigene Nudeln her?
- Produziere ich Vorräte für meine Gruppe?
- Koche ich ein frisches Essen oder nehme ich haltbaren Reiseproviant?

Die gesamte Gameplay-Kette lautet:

```text
JAGEN / ANGELN / FARMEN / SAMMELN
↓
ROHSTOFFE
↓
VERARBEITUNG
↓
GEWÜRZE / KRÄUTER
↓
KOCHEN
↓
KONSERVIERUNG
↓
SERVIEREN
↓
ESSEN / LAGERN / HANDELN
```

ChefZ soll damit weit über einen reinen Recipe-Mod hinausgehen und zu einem eigenständigen DME-Survival-Feature werden.
