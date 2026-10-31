// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn Community Online Tools
// geladen ist. Fehlt der Mod, ist JM_COT nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef JM_COT
// ChefZ_CotObjectSpawner - haengt die acht ChefZ-Kategorien an COTs Object
// Spawner.
//
// Das ist die EINZIGE Datei dieses Mods, die COT-Code erweitert, und sie
// erweitert zwei Klassen: JMObjectSpawnerForm und JMItemCategoryPicker. Keine COT-Datei wird
// veraendert; jede Ueberschreibung ruft super auf oder faellt auf super zurueck.
//
// STAND: COT_New (COT_New/Scripts/...). Alle Zeilennummern unten beziehen sich
// auf COT_New/Scripts/5_mission/communityonlinetools/modules/object/
// jmobjectspawnerform.c, sofern nicht anders genannt. Das alte COT
// (JM/COT/...) wird nicht mehr bedient: dort hiessen die Haken SetListType und
// AddObjectType, und der Aktionsstreifen m_SpawnerActionsWrapper - beides gibt
// es in COT_New nicht mehr.
//
// ---------------------------------------------------------------------------
// WIE COTs OBJECT SPAWNER FILTERT - und warum das hier nicht reicht
// ---------------------------------------------------------------------------
// UpdateList() (:2386-2488) laeuft ueber CfgVehicles, CfgWeapons und
// CfgMagazines, verwirft scope 0 (und scope 1 ohne
// m_AllowRestrictedClassNames, :2430), verwirft Eintraege ohne Modell oder mit
// dem Platzhaltermodell "bmp" (:2434), ruft m_Module.IsExcludedClassName
// (:2439) und filtert zuletzt ueber das Suchfeld (:2451). Der Typfilter selbst
// ist eine Zeile (:2437):
//
//     if (m_Module.m_CurrentType == "" || g_Game.IsKindOf( strNameLower, m_Module.m_CurrentType ))
//
// m_CurrentType ist also ein einzelner BASISKLASSENNAME. Gesetzt wird er in
// OnCategoryChanged aus der Auswahl des JMItemCategoryPicker.
//
// Warum ein einzelner Basisklassenname die acht ChefZ-Kategorien nicht
// abbilden kann, steht ausfuehrlich im Kopf von
// Scripts/4_World/ChefZ/Cot/ChefZ_CotCategories.c. Kurz: Milchprodukte und
// Stationen haben gar keine gemeinsame Basis, Kraeuter haetten vier.
//
// ---------------------------------------------------------------------------
// DER EINGRIFF: EINE GRUPPE IM KATEGORIEMENUE
// ---------------------------------------------------------------------------
// COT_New zeigt seine Kategorien nicht mehr als Knopfstreifen, sondern hinter
// dem Filterknopf neben dem Suchfeld: ein UIActionFilterMenu mit vier
// Gruppenzeilen, jede oeffnet eine eigene Seite. ChefZ haengt dort genau EINE
// weitere Gruppenzeile an und fuellt deren Seite mit den acht Kategorien. Kein
// eigenes Bedienelement, kein eigener Platz im Fenster.
//
//   1. BuildRootPage                super, danach die ChefZ-Gruppenzeile, markiert
//                                   solange eine ChefZ-Kategorie aktiv ist.
//   2. ChefZ_BuildGroupPage         die acht Zeilen der ChefZ-Gruppe.
//   3. Toggle                       meldet die Seite der Gruppe an, sobald
//                                   COT das Menue angelegt hat.
//   4. PushRecent                   ChefZ-Kategorien bekommen keinen Chip.
//   5. UpdateList (Formular)        erkennt an m_CurrentType, ob eine
//                                   ChefZ-Kategorie gewaehlt ist. Wenn nein -
//                                   und das ist der Normalfall - laeuft
//                                   unveraendert super.UpdateList(). Wenn ja,
//                                   fuellt ChefZ_FillClassList die Liste aus
//                                   der Kategorientabelle.
//
// STAND 30.10.2026: COT hat das Kategoriemenue aus JMObjectSpawnerForm nach
// JMItemCategoryPicker verlegt. RebuildCategoryMenu und die uebrigen fuenf
// frueheren Haken gibt es nicht mehr; ihre Overrides brachen das Mission-Modul.
// Ein Klick laeuft jetzt OnGroupChange -> Select -> OnCategoryChanged (Formular)
// -> m_CurrentType = "chefz_cot_*" -> UpdateList.
//
// JMFilterRegistry waere der vorgesehene Weg fuer einzelne Zeilen, kennt aber
// nur die Wurzelseite: acht flache Zeilen statt einer Gruppe wie bei COT selbst.
//
// CategoryLabel und CategoryIcon des Pickers sind statisch und lassen sich nicht
// ueberschreiben. Ein Chip stuende deshalb ohne Text da; die Kategorien bleiben
// ueber das Menue erreichbar, darum faellt nur der Chip weg (PushRecent).
//
// ---------------------------------------------------------------------------
// WARUM KEINE EIGENE AUSWAHLBOX MEHR
// ---------------------------------------------------------------------------
// Bis zum Umbau auf COT_New hing hier eine UIActionSelectBox im
// Aktionsstreifen m_SpawnerActionsWrapper. Dieses Member gibt es in COT_New
// nicht mehr (die Container heissen jetzt m_SearchWrapper, m_FilterWrapper,
// m_RecentWrapper und m_ListWrapper, :33-36, und werden in OnResize auf feste
// Hoehen gepinnt). Eine Box mit eigenem Platz im Fenster waere ausserdem genau
// das, was COT_New abgeschafft hat. Deshalb: keine Box, kein OnInit-Override.
//
// Nebenbei erledigt sich damit auch eine Falle: UIActionSelectBox.SetSelections
// reicht seine Eintraege UNUEBERSETZT an OptionSelectorMultistate weiter
// (gui/actions/uiactionselectbox.c:34-45, nur SetLabel uebersetzt, :65-71).
// COT uebersetzt seine Optionen deshalb selbst mit Widget.TranslateString
// (:362-363, :374-375). Die Kontextmenue-Zeilen sind davon nicht betroffen.
//
// ---------------------------------------------------------------------------
// PRIVATE FELDER DER BASISKLASSE
// ---------------------------------------------------------------------------
// JMObjectSpawnerForm fuehrt m_ClassList, m_ListClasses, m_SearchBox und m_Module
// als protected. Eine modded class ist die Klasse selbst und erreicht sie direkt.
//
// ---------------------------------------------------------------------------
// KEINE SPIELMECHANIK
// ---------------------------------------------------------------------------
// Diese Datei liest Config und fuellt eine Liste. Sie spawnt nichts, sie
// veraendert kein Item, sie fasst weder Rezept noch Naehrwert an. Das Spawnen
// selbst bleibt vollstaendig COTs unveraenderte Sache - inklusive
// Rechtepruefung (JMObjectSpawnerModule registriert "Entity.Spawn.Position"
// und "Entity.Spawn.Inventory", jmobjectspawnermodule.c:36-37), an der hier
// bewusst NICHTS vorbeigefuehrt wird. Die Kategorien machen Items auffindbar,
// nicht spawnbar; wer sie ohne Recht anwaehlt, sieht eine Liste und bekommt
// beim Spawnen dieselbe Absage wie zuvor.
//
// IsExcludedClassName (jmobjectspawnermodule.c:964-984) wird hier aus
// OBERFLAECHENGLEICHHEIT mitgeprueft, nicht als Rechtegrenze: seine Listen
// werden nur beim Mission-Host aus JMSpawnerConfig gefuellt (:42-48, :50-66).
// Ein Eintrag, den COT im Zweig "Alle" verwirft, soll auch in einer
// ChefZ-Kategorie nicht erscheinen - sonst waere der Filter ein Schleichweg an
// m_AllowRestrictedClassNames vorbei.

/**
 * Die Kennungen der ChefZ-Gruppe im COT-Kategoriemenue.
 *
 * Eigene Klasse statt static const in der modded class: eine modded class
 * erweitert eine fremde Klasse, und dort gehoeren nur Member mit Mod-Praefix
 * hinein - Konstanten, die niemand ausserhalb dieser Datei braucht, bleiben
 * besser daneben.
 *
 * GROUP_ID ist NIE ein Wert von m_CurrentType. Sie ist nur die Id der
 * Gruppenzeile im Kontextmenue; die Zeilen im Untermenue tragen die FilterIds
 * aus ChefZ_CotCategories.
 */
class ChefZ_CotMenu
{
	static const string GROUP_ID = "chefz_cot_group";
	static const string GROUP_LABEL = "#STR_CHEFZ_COT_GROUP";
	static const string GROUP_ICON = "chef-hat";

	//! Die Pfanne, unter der die Pfannengerichte als Auswahl stehen.
	static const string PAN_CLASS = "ChefZ_FryingPan";
	//! Woran ein Pfannengericht erkennbar ist: sein Modell (30.10.2026).
	static const string PAN_DISH_MODEL = "panfood_base.p3d";
}

// Die ChefZ-Gruppe im Kategoriemenue des Object Spawners. Der Picker dient auch
// der Loot-Analyse; die kennt die ChefZ-Filter nicht, also gilt alles nur fuer den
// Geltungsbereich OBJECTS.
modded class JMItemCategoryPicker
{
	protected bool m_ChefZCotPageAdded;

	override void Toggle(notnull Widget button)
	{
		super.Toggle(button);

		if (!m_Menu || m_ChefZCotPageAdded || m_RegistryScope != JMFilterRegistry.OBJECTS)
		{
			return;
		}

		m_ChefZCotPageAdded = true;
		m_Menu.AddPage(ChefZ_CotMenu.GROUP_ID, this, "ChefZ_BuildGroupPage", PAGE_ROOT, false, "", this, "OnGroupChange", true);
	}

	override void BuildRootPage(UIActionFilterMenu menu)
	{
		super.BuildRootPage(menu);

		if (m_RegistryScope != JMFilterRegistry.OBJECTS)
		{
			return;
		}

		// 0 heisst "Menuefarbe benutzen". COTs GroupOfCategory kennt die Gruppe
		// nicht, die Markierung steht deshalb hier.
		int color = 0;
		if (ChefZ_CotCategories.Find(m_Current))
		{
			color = JMTheme.ACCENT;
		}

		menu.AddRow(ChefZ_CotMenu.GROUP_ID, ChefZ_CotMenu.GROUP_LABEL, JMConstants.Lucide(ChefZ_CotMenu.GROUP_ICON), color, true, ChefZ_CotMenu.GROUP_ID);
	}

	//! Oeffentlich, weil das Menue die Seite ueber ihren Namen aufruft.
	void ChefZ_BuildGroupPage(UIActionFilterMenu menu)
	{
		array<ref ChefZ_CotCategory> categories = ChefZ_CotCategories.Get();
		if (!categories)
		{
			return;
		}

		ChefZ_CotCategory category;
		int color;
		for (int i = 0; i < categories.Count(); i++)
		{
			category = categories.Get(i);
			if (!category)
			{
				continue;
			}

			color = 0;
			if (category.GetFilterId() == m_Current)
			{
				color = JMTheme.ACCENT;
			}

			menu.AddRow(category.GetFilterId(), category.GetLabel(), JMConstants.Lucide(category.GetIconName()), color);
		}
	}

	override protected bool PushRecent(string id)
	{
		if (ChefZ_CotCategories.Find(id))
		{
			return false;
		}

		return super.PushRecent(id);
	}
}

// Eine Ueberschreibung: UpdateList faellt in zwei von drei Zweigen auf super
// zurueck und geht nur bei einer der acht ChefZ-Kategorien eigene Wege.
// TerjeCompatibilityCOT moddet dieselbe Klasse auf OnInit und UpdateItemStateType.
modded class JMObjectSpawnerForm
{
	/**
	 * Der Verteiler.
	 *
	 * Find() liefert nur fuer die acht ChefZ-FilterIds eine Kategorie. Fuer
	 * COTs eigene Typen, fuer den leeren Text und fuer alles, was ein dritter
	 * Mod je in m_CurrentType schreiben mag, liefert sie NULL - und dann laeuft
	 * hier COTs Original, Zeile fuer Zeile unveraendert.
	 */
	override void UpdateList()
	{
		if (!m_Module)
		{
			super.UpdateList();
			return;
		}

		ChefZ_CotCategory category = ChefZ_CotCategories.Find(m_Module.m_CurrentType);
		if (!category)
		{
			super.UpdateList();
			return;
		}

		ChefZ_FillClassList(category);
	}

	/**
	 * Die Liste aus einer ChefZ-Kategorie fuellen.
	 *
	 * Bewusst dieselben Pruefungen in derselben Reihenfolge wie COTs
	 * UpdateList (:2417-2473) und derselbe Abschluss: m_ListClasses index-
	 * parallel zu den Zeilen fuellen und die Liste mit EINEM SetItems setzen
	 * (:2477). Das ist keine Formsache - UIActionItemList kennt weder AddItem
	 * noch ClearItems (gui/actions/uiactionitemlist.c:290 SetItems, :311
	 * Clear), und GetCurrentSelection (:2514-2522) liest den Klassennamen
	 * ausschliesslich aus m_ListClasses[row]. Bliebe das Array ungepflegt,
	 * spawnte der Admin in einer ChefZ-Kategorie die Klasse, die in der
	 * vorherigen Liste an derselben Zeile stand, und der Export in die
	 * Zwischenablage (:2161-2209) lieferte die alte Liste.
	 *
	 * Ein Eintrag, den COT im Zweig "Alle" verwirft, wird auch hier verworfen -
	 * Begruendung im Dateikopf unter "KEINE SPIELMECHANIK".
	 *
	 * Der eine Unterschied: hier wird nicht ueber ganz CfgVehicles gelaufen,
	 * sondern nur ueber die Namen der Kategorie. Deshalb steht ganz vorn
	 * ConfigIsExisting - ein Name aus einem nicht geladenen ChefZ-Addon faellt
	 * dort still heraus.
	 *
	 * COTs ReportExactClassRejection (:2341, aufgerufen :2480) wird hier
	 * bewusst NICHT aufgerufen: es durchsucht alle drei Configbaeume nach dem
	 * Suchtext und meldete dem Admin dann, warum eine Klasse abgelehnt wurde,
	 * die in dieser Kategorie ohnehin nicht steht. In einer Kategorienliste ist
	 * das keine Hilfe, sondern eine falsche Faehrte.
	 */
	/**
	 * Auswahl "mit welchem Gericht spawnen" an der Pfanne (30.10.2026,
	 * Spieltest: "ChefZ FryingPan sollte im COT eine Auswahlmoeglichkeit haben,
	 * mit welchem Gericht man es spawnen kann, z. B. ChefZ_HunterPasta").
	 *
	 * Ein Pfannengericht IST die Pfanne mit dem Gericht darin: dasselbe Modell
	 * Panfood_Base.p3d, nur die Textur traegt das Gericht. Deshalb spawnt jede
	 * Zeile direkt die Gerichtsklasse - eine echte Klasse, mit Vorschau und
	 * ohne Sonderweg im Spawner. Welche Gerichte dazugehoeren, liest diese
	 * Methode aus dem Modellpfad der Gerichtskategorie; ein neues
	 * Pfannengericht erscheint ohne Aenderung hier.
	 */
	protected void ChefZ_AddPanDishRows(array<string> rowLabels, array<string> rowSubs)
	{
		ChefZ_CotCategory dishes = ChefZ_CotCategories.Find("chefz_cot_dishes");
		if (!dishes)
		{
			return;
		}

		array<string> dishClasses = dishes.GetClasses();
		foreach (string dish : dishClasses)
		{
			string path = CFG_VEHICLESPATH + " " + dish;
			string model;
			if (!g_Game.ConfigGetText(path + " model", model))
			{
				continue;
			}

			model.ToLower();
			if (!model.Contains(ChefZ_CotMenu.PAN_DISH_MODEL))
			{
				continue;
			}

			string label = "    " + ChefZ_CotMenu.PAN_CLASS + " -> " + dish;
			string displayName;
			if (m_Module.m_FilterWithDisplayName && g_Game.ConfigGetText(path + " displayName", displayName) && displayName != "")
			{
				label = "    -> " + Widget.TranslateString(displayName);
			}

			rowLabels.Insert(label);
			rowSubs.Insert(dish);
			m_ListClasses.Insert(dish);
		}
	}

	protected void ChefZ_FillClassList(ChefZ_CotCategory category)
	{
		if (!m_ClassList || !m_ListClasses)
		{
			return;
		}

		m_ListClasses.Clear();

		array<string> rowLabels = new array<string>;
		array<string> rowSubs = new array<string>;

		string closestMatch;
		string className;
		string path;
		string model;
		string displayName;
		string rowText;
		int scope;

		COT_String search = m_Module.m_SearchText;
		bool requireAllKeywords;
		TStringArray keywords = search.KeywordSearch_Prepare(requireAllKeywords);

		COT_String candidate;

		array<string> classNames = category.GetClasses();
		for (int i = 0; i < classNames.Count(); i++)
		{
			className = classNames.Get(i);
			path = CFG_VEHICLESPATH + " " + className;

			// Addon nicht geladen -> Eintrag entfaellt, ohne Meldung. Das ist
			// die Stelle, an der dieses Modul optional wird.
			if (!g_Game.ConfigIsExisting(path))
			{
				continue;
			}

			scope = g_Game.ConfigGetInt(path + " scope");
			if (scope == 0 || (scope == 1 && !m_Module.m_AllowRestrictedClassNames))
			{
				continue;
			}

			if (!g_Game.ConfigGetText(path + " model", model) || model == string.Empty || model == "bmp")
			{
				continue;
			}

			candidate = className;
			candidate.ToLower();

			if (m_Module.IsExcludedClassName(candidate))
			{
				continue;
			}

			// Dieselbe Umschaltung wie in COT (:2443-2449): ist der
			// Anzeigename-Modus gesetzt, sucht der Admin im Anzeigenamen statt
			// im Klassennamen.
			if (m_Module.m_FilterWithDisplayName)
			{
				if (!g_Game.ConfigGetText(path + " displayName", candidate))
				{
					continue;
				}

				candidate.ToLower();
			}

			if (search != "")
			{
				if (!candidate.KeywordSearchImplEx(search, keywords, requireAllKeywords, closestMatch))
				{
					continue;
				}
			}

			// Der Anzeigename aendert, was die Zeile ZEIGT, nicht was sie
			// bedeutet - genau wie in COT (:2454-2470). Der Klassenname geht in
			// jedem Fall nach m_ListClasses und rutscht in der Zeile nach
			// hinten, statt zu verschwinden: er ist das, was der Admin
			// anderswo eintippen muss.
			rowText = className;

			if (m_Module.m_FilterWithDisplayName)
			{
				if (g_Game.ConfigGetText(path + " displayName", displayName) && displayName != "")
				{
					rowText = Widget.TranslateString(displayName);
				}
			}

			rowLabels.Insert(rowText);

			if (rowText == className)
			{
				rowSubs.Insert("");
			}
			else
			{
				rowSubs.Insert(className);
			}

			m_ListClasses.Insert(className);

			if (className == ChefZ_CotMenu.PAN_CLASS)
			{
				ChefZ_AddPanDishRows(rowLabels, rowSubs);
			}
		}

		m_ClassList.SetItems(rowLabels, rowSubs);

		if (m_SearchBox)
		{
			m_SearchBox.SetTextPreview(closestMatch);
		}
	}
}
#endif // JM_COT
