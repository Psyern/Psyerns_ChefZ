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
// erweitert genau eine Klasse: JMObjectSpawnerForm. Keine COT-Datei wird
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
// SelectCategory(string) (:935-954, die Zuweisung in :937) aus der Tabelle
// CategoryTable (:527-560).
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
// dem Filterknopf neben dem Suchfeld: ein Kontextmenue mit vier Gruppenzeilen
// (:760-777), jede oeffnet ein Untermenue (:841-854). ChefZ haengt dort genau
// EINE weitere Gruppenzeile an und fuellt deren Untermenue mit den acht
// Kategorien. Kein eigenes Bedienelement, kein eigener Platz im Fenster.
//
//   1. RebuildCategoryMenu          super, danach EINMAL die ChefZ-Gruppenzeile
//                                   anhaengen (:760, Aufrufer :744).
//   2. RefreshCategoryMenuColors    super, danach die ChefZ-Gruppenzeile
//                                   einfaerben, wenn eine ChefZ-Kategorie
//                                   aktiv ist (:779-810).
//   3. RebuildCategorySubMenu       super, danach - nur fuer die ChefZ-Gruppe -
//                                   die acht Zeilen (:841-854).
//   4. RefreshCategorySubMenuColors super, danach die aktive ChefZ-Zeile
//                                   einfaerben (:856-872).
//   5. CategoryLabel / CategoryIcon Beschriftung und Symbol fuer die
//                                   ChefZ-Ids, sonst super (:661-687).
//   6. UpdateList                   erkennt an m_CurrentType, ob eine
//                                   ChefZ-Kategorie gewaehlt ist. Wenn nein -
//                                   und das ist der Normalfall - laeuft
//                                   unveraendert super.UpdateList(). Wenn ja,
//                                   fuellt ChefZ_FillClassList die Liste aus
//                                   der Kategorientabelle.
//
// SelectCategory (:935) braucht KEINE Ueberschreibung. Der Klick im Untermenue
// laeuft ueber COTs unveraenderten Pfad OnClick_CategorySubMenu (:905-912) ->
// CategoryIdFor (:702, laesst jede Id ausser "__all" unveraendert) ->
// SelectCategory -> m_CurrentType = "chefz_cot_*" -> UpdateList (:953).
//
// GroupOfCategory (:638) braucht ebenfalls keine Ueberschreibung.
// RefreshCategoryMenuColors faerbt ausschliesslich Zeilen, deren Id aus der
// statischen CategoryGroupTable (:570-580) stammt; die ChefZ-Gruppe steht dort
// nicht und wird deshalb in Punkt 2 selbst gefaerbt. Fuer eine ChefZ-Kategorie
// liefert COTs GroupOfCategory "" - genau richtig, denn dann faerbt super
// keine der vier COT-Gruppen ein.
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
// m_ClassList (:146), m_ListClasses (:31), m_SearchBox (:125), m_CategoryMenu
// (:18), m_CategorySubMenu (:19) und m_CurrentGroup (:22) sind in
// JMObjectSpawnerForm als "private" deklariert (m_Module :169 ist protected).
// In Enforce sind sie aus einer modded class dennoch erreichbar - die modded
// class IST die Klasse, nicht ihr Nachfahre. TerjeCompatibilityCOT/Scripts/
// 5_Mission/CotCompatibility.c nutzt dasselbe an derselben Klasse
// (m_ObjItemStateLiquid, m_PreviewItem).
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
}

// SCOUT-GEPRUEFT 2026-09-17 (chefz-cot130)
// Sieben Ueberschreibungen. Vier davon (RebuildCategoryMenu,
// RefreshCategoryMenuColors, RebuildCategorySubMenu,
// RefreshCategorySubMenuColors) rufen super als ERSTE Anweisung und haengen
// danach nur an; CategoryLabel und CategoryIcon geben fuer jede fremde Id
// super zurueck; UpdateList faellt in zwei von drei Zweigen auf super zurueck
// und geht nur bei einer der acht ChefZ-Kategorien eigene Wege. Das einzige
// Member traegt Mod-Praefix. TerjeCompatibilityCOT moddet dieselbe Klasse auf
// OnInit und UpdateItemStateType - keine Feld- oder Methodenueberschneidung.
modded class JMObjectSpawnerForm
{
	// Die Gruppenzeile wird genau EINMAL angehaengt. Ein eigenes Flag und
	// nicht GetItemCount(): COTs eigener Aufbau laeuft nur bei
	// GetItemCount() == 0 (:765), nach super ist der Zaehler also immer
	// groesser als 0 und taugt nicht als Unterscheidung.
	// UIActionContextMenu hat kein HasItem (gui/actions/uiactioncontextmenu.c:
	// 268-396), und AddItem baut bei jedem Aufruf alle Zeilen neu (:300-304) -
	// eine zweite Gruppenzeile waere also nicht nur doppelt, sondern teuer.
	protected bool m_ChefZCotGroupInMenu;

	/**
	 * Die ChefZ-Gruppenzeile an COTs Kategoriemenue anhaengen.
	 *
	 * submenu = true, damit die Zeile den Pfeil traegt und der Klick in
	 * OnClick_CategoryMenu (:885-903) nicht als Kategorie, sondern als
	 * Gruppenoeffnung gelesen wird: die Id ist nicht MENU_ID_ALL, also landet
	 * sie in OpenCategorySubMenu (:902).
	 */
	override protected void RebuildCategoryMenu()
	{
		super.RebuildCategoryMenu();

		if (!m_CategoryMenu)
		{
			return;
		}

		if (m_ChefZCotGroupInMenu)
		{
			return;
		}

		m_ChefZCotGroupInMenu = true;
		m_CategoryMenu.AddItem(ChefZ_CotMenu.GROUP_ID, ChefZ_CotMenu.GROUP_LABEL, JMConstants.Lucide(ChefZ_CotMenu.GROUP_ICON), 0, true);

		// super hat seine Farben gesetzt, bevor es die Zeile gab.
		RefreshCategoryMenuColors();
	}

	/**
	 * Die ChefZ-Gruppenzeile markieren, solange eine ChefZ-Kategorie aktiv ist.
	 *
	 * super faerbt nur Zeilen, deren Id aus CategoryGroupTable (:570-580)
	 * stammt (:794-807). Die ChefZ-Gruppe steht dort nicht und wuerde sonst nie
	 * markiert - der Admin saehe nicht, aus welcher Gruppe seine Liste kommt.
	 */
	override protected void RefreshCategoryMenuColors()
	{
		super.RefreshCategoryMenuColors();

		if (!m_CategoryMenu || !m_ChefZCotGroupInMenu || !m_Module)
		{
			return;
		}

		// 0 heisst "Menuefarbe benutzen" - genau wie bei COT (:787, :801).
		int color = 0;
		if (ChefZ_CotCategories.Find(m_Module.m_CurrentType))
		{
			color = JMTheme.ACCENT;
		}

		m_CategoryMenu.SetItemTextColor(ChefZ_CotMenu.GROUP_ID, color);
	}

	/**
	 * Das Untermenue der ChefZ-Gruppe fuellen.
	 *
	 * super leert das Menue und fuellt es aus CategoryGroupMembers
	 * (:585-636). Fuer die ChefZ-Gruppe liefert diese statische Tabelle eine
	 * LEERE Liste - sie wird bewusst nicht angefasst -, super hinterlaesst
	 * also ein leeres, sauber geraeumtes Menue, in das hier die acht Zeilen
	 * gehen.
	 */
	override protected void RebuildCategorySubMenu()
	{
		super.RebuildCategorySubMenu();

		if (!m_CategorySubMenu || m_CurrentGroup != ChefZ_CotMenu.GROUP_ID)
		{
			return;
		}

		array<ref ChefZ_CotCategory> categories = ChefZ_CotCategories.Get();
		if (!categories)
		{
			return;
		}

		ChefZ_CotCategory category;
		for (int i = 0; i < categories.Count(); i++)
		{
			category = categories.Get(i);
			if (!category)
			{
				continue;
			}

			// MenuIdFor (:694) waere hier wirkungslos: es tauscht nur den
			// leeren Text gegen MENU_ID_ALL, und keine FilterId ist leer.
			m_CategorySubMenu.AddItem(category.GetFilterId(), category.GetLabel(), JMConstants.Lucide(category.GetIconName()));
		}

		RefreshCategorySubMenuColors();
	}

	/** Die aktive ChefZ-Zeile im Untermenue markieren. */
	override protected void RefreshCategorySubMenuColors()
	{
		super.RefreshCategorySubMenuColors();

		if (!m_CategorySubMenu || m_CurrentGroup != ChefZ_CotMenu.GROUP_ID || !m_Module)
		{
			return;
		}

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
			if (category.GetFilterId() == m_Module.m_CurrentType)
			{
				color = JMTheme.ACCENT;
			}

			m_CategorySubMenu.SetItemTextColor(category.GetFilterId(), color);
		}
	}

	/**
	 * Beschriftung einer ChefZ-Id.
	 *
	 * Nicht nur fuer das Untermenue: SelectCategory legt jede gewaehlte Id in
	 * den Verlauf (:944, PushRecentCategory :962-976), und die Chips holen
	 * ihre Beschriftung genau hier (:1011). Ohne diese Ueberschreibung liefe
	 * COTs Suche in der statischen CategoryTable ins Leere und gaebe "" zurueck
	 * (:672) - der Chip stuende dann ohne Text da.
	 */
	override protected string CategoryLabel(string id)
	{
		ChefZ_CotCategory category = ChefZ_CotCategories.Find(id);
		if (category)
		{
			return category.GetLabel();
		}

		return super.CategoryLabel(id);
	}

	/**
	 * Symbol einer ChefZ-Id. Ohne diese Ueberschreibung faellt COT auf das
	 * Sammelsymbol "layers" zurueck (:686), und alle acht Kategorien saehen im
	 * Verlauf gleich aus.
	 */
	override protected string CategoryIcon(string id)
	{
		ChefZ_CotCategory category = ChefZ_CotCategories.Find(id);
		if (category)
		{
			return JMConstants.Lucide(category.GetIconName());
		}

		return super.CategoryIcon(id);
	}

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
		}

		m_ClassList.SetItems(rowLabels, rowSubs);

		if (m_SearchBox)
		{
			m_SearchBox.SetTextPreview(closestMatch);
		}
	}
}
#endif // JM_COT
