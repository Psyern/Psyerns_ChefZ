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
// ---------------------------------------------------------------------------
// WIE COTs OBJECT SPAWNER FILTERT - und warum das hier nicht reicht
// ---------------------------------------------------------------------------
// JMObjectSpawnerForm.c:911-982, UpdateList(): COT laeuft ueber CfgVehicles,
// CfgWeapons und CfgMagazines, verwirft scope 0 (und scope 1 ohne
// m_AllowRestrictedClassNames), verwirft Eintraege ohne Modell oder mit dem
// Platzhaltermodell "bmp", ruft m_Module.IsExcludedClassName und filtert
// zuletzt ueber das Suchfeld. Der Typfilter selbst ist eine Zeile:
//
//     if (m_Module.m_CurrentType == "" || g_Game.IsKindOf( strNameLower, m_Module.m_CurrentType ))
//
// m_CurrentType ist also ein einzelner BASISKLASSENNAME. Gesetzt wird er in
// SetListType (Z. 722) aus der Tabelle, die AddObjectType (Z. 494) fuellt -
// "edible_base", "transport", "weapon_base" und so fort (Z. 96-105).
//
// Warum ein einzelner Basisklassenname die acht ChefZ-Kategorien nicht
// abbilden kann, steht ausfuehrlich im Kopf von
// Scripts/4_World/ChefZ/Cot/ChefZ_CotCategories.c. Kurz: Milchprodukte und
// Stationen haben gar keine gemeinsame Basis, Kraeuter haetten fuenf.
//
// ---------------------------------------------------------------------------
// DER EINGRIFF, IN DREI TEILEN
// ---------------------------------------------------------------------------
// 1. OnInit       haengt EINE Auswahlbox unten an die Aktionsleiste. Sie traegt
//                 neun Eintraege: "Alle" plus die acht Kategorien.
// 2. UpdateList   erkennt an m_CurrentType, ob eine ChefZ-Kategorie gewaehlt
//                 ist. Wenn nein - und das ist der Normalfall - laeuft
//                 unveraendert super.UpdateList(). Wenn ja, fuellt
//                 ChefZ_FillClassList die Liste aus der Kategorientabelle.
// 3. SetListType  setzt die Auswahlbox auf "Alle" zurueck, sobald der Admin
//                 einen von COTs eigenen Typknoepfen drueckt. Ohne das zeigte
//                 die Box eine Kategorie an, die laengst nicht mehr gilt.
//
// ---------------------------------------------------------------------------
// WARUM EINE AUSWAHLBOX UNTEN UND KEINE ACHT KNOEPFE OBEN
// ---------------------------------------------------------------------------
// Der Knopfstreifen fuer Typfilter sitzt in "object_types_actions_wrapper".
// Dieses Panel ist laut JM/COT/GUI/layouts/objectspawner_form.layout Z. 26-35
// genau 320 Pixel hoch (0.2 Breite mal die Hoehe von
// object_spawn_wrapper_cont, "size 1 320"). Ein UIActionButton ist laut
// JM/COT/GUI/layouts/uiactions/UIActionButton.layout Z. 2 genau 30 Pixel hoch.
// COT setzt dort bereits zehn Knoepfe hin - 300 von 320 Pixeln. Fuer acht
// weitere ist kein Platz; sie wuerden aus dem Panel herauslaufen und den
// Bereich darunter ueberdecken.
//
// Deshalb eine Auswahlbox als zusaetzliche Zeile in m_SpawnerActionsWrapper.
// Das ist der GridSpacer, in den COT selbst seine vier Aktionszeilen haengt
// (JMObjectSpawnerForm.c:110); er traegt "Size To Content V" und waechst mit.
// Eine Auswahlbox statt einer Aufklappliste, weil eine Aufklappliste sich in
// der untersten Zeile nach unten aus dem Fenster oeffnen wuerde
// (UIActionDropdownList.c:178, "m_List.SetPos( xPos, yPos + 21, true )").
//
// ---------------------------------------------------------------------------
// PRIVATE FELDER DER BASISKLASSE
// ---------------------------------------------------------------------------
// m_ClassList, m_SearchBox, m_Module und m_SpawnerActionsWrapper sind in
// JMObjectSpawnerForm als "private" deklariert. In Enforce sind sie aus einer
// modded class dennoch erreichbar - die modded class IST die Klasse, nicht ihr
// Nachfahre. TerjeCompatibilityCOT/Scripts/5_Mission/CotCompatibility.c nutzt
// dasselbe an derselben Klasse (m_ObjItemStateLiquid, m_PreviewItem).
//
// ---------------------------------------------------------------------------
// KEINE SPIELMECHANIK
// ---------------------------------------------------------------------------
// Diese Datei liest Config und fuellt eine Liste. Sie spawnt nichts, sie
// veraendert kein Item, sie fasst weder Rezept noch Naehrwert an. Das Spawnen
// selbst bleibt vollstaendig COTs unveraenderte Sache - inklusive
// Rechtepruefung (JMObjectSpawnerModule registriert "Entity.Spawn.Position"
// und "Entity.Spawn.Inventory"), an der hier bewusst NICHTS vorbeigefuehrt
// wird. Die Kategorien machen Items auffindbar, nicht spawnbar; wer sie ohne
// Recht anwaehlt, sieht eine Liste und bekommt beim Spawnen dieselbe Absage
// wie zuvor.
modded class JMObjectSpawnerForm
{
	// Die zusaetzliche Auswahlbox. NULL, solange OnInit nicht gelaufen ist -
	// und UpdateList laeuft am Ende von super.OnInit() bereits einmal. Jeder
	// Zugriff unten ist deshalb abgesichert.
	protected UIActionSelectBox m_ChefZCategorySelect;

	override void OnInit()
	{
		super.OnInit();

		// Kein Wrapper, keine Box - und COT laeuft weiter wie ohne diesen Mod.
		// CreateGridSpacer kann NULL liefern, wenn ein Layout fehlt
		// (UIActionManager.c:9, CheckWidget), und CreateSelectionBox verlangt
		// "notnull Widget parent". Eine fehlende Adminmaske ist ein Aergernis,
		// ein Absturz beim Oeffnen des Spawners ist einer mehr.
		if (!m_SpawnerActionsWrapper)
		{
			return;
		}

		// Eintrag 0 ist "Alle": kein ChefZ-Filter, COT verhaelt sich exakt wie
		// ohne diesen Mod. Danach die acht Kategorien in Tabellenreihenfolge.
		array<string> options = new array<string>;
		options.Insert("#STR_CHEFZ_COT_CAT_NONE");

		foreach (ref ChefZ_CotCategory category : ChefZ_CotCategories.Get())
		{
			options.Insert(category.GetLabel());
		}

		m_ChefZCategorySelect = UIActionManager.CreateSelectionBox(
			m_SpawnerActionsWrapper, "#STR_CHEFZ_COT_CATEGORY", options,
			this, "ChefZ_OnCategoryChanged");

		if (m_ChefZCategorySelect && m_Module)
		{
			m_ChefZCategorySelect.SetSelectorWidth(0.6);

			// JMObjectSpawnerModule lebt laenger als das Formular:
			// m_CurrentType ueberdauert das Schliessen des Fensters. Stand dort
			// beim letzten Mal eine ChefZ-Kategorie, zeigt die Box sie wieder
			// an - sonst stuende sie auf "Alle", waehrend die Liste gefiltert
			// ist. SetSelection mit sendEvent = false, sonst loeste das
			// Wiederherstellen ein UpdateList aus, das super.OnInit() gerade
			// erledigt hat.
			m_ChefZCategorySelect.SetSelection(
				ChefZ_CotCategories.IndexOf(m_Module.m_CurrentType) + 1, false);
		}
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
	 * COTs eigener Typfilter und der ChefZ-Filter schliessen einander aus -
	 * beide leben in derselben Variablen. Wer oben einen Typknopf drueckt,
	 * bekommt deshalb hier die Auswahlbox auf "Alle" zurueckgestellt, bevor
	 * super seinen Wert in m_CurrentType schreibt.
	 *
	 * sendEvent = false: der Rueckstellung darf kein UpdateList folgen, sonst
	 * liefe die Liste einmal ohne den Typ, den super gleich setzt.
	 */
	override void SetListType(UIEvent eid, UIActionBase action)
	{
		if (eid == UIEvent.CLICK && m_ChefZCategorySelect)
		{
			m_ChefZCategorySelect.SetSelection(0, false);
		}

		super.SetListType(eid, action);
	}

	/** Rueckruf der Auswahlbox. */
	void ChefZ_OnCategoryChanged(UIEvent eid, UIActionBase action)
	{
		if (eid != UIEvent.CHANGE)
		{
			return;
		}

		if (!m_ChefZCategorySelect || !m_Module)
		{
			return;
		}

		// Eintrag 0 ist "Alle" - der leere Text ist genau der Wert, den COTs
		// Knopf "ALL" setzt (JMObjectSpawnerForm.c:96). Damit landet der Admin
		// wieder in COTs unveraendertem Zweig.
		int index = m_ChefZCategorySelect.GetSelection() - 1;
		array<ref ChefZ_CotCategory> categories = ChefZ_CotCategories.Get();

		if (index < 0 || index >= categories.Count())
		{
			m_Module.m_CurrentType = "";
		}
		else
		{
			m_Module.m_CurrentType = categories.Get(index).GetFilterId();
		}

		UpdateList();
	}

	/**
	 * Die Liste aus einer ChefZ-Kategorie fuellen.
	 *
	 * Bewusst dieselben Pruefungen in derselben Reihenfolge wie COTs
	 * UpdateList (JMObjectSpawnerForm.c:938-975). Ein Eintrag, den COT im
	 * Zweig "Alle" verwirft, wird auch hier verworfen - sonst zeigte eine
	 * ChefZ-Kategorie Items an, die COT sonst nirgends anbietet, und der
	 * Filter waere ein Schleichweg an m_AllowRestrictedClassNames und
	 * IsExcludedClassName vorbei.
	 *
	 * Der eine Unterschied: hier wird nicht ueber ganz CfgVehicles gelaufen,
	 * sondern nur ueber die Namen der Kategorie. Deshalb steht ganz vorn
	 * ConfigIsExisting - ein Name aus einem nicht geladenen ChefZ-Addon faellt
	 * dort still heraus.
	 */
	protected void ChefZ_FillClassList(ChefZ_CotCategory category)
	{
		if (!m_ClassList)
		{
			return;
		}

		m_ClassList.ClearItems();

		string closestMatch;

		COT_String search = m_Module.m_SearchText;
		bool requireAllKeywords;
		TStringArray keywords = search.KeywordSearch_Prepare(requireAllKeywords);

		array<string> classNames = category.GetClasses();
		for (int i = 0; i < classNames.Count(); i++)
		{
			string className = classNames.Get(i);
			string path = CFG_VEHICLESPATH + " " + className;

			// Addon nicht geladen -> Eintrag entfaellt, ohne Meldung. Das ist
			// die Stelle, an der dieses Modul optional wird.
			if (!g_Game.ConfigIsExisting(path))
			{
				continue;
			}

			int scope = g_Game.ConfigGetInt(path + " scope");
			if (scope == 0 || (scope == 1 && !m_Module.m_AllowRestrictedClassNames))
			{
				continue;
			}

			string model;
			if (!g_Game.ConfigGetText(path + " model", model) || model == string.Empty || model == "bmp")
			{
				continue;
			}

			COT_String candidate = className;
			candidate.ToLower();

			if (m_Module.IsExcludedClassName(candidate))
			{
				continue;
			}

			// Dieselbe Umschaltung wie in COT: ist der Haken
			// "#STR_COT_OBJECT_MODULE_SPAWN_DISPLAYNAME" gesetzt, sucht der
			// Admin im Anzeigenamen statt im Klassennamen.
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

			m_ClassList.AddItem(className, NULL, 0);
		}

		if (m_SearchBox)
		{
			m_SearchBox.SetTextPreview(closestMatch);
		}
	}
}
#endif // JM_COT
