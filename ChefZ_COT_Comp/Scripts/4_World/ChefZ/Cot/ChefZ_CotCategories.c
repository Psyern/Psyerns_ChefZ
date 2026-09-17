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
// Diese Datei ist eine TABELLE und sonst nichts: acht Namen, acht Klassenlisten,
// acht Symbolnamen. Kein Zugriff auf Mission, GUI oder Engine-Typen, keine
// Spielmechanik. Genau deshalb liegt sie in 4_World und nicht neben der
// modded-Klasse in 5_Mission - wer wissen will, WAS ChefZ an COT meldet, liest
// diese Datei; wer wissen will, WIE es gemeldet wird, die andere.
//
// Der Symbolname ist absichtlich ein nackter Lucide-Name ("carrot") und kein
// fertiger Dateipfad: den Pfad baut erst 5_Mission mit JMConstants.Lucide
// (COT_New/Scripts/3_game/communityonlinetools/jmconstants.c:261). So bleibt
// diese Schicht frei von COT-Bezeichnern und ist reine Zeichenkette.
//
// ---------------------------------------------------------------------------
// WOHER DIE KLASSENNAMEN STAMMEN
// ---------------------------------------------------------------------------
// Aus den config.cpp unter ChefZ_Core/Addons/ - und nur von dort. Jeder Name
// unten ist eine Klasse, die dort mit Rumpf und scope = 2 definiert ist.
//
// NACHGEZAEHLT AM 17.09.2026, maschinell gegen alle
// ChefZ_Core/Addons/*/config.cpp:
//
//   109   Klassennamen stehen hier, jeder in GENAU EINER Kategorie
//   109   davon existieren mit scope = 2 - kein einziger geratener Name
//    96   spawnbare ChefZ-Klassen stehen NICHT hier (Liste unten)
//
// Die 96 fehlen nicht aus Nachlaessigkeit, sondern weil fuer sie keine
// Kategoriezuordnung entschieden ist. Sie still einzusortieren waere geraten.
// Der Stand, damit die Luecke nachweisbar bleibt und nicht neu gezaehlt werden
// muss:
//
//   ChefZ_Fishing (68)      31 Fische, 30 Filets, 7 Koeder - der komplette
//                           Fishing-Slice. Er braucht eine eigene Entscheidung:
//                           "Fisch" passt weder unter Zutaten noch unter die
//                           Sammelkategorie "Fleisch und Wurst", in der heute
//                           nur die drei HALTBAR GEMACHTEN Fischwaren stehen.
//   ChefZ_Farming (17)      Imkerei (Beute, Rahmen, Smoker, Gabel) und die acht
//                           Beeren aus dem Slice "berries", dazu ChefZ_Chili.
//   ChefZ_Meat (4)          ChefZ_BeefLeg, ChefZ_PorkLeg, ChefZ_VenisonLeg,
//                           ChefZ_DicedMeat
//   ChefZ_Ingredients (3)   ChefZ_CheeseCurd, ChefZ_DriedBerries,
//                           ChefZ_MushroomCulture
//   ChefZ_Cooking (2)       ChefZ_PumpkinSoupBowl, ChefZ_SmallFishPan
//   ChefZ_Cookbook (1)      ChefZ_CookbookItem
//   ChefZ_Processing (1)    ChefZ_HoneyExtractor
//
// ---------------------------------------------------------------------------
// WARUM KLASSENLISTEN UND NICHT BASISKLASSEN
// ---------------------------------------------------------------------------
// COTs eigener Typfilter ist ein einzelner Basisklassenname und arbeitet mit
// g_Game.IsKindOf (COT_New/Scripts/5_mission/communityonlinetools/modules/
// object/jmobjectspawnerform.c:2437). Fuer ChefZ traegt das nicht:
//
//   Milchprodukte  ChefZ_Cream erbt Marmalade, ChefZ_Butter Lard,
//                  ChefZ_Cheese BoxCerealCrunchin - drei Waren, drei voellig
//                  verschiedene Vanilla-Aeste. Es gibt keine gemeinsame Basis,
//                  ueber die IsKindOf sie einsammeln koennte.
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
// UNAUFNEHMBARE WELTOBJEKTE - GEPRUEFT, ES AENDERT NICHTS
// ---------------------------------------------------------------------------
// In dieser Tabelle stehen Klassen, die ein Spieler nicht in die Hand nehmen
// kann: ChefZ_WildPlant_Base ueberschreibt IsTakeable, CanPutInCargo,
// CanRemoveFromCargo und CanPutIntoHands mit false
// (ChefZ_Farming/Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c:208-226).
// Die naheliegende Sorge - "COT kann so etwas nicht spawnen" - ist am
// COT_New-Stand nachgeprueft und unbegruendet:
//
//   Liste     jmobjectspawnerform.c:2386-2488 (UpdateList) prueft scope
//             (:2430), Modell (:2434) und IsExcludedClassName (:2439).
//             IsTakeable kommt dort nicht vor. Alle vier Wildpflanzen haben
//             scope = 2 und ein echtes Modell, keines ist "bmp".
//   Spawn     jmobjectspawnermodule.c:655-670 geht fuer einen Spawn an eine
//             Position ueber CreateObjectEx mit ECE_PLACE_ON_SURFACE (:665) -
//             dieselbe Erzeugung, die ChefZ selbst fuer seine Begleitpflanzen
//             benutzt. Kein Inventarweg, kein IsTakeable.
//
// ZWEI DINGE, DIE DER ADMIN WISSEN SOLLTE - beides COTs unveraendertes
// Verhalten, nicht unseres:
//
//   1. "Spawn to inventory" ist fuer diese vier sinnlos. Der Inventarzweig
//      greift nur bei IsInventoryType (jmobjectspawnermodule.c:634, :655) -
//      das trifft zu -, und legt das Objekt dann per FindFreeLocationFor ab.
//      CanPutInCargo/CanPutIntoHands sagen dort nein, COT protokolliert
//      "Couldn't move ..." (:705) und die Pflanze bleibt liegen, wo sie
//      erzeugt wurde. Kein Absturz, nur kein Nutzen.
//   2. Im Setup-Modus CE ruft COT EEOnCECreate() am frisch erzeugten Objekt
//      (jmobjectspawnermodule.c:770). Bei ChefZ_WildCorn stellt genau dieser
//      Haken 0..2 Begleitpflanzen daneben (ChefZ_WildPlants.c:342-346). Ein
//      Admin, der in diesem Modus einmal Mais spawnt, bekommt also unter
//      Umstaenden drei - gewollt und CE-getreu, aber ueberraschend, wenn man
//      es nicht weiss.
//
// Diese Datei aendert daran nichts und soll es auch nicht: sie macht Klassen
// auffindbar, nicht spawnbar.
//
// ---------------------------------------------------------------------------
// EINE NEUE KLASSE NACHTRAGEN
// ---------------------------------------------------------------------------
// Nur, wenn sie in einer config.cpp unter ChefZ_Core/Addons/ mit scope = 2
// wirklich existiert. Ein geratener Name faellt hier nicht auf - er wird zur
// Laufzeit lautlos verworfen, und der Admin sucht ein Item, das es nie gab.
// Ein Eintrag in requiredAddons[] ist dafuer NICHT noetig und auch nicht
// erwuenscht: dort steht seit dem Umbau auf weiche Abhaengigkeiten nur noch
// ChefZ_Core, damit ein fehlendes ChefZ-Addon genau die oben beschriebene
// lautlose Nachsicht ausloest und nicht den Start des ganzen PBOs verhindert
// (Begruendung im Kopf der config.cpp, Abschnitt "requiredAddons[]").
//
// Jede Klassenliste wird in einer EIGENEN Funktion aufgebaut, Zeile fuer Zeile
// mit Insert. Das ist kein Stil, sondern die Auftragsregel "Funktionsaufrufe
// auf EINER Zeile": ein Add(...) mit mehrzeiligem Array-Literal verletzt sie,
// und eine eigene Funktion je Kategorie gibt jeder Liste ausserdem ihren
// eigenen Gueltigkeitsbereich fuer die lokale Variable.

/**
 * Eine Kategorie: Anzeigename, Symbolname und die Klassennamen, die sie fuehrt.
 */
class ChefZ_CotCategory : Managed
{
	protected string m_FilterId;
	protected string m_Label;
	protected string m_IconName;
	protected ref array<string> m_Classes;

	void ChefZ_CotCategory(string filterId, string label, string iconName, array<string> classes)
	{
		m_FilterId = filterId;
		m_Label = label;
		m_IconName = iconName;
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
	 * weiter (jmobjectspawnerform.c:2437), und kein Config-Klassenname sieht so
	 * aus. Selbst wenn dieser Wert also einmal am ChefZ-Zweig vorbeilaeuft,
	 * liefert er eine leere Liste statt eines falschen Treffers.
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

	/** Nackter Lucide-Symbolname, ohne Pfad und ohne Endung. */
	string GetIconName()
	{
		return m_IconName;
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
	 * COTs eigener Kategorientabelle (jmobjectspawnerform.c:527-560). Der
	 * Aufrufer nimmt genau das als Signal, COTs unveraenderten Zweig zu
	 * benutzen.
	 *
	 * Hier stand bis zum 09.09.2026 ein "foreach (ref ChefZ_CotCategory
	 * category : Get())". Die Laufvariable einer Schleife ist eine LOKALE
	 * Variable, und an eine lokale gehoert kein ref. Das Client-Log vom
	 * 09.09.2026 zeigt, was daraus wird: 36 VM-Ausnahmen "NULL pointer to
	 * instance", 34 davon in genau dieser Schleife, zwei in der gleich
	 * gebauten von ChefZ_CotObjectSpawner.OnInit. Der Object Spawner blieb
	 * dabei leer, weil UpdateList nie bis super.UpdateList() kam.
	 * Deshalb: Quelle in eine lokale Variable holen, pruefen, mit einer
	 * gewoehnlichen for-Schleife laufen - so wie IndexOf() es immer schon tat.
	 * Die Validierung faengt den Rueckfall ab (tools/chefz-validate,
	 * enforce.mjs, Regel "foreach-ref").
	 */
	static ChefZ_CotCategory Find(string filterId)
	{
		if (filterId == "")
		{
			return NULL;
		}

		array<ref ChefZ_CotCategory> categories = Get();
		if (!categories)
		{
			return NULL;
		}

		for (int i = 0; i < categories.Count(); i++)
		{
			ChefZ_CotCategory category = categories.Get(i);
			if (category && category.GetFilterId() == filterId)
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
		if (!categories)
		{
			return -1;
		}

		for (int i = 0; i < categories.Count(); i++)
		{
			if (categories.Get(i).GetFilterId() == filterId)
			{
				return i;
			}
		}

		return -1;
	}

	protected static void Add(string filterId, string label, string iconName, array<string> classes)
	{
		s_Categories.Insert(new ChefZ_CotCategory(filterId, label, iconName, classes));
	}

	/**
	 * Die acht Kategorien. Reihenfolge = Reihenfolge im Untermenue der
	 * ChefZ-Gruppe des COT-Kategoriemenues.
	 */
	protected static void Build()
	{
		BuildIngredients();
		BuildHerbs();
		BuildMeat();
		BuildBaking();
		BuildDairy();
		BuildStations();
		BuildDishes();
		BuildContainers();
	}

	// ChefZ / Zutaten - alles, was als Eingang in ein Rezept geht und in keine
	// der spezielleren Kategorien gehoert: Gemuese samt Saat und Pflanze,
	// Schnittgut vom Brett, Ei, Salz, Weizen und Mehl.
	//
	// Die drei Saucen und die Bruehe stehen hier und NICHT unter "Gerichte".
	// Sie sind laut dem Kopf von ChefZ_Cooking/config.cpp Zutat eines Gerichts,
	// kein Gericht - ein Admin, der Rahmsauce sucht, sucht sie als Zutat.
	//
	// Weizen und Mehl stehen hier und nicht unter "Teig, Brot und Pasta",
	// obwohl sie dieselbe Basisklasse ChefZ_GrainFoodBase teilen: sie sind der
	// Rohstoff der Kette, nicht ihr Erzeugnis.
	//
	// ChefZ_WildCorn steht in derselben Reihe wie ChefZ_Corn und
	// ChefZ_CornPlant, und aus demselben Grund: es ist die dritte Gestalt
	// desselben Rohstoffs. Ein Admin, der Mais sucht, sucht ihn als Zutat - ob
	// er den Kolben, die Beetpflanze oder den Wildwuchs braucht, weiss er
	// selbst, aber er will alle drei an einer Stelle finden. Die Wildpflanze
	// ist ein Weltobjekt und kein Inventaritem; dass das fuer COT nichts
	// aendert, ist im Dateikopf unter "UNAUFNEHMBARE WELTOBJEKTE" nachgewiesen.
	protected static void BuildIngredients()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_Wheat");
		classes.Insert("ChefZ_Flour");
		classes.Insert("ChefZ_Onion");
		classes.Insert("ChefZ_Garlic");
		classes.Insert("ChefZ_Carrot");
		classes.Insert("ChefZ_Cabbage");
		classes.Insert("ChefZ_Corn");
		classes.Insert("ChefZ_CornPlant");
		classes.Insert("ChefZ_WildCorn");
		classes.Insert("ChefZ_Egg");
		classes.Insert("ChefZ_RawSalt");
		classes.Insert("ChefZ_Salt");
		classes.Insert("ChefZ_BoneBroth");
		classes.Insert("ChefZ_TomatoSauce");
		classes.Insert("ChefZ_CreamSauce");
		classes.Insert("ChefZ_MushroomCreamSauce");

		Add("chefz_cot_ingredients", "#STR_CHEFZ_COT_CAT_INGREDIENTS", "carrot", classes);
	}

	// ChefZ / Kraeuter und Gewuerze - die vollstaendige Kraeuterkette in EINER
	// Kategorie: frisch (Fundpflanze), getrocknet, gemahlen.
	//
	// Bewusst nicht nach Verarbeitungsgrad aufgeteilt. Ein Admin sucht
	// "Thymian", nicht "Thymian, Stufe 2 von 3"; die drei Basisklassen dahinter
	// (ChefZ_FreshHerbBase, ChefZ_DriedHerbBase, ChefZ_SpiceBase)
	// interessieren ihn nicht.
	//
	// Die drei Wildkraeuter sind die nullte Stufe derselben Kette und stehen
	// deshalb hier - jedes direkt hinter seiner Ernte, damit die Liste Pflanze
	// und Bund nebeneinander zeigt. Sie sind Weltobjekte (ChefZ_WildPlant_Base,
	// vierte Basisklasse dieser Kategorie); genau deshalb kaeme man mit COTs
	// Typfilter ueber eine gemeinsame Basis hier noch weniger weit als zuvor.
	//
	// ChefZ_WildGarlic ist trotz des Namens KEINE Wildpflanze im Sinne des
	// Slice "wildplants", sondern der Baerlauch selbst - ein
	// ChefZ_FreshHerbBase wie Petersilie. Nicht mit ChefZ_Wild* verwechseln; es
	// gibt kein "ChefZ_WildWildGarlic".
	protected static void BuildHerbs()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_Parsley");
		classes.Insert("ChefZ_WildParsley");
		classes.Insert("ChefZ_Thyme");
		classes.Insert("ChefZ_WildThyme");
		classes.Insert("ChefZ_Rosemary");
		classes.Insert("ChefZ_WildRosemary");
		classes.Insert("ChefZ_WildGarlic");
		classes.Insert("ChefZ_PepperBerries");
		classes.Insert("ChefZ_DriedParsley");
		classes.Insert("ChefZ_DriedThyme");
		classes.Insert("ChefZ_DriedRosemary");
		classes.Insert("ChefZ_DriedWildGarlic");
		classes.Insert("ChefZ_DriedPaprika");
		classes.Insert("ChefZ_PaprikaPowder");
		classes.Insert("ChefZ_DriedPeppercorns");
		classes.Insert("ChefZ_BlackPepper");
		classes.Insert("ChefZ_HerbMix");
		classes.Insert("ChefZ_HunterSeasoning");

		Add("chefz_cot_herbs", "#STR_CHEFZ_COT_CAT_HERBS", "leaf", classes);
	}

	// ChefZ / Fleisch und Wurst - Hackfleisch, rohe und gegarte Wurst.
	//
	// Die acht haltbar gemachten Waren aus ChefZ_Preservation stehen ebenfalls
	// hier, obwohl sie eine eigene Basisklasse (ChefZ_PreservedFood_Base) und
	// ein eigenes Addon haben. Grund: die vereinbarten acht Kategorien kennen
	// keine "Konserven", und Salzfleisch, Doerrfleisch und Dauerwurst sind das,
	// was ein Admin unter Fleisch und Wurst sucht. Eine neunte Kategorie waere
	// eine eigenmaechtige Erweiterung des Auftrags gewesen.
	//
	// Salzfisch, Doerrfisch und Raeucherfisch stehen damit ebenfalls unter
	// "Fleisch und Wurst". Das ist der bewusst in Kauf genommene Preis
	// derselben Entscheidung. Der frische Fisch aus ChefZ_Fishing steht NICHT
	// hier - siehe die Luecke im Dateikopf.
	protected static void BuildMeat()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_MincedMeat");
		classes.Insert("ChefZ_MincedPork");
		classes.Insert("ChefZ_MincedVenison");
		classes.Insert("ChefZ_MincedBoar");
		classes.Insert("ChefZ_MincedChicken");
		classes.Insert("ChefZ_MincedBear");
		classes.Insert("ChefZ_RawSausage");
		classes.Insert("ChefZ_RawPorkSausage");
		classes.Insert("ChefZ_RawVenisonSausage");
		classes.Insert("ChefZ_RawBoarSausage");
		classes.Insert("ChefZ_RawHunterSausage");
		classes.Insert("ChefZ_RawSpicySausage");
		classes.Insert("ChefZ_CookedSausage");
		classes.Insert("ChefZ_PorkSausage");
		classes.Insert("ChefZ_VenisonSausage");
		classes.Insert("ChefZ_BoarSausage");
		classes.Insert("ChefZ_HunterSausage");
		classes.Insert("ChefZ_SpicySausage");
		classes.Insert("ChefZ_SaltedMeat");
		classes.Insert("ChefZ_DriedMeat");
		classes.Insert("ChefZ_SmokedMeat");
		classes.Insert("ChefZ_SaltedFish");
		classes.Insert("ChefZ_DriedFish");
		classes.Insert("ChefZ_SmokedFish");
		classes.Insert("ChefZ_SmokedSausage");
		classes.Insert("ChefZ_DrySausage");

		Add("chefz_cot_meat", "#STR_CHEFZ_COT_CAT_MEAT", "ham", classes);
	}

	// ChefZ / Teig, Brot und Pasta - das Erzeugnis der Getreidekette.
	//
	// Der Rohstoff der Kette (Weizen, Mehl) steht dagegen unter "Zutaten".
	protected static void BuildBaking()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_Dough");
		classes.Insert("ChefZ_RawPasta");
		classes.Insert("ChefZ_DriedPasta");
		classes.Insert("ChefZ_Bread");
		classes.Insert("ChefZ_Flatbread");

		Add("chefz_cot_baking", "#STR_CHEFZ_COT_CAT_BAKING", "croissant", classes);
	}

	// ChefZ / Milchprodukte - Rahm, Butter, Kaese.
	//
	// Die kleinste Kategorie, und die, an der COTs Typfilter am deutlichsten
	// scheitert: die drei erben von drei verschiedenen Vanilla-Klassen
	// (Marmalade, Lard, BoxCerealCrunchin).
	//
	// ChefZ_Egg gehoert im Slice "dairy" dazu, steht hier aber NICHT: ein Ei
	// ist kein Milchprodukt. Es steht unter "Zutaten".
	//
	// Butterfass und Kaesepresse sind Geraete und stehen unter "Stationen und
	// Werkzeuge", nicht hier. Die Milchkanne ist ein Leergefaess und steht
	// unter "Behaelter".
	protected static void BuildDairy()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_Cream");
		classes.Insert("ChefZ_Butter");
		classes.Insert("ChefZ_Cheese");

		Add("chefz_cot_dairy", "#STR_CHEFZ_COT_CAT_DAIRY", "milk", classes);
	}

	// ChefZ / Stationen und Werkzeuge - alles, was nicht gegessen wird.
	//
	// Die einzige Kategorie ohne ein einziges essbares Item. Sie ist im Alltag
	// die meistgebrauchte: eine Station, die einem Spieler abhandenkommt,
	// ersetzt der Admin - und dafuer muss er sie finden.
	protected static void BuildStations()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_GrainMill");
		classes.Insert("ChefZ_PastaMachine");
		classes.Insert("ChefZ_Mortar");
		classes.Insert("ChefZ_DryingRack");
		classes.Insert("ChefZ_ButterChurn");
		classes.Insert("ChefZ_CheesePress");
		classes.Insert("ChefZ_FryingPan");
		classes.Insert("ChefZ_MeatGrinder");
		classes.Insert("ChefZ_Smoker");
		classes.Insert("ChefZ_HandRake");

		Add("chefz_cot_stations", "#STR_CHEFZ_COT_CAT_STATIONS", "cooking-pot", classes);
	}

	// ChefZ / Gerichte - die 25 fertigen Speisen.
	//
	// Jeder Eintrag ist die ANGERICHTETE Form (ChefZ_ServedDish_Base,
	// ChefZ_Cooking/config.cpp:867-868). Die Bulk-Form im Kochgeraet
	// (ChefZ_<Name>Bulk : ChefZ_PortionedDish_Base, dort :736-737 als Muster
	// beschrieben) fehlt hier nicht, sondern existiert noch nicht: am
	// 17.09.2026 traegt keine config.cpp unter ChefZ_Core/Addons/ eine einzige
	// Klasse mit dem Suffix "Bulk". Kommt sie, gehoert sie in diese Liste,
	// paarweise unter ihre Portion.
	protected static void BuildDishes()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_TacticalBreakfast");
		classes.Insert("ChefZ_ScrambledEggSausage");
		classes.Insert("ChefZ_FarmersBreakfast");
		classes.Insert("ChefZ_CheeseFlatbread");
		classes.Insert("ChefZ_SausageBreadPlate");
		classes.Insert("ChefZ_MushroomPan");
		classes.Insert("ChefZ_PotatoPancakes");
		classes.Insert("ChefZ_MeatDumplings");
		classes.Insert("ChefZ_MilkRice");
		classes.Insert("ChefZ_HoneyBreadPlate");
		classes.Insert("ChefZ_HunterStewBowl");
		classes.Insert("ChefZ_FishermanStewBowl");
		classes.Insert("ChefZ_VegetableSoupBowl");
		classes.Insert("ChefZ_BoneBrothSoupBowl");
		classes.Insert("ChefZ_ChernarusChiliBowl");
		classes.Insert("ChefZ_SurvivorSpaghetti");
		classes.Insert("ChefZ_SausagePasta");
		classes.Insert("ChefZ_HunterPasta");
		classes.Insert("ChefZ_CreamMushroomPasta");
		classes.Insert("ChefZ_MacAndCheese");
		classes.Insert("ChefZ_SausagePotatoes");
		classes.Insert("ChefZ_HunterPlate");
		classes.Insert("ChefZ_BloodSausagePlate");
		classes.Insert("ChefZ_FishPotatoPlate");
		classes.Insert("ChefZ_BeanSausagePlate");

		Add("chefz_cot_dishes", "#STR_CHEFZ_COT_CAT_DISHES", "utensils", classes);
	}

	// ChefZ / Behaelter - die leeren Gefaesse.
	//
	// Getrennt von "Stationen und Werkzeuge", obwohl beides Nicht-Essbares ist:
	// ein Teller ist Verbrauchsgut und wird in Mengen ausgegeben, eine
	// Kaesepresse ist ein Einzelstueck.
	protected static void BuildContainers()
	{
		array<string> classes = new array<string>;
		classes.Insert("ChefZ_EmptyPlate");
		classes.Insert("ChefZ_EmptyBowl");
		classes.Insert("ChefZ_EmptyCan");
		classes.Insert("ChefZ_EmptyJar");
		classes.Insert("ChefZ_EmptyBox");
		classes.Insert("ChefZ_MilkCan");

		Add("chefz_cot_containers", "#STR_CHEFZ_COT_CAT_CONTAINERS", "container", classes);
	}
}
#endif // JM_COT
