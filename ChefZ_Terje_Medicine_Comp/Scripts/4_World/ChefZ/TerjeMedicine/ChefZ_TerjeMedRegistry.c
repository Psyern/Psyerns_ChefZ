// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeMedicine
// geladen ist. Fehlt der Mod, ist TERJE_MEDICINE_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_MEDICINE_MOD
// ============================================================================
// ChefZ_TerjeMedRegistry
//
// Nachschlagewerk fuer CfgChefZTerjeMedicine. Beantwortet genau eine Frage:
// "Gehoert der Consumable-Pfad, den TerjeCore gerade uebergibt, zu einem
//  ChefZ-Item mit hinterlegter Terje-Medizinwirkung?"
//
// Warum ueberhaupt ein Registry und nicht ein ConfigIsExisting je Bissen:
// TerjeConsumableEffects.Apply laeuft bei JEDEM Schluck und JEDEM Bissen JEDES
// Spielers. Der Klassenname-Vergleich muss deshalb ein Set-Lookup sein und kein
// Config-Zugriff. Die WERTE dagegen werden absichtlich NICHT zwischengespeichert:
// GetTerjeGameConfig() mischt Server-Overrides aus
// $profile:TerjeSettings/Core/GameOverrides.xml erst beim Lesen dazu, und der
// Client bekommt seine Kopie ueber einen RPC nachgereicht
// (TerjeCore/Scripts/3_Game/TerjeGameConfig.c:44-52). Ein Wertecache waere
// genau dort stillschweigend veraltet.
//
// Der Klassenbestand selbst kann sich zur Laufzeit nicht aendern - er kommt aus
// der binarisierten config.cpp - und wird deshalb einmal gebaut.
// ============================================================================

class ChefZ_TerjeMedRegistry : Managed
{
	// Wurzelknoten dieses Moduls. Bewusst NICHT "CfgVehicles": die Tee-Klassen
	// gehoeren dem Hauptmod, und ein Compatibility-Modul oeffnet keine fremde
	// Klasse ein zweites Mal. Begruendung ausfuehrlich im Kopf der config.cpp.
	static const string CFG_ROOT = "CfgChefZTerjeMedicine";

	// Praefixschutz. Selbst wenn jemand in CfgChefZTerjeMedicine versehentlich
	// einen Vanilla- oder Terje-Namen eintraegt, greift dieses Modul nicht
	// darauf zu. Ein Compatibility-Modul darf niemals fremde Items umlenken.
	static const string CHEFZ_PREFIX = "chefz_";

	// Singleton als statischer Member der Klasse, nicht als freie Variable auf
	// Dateiebene: "ref" gehoert an einen Member. Dasselbe Muster benutzt der
	// Core bereits, z.B.
	// ChefZ_Core/Addons/ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_ConfigManager.c:57
	// mit dem Getter :158-162.
	private static ref ChefZ_TerjeMedRegistry s_Instance;

	protected ref set<string> m_KnownIds;

	void ChefZ_TerjeMedRegistry()
	{
		m_KnownIds = new set<string>;
		Build();
	}

	// Faul erzeugt wie zuvor - es gibt weiterhin keinen Init-Zeitpunkt, den ein
	// anderer Mod verpassen koennte. Der erste Aufruf kommt entweder aus
	// ChefZ_TerjeMedStartupCheck (Serverstart) oder aus dem ersten
	// TerjeConsumableEffects.Apply.
	static ChefZ_TerjeMedRegistry Get()
	{
		if (!s_Instance)
		{
			s_Instance = new ChefZ_TerjeMedRegistry();
		}

		return s_Instance;
	}

	protected void Build()
	{
		int count = GetTerjeGameConfig().ConfigGetChildrenCount(CFG_ROOT);
		for (int i = 0; i < count; i++)
		{
			string childName;
			if (!GetTerjeGameConfig().ConfigGetChildName(CFG_ROOT, i, childName))
			{
				continue;
			}

			if (childName == string.Empty)
			{
				continue;
			}

			childName.ToLower();

			// Fremde Namen werden hier still verworfen statt registriert.
			if (childName.IndexOf(CHEFZ_PREFIX) != 0)
			{
				TerjeLog_Warning("ChefZ_TerjeMedRegistry: '" + childName + "' in " + CFG_ROOT + " traegt kein ChefZ-Praefix und wird ignoriert.");
				continue;
			}

			if (m_KnownIds.Find(childName) == -1)
			{
				m_KnownIds.Insert(childName);
			}
		}
	}

	int GetCount()
	{
		return m_KnownIds.Count();
	}

	string GetIdAt(int index)
	{
		return m_KnownIds.Get(index);
	}

	// Der Pfad, den TerjeCore uebergibt, sieht so aus:
	//
	//   "CfgVehicles ChefZ_ThymeTea"              Stueckware (Edible_Base)
	//   "CfgTerjeCustomLiquids ChefZ_HerbalTea"   Terje-Custom-Liquid
	//   "CfgLiquidDefinitions Water"              Vanilla-Fluessigkeit
	//
	// (TerjeCore/Scripts/4_World/Entities/ItemBase.c:294-316 und
	//  TerjeCore/Scripts/4_World/Entities/PlayerBase.c:709-740)
	//
	// Welcher Wurzelknoten davorsteht, ist uns egal - entscheidend ist der
	// Klassenname dahinter. Dadurch funktioniert dieses Modul unveraendert,
	// egal ob das Hauptmod die Tees spaeter als Essitem oder als Fluessigkeit
	// umsetzt. Das ist keine Bequemlichkeit, sondern die Absicherung gegen den
	// offenen Punkt aus dem Uebergabevertrag: die Items existieren noch nicht.
	bool Resolve(string consumablePath, out string effectPath)
	{
		effectPath = string.Empty;

		if (consumablePath == string.Empty)
		{
			return false;
		}

		int sep = consumablePath.IndexOf(" ");
		if (sep < 1)
		{
			return false;
		}

		int tailLength = consumablePath.Length() - sep - 1;
		if (tailLength < 1)
		{
			return false;
		}

		string id = consumablePath.Substring(sep + 1, tailLength);
		id.ToLower();

		if (m_KnownIds.Find(id) == -1)
		{
			return false;
		}

		effectPath = CFG_ROOT + " " + id;
		return true;
	}

	// "amount" ist bei Stueckware eine Quantity-Einheit, bei einer Fluessigkeit
	// ein Milliliter (Edible_Base.Consume -> ApplyTerjeConsumableEffects). Die
	// Zeiten in der config.cpp sind je VOLLER PORTION angegeben; chefzServingSize
	// rechnet die eine Einheit in die andere um. Ohne das waere ein 250-ml-Becher
	// 250-mal so stark wie eine Tasse Stueckware.
	float GetServings(string effectPath, float amount)
	{
		if (amount <= 0)
		{
			return 0;
		}

		float servingSize = GetTerjeGameConfig().ConfigGetFloat(effectPath + " chefzServingSize");
		if (servingSize <= 0)
		{
			servingSize = 1;
		}

		return amount / servingSize;
	}

	// Der Pharmacologist-Modifikator, wortgleich zu
	// TerjeMedicine/Scripts/4_World/Classes/TerjeConsumableEffects.c:6-17 und
	// TerjeRadiation/Scripts/4_World/Classes/TerjeConsumableEffects.c:6-17.
	// Bewusst dieselbe Rechnung: ein Kraeutertee soll sich fuer den Perk genau
	// so verhalten wie jedes andere Terje-Consumable, nicht anders.
	float GetPharmacologistTimeModifier(PlayerBase player)
	{
		float timeModifier;
		if (player && player.GetTerjeSkills() && player.GetTerjeSkills().GetPerkValue("med", "pharmac", timeModifier))
		{
			return 1.0 + timeModifier;
		}

		return 1.0;
	}
}

// Zugriffsform wie bei GetTerjeGameConfig()
// (TerjeCore/Scripts/3_Game/TerjeGameConfig.c:443-451) - eine freie Funktion,
// damit die Aufrufstellen unveraendert bleiben. Die Referenz selbst liegt
// jedoch NICHT mehr auf Dateiebene, sondern als statischer Member in der
// Klasse (siehe ChefZ_TerjeMedRegistry.Get()): "ref" gehoert an einen Member.
// Diese Funktion haelt keinen eigenen Zustand mehr, sie reicht nur durch.
ChefZ_TerjeMedRegistry GetChefZTerjeMedRegistry()
{
	return ChefZ_TerjeMedRegistry.Get();
}
#endif // TERJE_MEDICINE_MOD
