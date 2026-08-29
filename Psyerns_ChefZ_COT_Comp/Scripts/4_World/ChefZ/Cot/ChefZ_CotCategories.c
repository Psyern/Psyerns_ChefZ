// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn Community Online Tools
// geladen ist. Fehlt der Mod, ist JM_COT nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef JM_COT
// ChefZ_CotCategories - die acht ChefZ-Spawnkategorien fuer COTs Object Spawner.
//
// Diese Datei ist eine TABELLE und sonst nichts: acht Namen, acht Klassenlisten.
// Kein Zugriff auf Mission, GUI oder Engine-Typen, keine Spielmechanik. Genau
// deshalb liegt sie in 4_World und nicht neben der modded-Klasse in 5_Mission -
// wer wissen will, WAS ChefZ an COT meldet, liest diese Datei; wer wissen will,
// WIE es gemeldet wird, die andere.
//
// ---------------------------------------------------------------------------
// WOHER DIE KLASSENNAMEN STAMMEN
// ---------------------------------------------------------------------------
// Aus den config.cpp unter Psyerns_ChefZ_Core/Addons/ - und nur von dort. Jeder
// Name unten ist eine Klasse mit scope = 2, die dort mit Rumpf definiert ist.
// Gegengeprueft gegen die classes-Listen in Psyerns_ChefZ_Core/_deltas/*.json:
// beide Quellen nennen dieselben 185 Klassen, davon 168 mit scope = 2. Diese
// 168 stehen hier, jede in GENAU EINER Kategorie.
//
// Die 17 fehlenden sind die Basisklassen mit scope = 0 (ChefZ_GrainFoodBase,
// ChefZ_MeatItemBase, ChefZ_ServedDish_Base ...). Sie fehlen bewusst: sie sind
// nicht spawnbar. COT wuerde sie ohnehin verwerfen (JMObjectSpawnerForm.c:940 -
// "scope == 0" -> continue); sie hier zu fuehren hiesse, dem Admin siebzehn
// tote Zeilen anzubieten.
//
// ---------------------------------------------------------------------------
// WARUM KLASSENLISTEN UND NICHT BASISKLASSEN
// ---------------------------------------------------------------------------
// COTs eigener Typfilter ist ein einzelner Basisklassenname und arbeitet mit
// g_Game.IsKindOf (JMObjectSpawnerForm.c:954). Fuer ChefZ traegt das nicht:
//
//   Milchprodukte  ChefZ_Cream erbt Marmalade,
//                  ChefZ_Butter Lard, ChefZ_Cheese BoxCerealCrunchin - vier
//                  Waren, vier voellig verschiedene Vanilla-Aeste. Es gibt
//                  keine gemeinsame Basis, ueber die IsKindOf sie einsammeln
//                  koennte.
//   Stationen      ChefZ_ButterChurn erbt Pot, ChefZ_CheesePress Cauldron,
//                  der Rest Inventory_Base. Dasselbe Bild.
//   Kraeuter       verteilen sich ueber drei Basen (ChefZ_FreshHerbBase,
//                  ChefZ_DriedHerbBase, ChefZ_SpiceBase).
//
// Die Alternative waere gewesen, die Vererbung der Items umzubauen, damit ein
// Adminfilter huebsch wird. Das waere eine Aenderung an der Spielmechanik fuer
// ein Werkzeug - und dieses Modul aendert keine Spielmechanik. Also: Listen.
//
// ---------------------------------------------------------------------------
// FEHLT EIN ADDON, FEHLEN SEINE EINTRAEGE - MEHR NICHT
// ---------------------------------------------------------------------------
// Diese Datei prueft NICHTS. Die Pruefung, ob eine Klasse zur Laufzeit
// ueberhaupt existiert, macht ChefZ_CotObjectSpawner.c mit
// g_Game.ConfigIsExisting, direkt bevor ein Eintrag in die Liste geht. Ein
// Name, den es nicht gibt, wird dort still uebersprungen. Deshalb darf hier
// ruhig die volle Liste stehen, auch wenn ein Server nur einen Teil der
// ChefZ-Addons laedt.
//
// ---------------------------------------------------------------------------
// EINE NEUE KLASSE NACHTRAGEN
// ---------------------------------------------------------------------------
// Nur, wenn sie in einer config.cpp unter Psyerns_ChefZ_Core/Addons/ mit
// scope = 2 wirklich existiert. Ein geratener Name faellt hier nicht auf - er
// wird zur Laufzeit lautlos verworfen, und der Admin sucht ein Item, das es nie
// gab. Ein Eintrag in requiredAddons[] ist dafuer NICHT noetig und auch nicht
// erwuenscht: dort steht seit dem Umbau auf weiche Abhaengigkeiten nur noch
// ChefZ_Core, damit ein fehlendes ChefZ-Addon genau die oben beschriebene
// lautlose Nachsicht ausloest und nicht den Start des ganzen PBOs verhindert
// (Begruendung im Kopf der config.cpp, Abschnitt "requiredAddons[]").

/**
 * Eine Kategorie: Anzeigename plus die Klassennamen, die sie fuehrt.
 */
class ChefZ_CotCategory : Managed
{
	protected string m_FilterId;
	protected string m_Label;
	protected ref array<string> m_Classes;

	void ChefZ_CotCategory(string filterId, string label, array<string> classes)
	{
		m_FilterId = filterId;
		m_Label = label;
		m_Classes = new array<string>;

		if (classes)
		{
			m_Classes.Copy(classes);
		}
	}

	/**
	 * Der Wert, den JMObjectSpawnerModule.m_CurrentType traegt, solange diese
	 * Kategorie gewaehlt ist. Bewusst kleingeschrieben und mit dem Praefix
	 * "chefz_cot_": COTs eigener Zweig reicht m_CurrentType an g_Game.IsKindOf
	 * weiter, und kein Config-Klassenname sieht so aus. Selbst wenn dieser Wert
	 * also einmal am ChefZ-Zweig vorbeilaeuft, liefert er eine leere Liste
	 * statt eines falschen Treffers.
	 */
	string GetFilterId()
	{
		return m_FilterId;
	}

	/** Stringtable-Schluessel des Anzeigenamens. */
	string GetLabel()
	{
		return m_Label;
	}

	/** Die gefuehrten Klassennamen. Ungeprueft - siehe Kopf der Datei. */
	array<string> GetClasses()
	{
		return m_Classes;
	}
}

/**
 * Die Tabelle selbst. Statisch und einmalig aufgebaut: die Liste ist konstant,
 * und der Object Spawner fragt sie bei jedem Tastendruck im Suchfeld erneut ab.
 */
class ChefZ_CotCategories
{
	protected static ref array<ref ChefZ_CotCategory> s_Categories;

	/** Alle Kategorien in Anzeigereihenfolge. */
	static array<ref ChefZ_CotCategory> Get()
	{
		if (!s_Categories)
		{
			s_Categories = new array<ref ChefZ_CotCategory>;
			Build();
		}

		return s_Categories;
	}

	/**
	 * Die Kategorie zu einer FilterId - oder NULL.
	 *
	 * NULL ist die Antwort fuer jeden Wert, der nicht von hier stammt: der
	 * leere Text (COTs "Alle"), "edible_base", "transport" und alles andere aus
	 * COTs eigener Typleiste. Der Aufrufer nimmt genau das als Signal, COTs
	 * unveraenderten Zweig zu benutzen.
	 */
	static ChefZ_CotCategory Find(string filterId)
	{
		if (filterId == "")
		{
			return NULL;
		}

		foreach (ref ChefZ_CotCategory category : Get())
		{
			if (category.GetFilterId() == filterId)
			{
				return category;
			}
		}

		return NULL;
	}

	/** Position einer FilterId in Get(), oder -1. */
	static int IndexOf(string filterId)
	{
		if (filterId == "")
		{
			return -1;
		}

		array<ref ChefZ_CotCategory> categories = Get();
		for (int i = 0; i < categories.Count(); i++)
		{
			if (categories.Get(i).GetFilterId() == filterId)
			{
				return i;
			}
		}

		return -1;
	}

	protected static void Add(string filterId, string label, array<string> classes)
	{
		s_Categories.Insert(new ChefZ_CotCategory(filterId, label, classes));
	}

	/**
	 * Die acht Kategorien. Reihenfolge = Anzeigereihenfolge in der Auswahlbox.
	 */
	protected static void Build()
	{
		// ChefZ / Zutaten - alles, was als Eingang in ein Rezept geht und in
		// keine der spezielleren Kategorien gehoert: Gemuese samt Saat und
		// Pflanze, Schnittgut vom Brett, Ei, Salz, Weizen und Mehl.
		//
		// Die drei Saucen und die Bruehe stehen hier und NICHT unter
		// "Gerichte". Sie sind laut dem Kopf von ChefZ_Cooking/config.cpp
		// Zutat eines Gerichts, kein Gericht - ein Admin, der Rahmsauce sucht,
		// sucht sie als Zutat.
		//
		// Weizen und Mehl stehen hier und nicht
		// unter "Teig, Brot und Pasta", obwohl sie dieselbe Basisklasse
		// ChefZ_GrainFoodBase teilen: sie sind der Rohstoff der Kette, nicht
		// ihr Erzeugnis.
		Add("chefz_cot_ingredients", "#STR_CHEFZ_COT_CAT_INGREDIENTS",
		{
			"ChefZ_Wheat",
			"ChefZ_Flour", 
			"ChefZ_Onion", 
			"ChefZ_Garlic", 
			"ChefZ_Carrot", 
			"ChefZ_Cabbage", 
			"ChefZ_Egg",
			"ChefZ_RawSalt", "ChefZ_Salt", "ChefZ_BoneBroth",
			"ChefZ_TomatoSauce", "ChefZ_CreamSauce", "ChefZ_MushroomCreamSauce"
		});

		// ChefZ / Kraeuter und Gewuerze - die vollstaendige Kraeuterkette in
		// EINER Kategorie: frisch (Fundpflanze), getrocknet, gemahlen.
		//
		// Bewusst nicht nach Verarbeitungsgrad aufgeteilt. Ein Admin sucht
		// "Thymian", nicht "Thymian, Stufe 2 von 3"; die fuenf Basisklassen
		// dahinter interessieren ihn nicht.
		Add("chefz_cot_herbs", "#STR_CHEFZ_COT_CAT_HERBS",
		{
			"ChefZ_Parsley",
			"ChefZ_Dill", "ChefZ_Thyme", "ChefZ_Rosemary",
			"ChefZ_WildGarlic", "ChefZ_PepperBerries",
			"ChefZ_DriedParsley", "ChefZ_DriedDill", "ChefZ_DriedThyme",
			"ChefZ_DriedRosemary", "ChefZ_DriedWildGarlic", "ChefZ_DriedPaprika",
			"ChefZ_PaprikaPowder", "ChefZ_DriedPeppercorns", "ChefZ_BlackPepper",
			"ChefZ_HerbMix", "ChefZ_HunterSeasoning"
		});

		// ChefZ / Fleisch und Wurst - Hackfleisch, Fett, Darm, rohe und
		// gegarte Wurst.
		//
		// Die acht haltbar gemachten Waren aus ChefZ_Preservation stehen
		// ebenfalls hier, obwohl sie eine eigene Basisklasse
		// (ChefZ_PreservedFood_Base) und ein eigenes Addon haben. Grund: die
		// vereinbarten acht Kategorien kennen keine "Konserven", und
		// Salzfleisch, Doerrfleisch und Dauerwurst sind das, was ein Admin
		// unter Fleisch und Wurst sucht. Eine neunte Kategorie waere eine
		// eigenmaechtige Erweiterung des Auftrags gewesen.
		//
		// Salzfisch, Doerrfisch und Raeucherfisch stehen damit ebenfalls unter
		// "Fleisch und Wurst". Das ist der bewusst in Kauf genommene Preis
		// derselben Entscheidung.
		Add("chefz_cot_meat", "#STR_CHEFZ_COT_CAT_MEAT",
		{
			"ChefZ_MincedMeat", "ChefZ_MincedPork",
			"ChefZ_MincedVenison", "ChefZ_MincedBoar", "ChefZ_MincedChicken",
			"ChefZ_MincedBear", 
			"ChefZ_RawSausage", "ChefZ_RawPorkSausage", "ChefZ_RawVenisonSausage",
			"ChefZ_RawBoarSausage", "ChefZ_RawHunterSausage", "ChefZ_RawSpicySausage",
			"ChefZ_CookedSausage", "ChefZ_PorkSausage", "ChefZ_VenisonSausage",
			"ChefZ_BoarSausage", "ChefZ_HunterSausage", "ChefZ_SpicySausage",
			"ChefZ_SaltedMeat", "ChefZ_DriedMeat", "ChefZ_SmokedMeat",
			"ChefZ_SaltedFish", "ChefZ_DriedFish", "ChefZ_SmokedFish",
			"ChefZ_SmokedSausage", "ChefZ_DrySausage"
		});

		// ChefZ / Teig, Brot und Pasta - das Erzeugnis der Getreidekette.
		//
		// Hefe steht hier: sie kommt aus ChefZ_Baking und existiert
		// ausschliesslich, um Teig gehen zu lassen. Der Rohstoff der Kette
		// (Weizen, Mehl) steht dagegen unter "Zutaten".
		Add("chefz_cot_baking", "#STR_CHEFZ_COT_CAT_BAKING",
		{
			"ChefZ_Dough", "ChefZ_RawPasta", "ChefZ_DriedPasta",
			"ChefZ_Bread", "ChefZ_Flatbread"
		});

		// ChefZ / Milchprodukte - Milch, Rahm, Butter, Kaese.
		//
		// Die kleinste Kategorie, und die, an der COTs Typfilter am
		// deutlichsten scheitert: die vier erben von vier verschiedenen
		// Vanilla-Klassen (PowderedMilk, Marmalade, Lard, BoxCerealCrunchin).
		//
		// ChefZ_Egg gehoert im Slice "dairy" dazu, steht hier aber NICHT: ein
		// Ei ist kein Milchprodukt. Es steht unter "Zutaten".
		//
		// Butterfass und Kaesepresse sind Geraete und stehen unter "Stationen
		// und Werkzeuge", nicht hier.
		Add("chefz_cot_dairy", "#STR_CHEFZ_COT_CAT_DAIRY",
		{
			"ChefZ_Cream", "ChefZ_Butter",
			"ChefZ_Cheese"
		});

		// ChefZ / Stationen und Werkzeuge - alles, was nicht gegessen wird.
		//
		// Die einzige Kategorie ohne ein einziges essbares Item. Sie ist im
		// Alltag die meistgebrauchte: eine Station, die einem Spieler
		// abhandenkommt, ersetzt der Admin - und dafuer muss er sie finden.
		Add("chefz_cot_stations", "#STR_CHEFZ_COT_CAT_STATIONS",
		{
			"ChefZ_GrainMill", "ChefZ_PastaMachine", "ChefZ_Mortar",
			"ChefZ_DryingRack", "ChefZ_ButterChurn", "ChefZ_CheesePress",
			"ChefZ_SaltPan", "ChefZ_MeatGrinder",
			"ChefZ_Smoker"
		});

		// ChefZ / Gerichte - die fertigen Speisen, 25 Stueck in je zwei
		// Formen.
		//
		// Jedes Gericht steht ZWEIMAL: einmal als "...Bulk" (die Menge im
		// Kochgeraet, ChefZ_PortionedDish_Base) und einmal als angerichtete
		// Portion (ChefZ_ServedDish_Base). Das sind verschiedene Items mit
		// verschiedenem Verhalten; wer nur eine Form anboete, laege in der
		// Haelfte der Faelle falsch. Sie stehen paarweise untereinander, damit
		// die Liste sie nebeneinander zeigt.
		Add("chefz_cot_dishes", "#STR_CHEFZ_COT_CAT_DISHES",
		{
			"ChefZ_TacticalBreakfastBulk", "ChefZ_TacticalBreakfast",
			"ChefZ_ScrambledEggSausageBulk", "ChefZ_ScrambledEggSausage",
			"ChefZ_FarmersBreakfastBulk", "ChefZ_FarmersBreakfast",
			"ChefZ_CheeseFlatbreadBulk", "ChefZ_CheeseFlatbread",
			"ChefZ_SausageBreadPlateBulk", "ChefZ_SausageBreadPlate",
			"ChefZ_MushroomPanBulk", "ChefZ_MushroomPan",
			"ChefZ_PotatoPancakesBulk", "ChefZ_PotatoPancakes",
			"ChefZ_MeatDumplingsBulk", "ChefZ_MeatDumplings",
			"ChefZ_MilkRiceBulk", "ChefZ_MilkRice",
			"ChefZ_HoneyBreadPlateBulk", "ChefZ_HoneyBreadPlate",
			"ChefZ_HunterStewBulk", "ChefZ_HunterStewBowl",
			"ChefZ_FishermanStewBulk", "ChefZ_FishermanStewBowl",
			"ChefZ_VegetableSoupBulk", "ChefZ_VegetableSoupBowl",
			"ChefZ_BoneBrothSoupBulk", "ChefZ_BoneBrothSoupBowl",
			"ChefZ_ChernarusChiliBulk", "ChefZ_ChernarusChiliBowl",
			"ChefZ_SurvivorSpaghettiBulk", "ChefZ_SurvivorSpaghetti",
			"ChefZ_SausagePastaBulk", "ChefZ_SausagePasta",
			"ChefZ_HunterPastaBulk", "ChefZ_HunterPasta",
			"ChefZ_CreamMushroomPastaBulk", "ChefZ_CreamMushroomPasta",
			"ChefZ_MacAndCheeseBulk", "ChefZ_MacAndCheese",
			"ChefZ_SausagePotatoesBulk", "ChefZ_SausagePotatoes",
			"ChefZ_HunterPlateBulk", "ChefZ_HunterPlate",
			"ChefZ_BloodSausagePlateBulk", "ChefZ_BloodSausagePlate",
			"ChefZ_FishPotatoPlateBulk", "ChefZ_FishPotatoPlate",
			"ChefZ_BeanSausagePlateBulk", "ChefZ_BeanSausagePlate"
		});

		// ChefZ / Behaelter - die leeren Gefaesse.
		//
		// Getrennt von "Stationen und Werkzeuge", obwohl beides Nicht-Essbares
		// ist: ein Teller ist Verbrauchsgut und wird in Mengen ausgegeben, eine
		// Kaesepresse ist ein Einzelstueck.
		Add("chefz_cot_containers", "#STR_CHEFZ_COT_CAT_CONTAINERS",
		{
			"ChefZ_EmptyPlate", "ChefZ_EmptyBowl", "ChefZ_EmptyCan",
			"ChefZ_EmptyJar", "ChefZ_EmptyBox"
		});
	}
}
#endif // JM_COT
