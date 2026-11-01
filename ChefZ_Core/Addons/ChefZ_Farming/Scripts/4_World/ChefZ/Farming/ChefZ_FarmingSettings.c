//==============================================================================
// ChefZ_FarmingSettings - Zeiten der Zucht, einstellbar in $profile:ChefZ
//
// Die Datei $profile:ChefZ\Farming.json legt der Betreiber fest:
//
//   HoneySecondsPerFrame   Sekunden, bis ein Raehmchen im Bienenstock voll ist.
//   MushroomGrowSeconds    Sekunden, bis die Pilze im Anzuchtkasten stehen.
//                          0 = jede Art behaelt ihre Zeit aus der Config
//                          (chefzGrowSeconds, zwei bis acht Stunden).
//
// Eigene Datei statt Overlay-Record: die Werte gehoeren zu diesem Modul, und der
// Core kennt keine Inhalte (Invariante I3). Erklaerung in Farming.README.txt,
// weil JSON keine Kommentare kennt. Eine vorhandene Datei wird nie ueberschrieben.
//
// Gelesen wird einmal je Serverstart, und nur auf dem Server: der Fortschritt
// entsteht dort, der Client bekommt nur die Menge. Eine Aenderung gilt also ab
// dem naechsten Start, und laufende Zucht rechnet dann mit der neuen Zeit weiter.
//
// Fehlt ein Feld oder steht dort eine Zahl <= 0, gilt der Standardwert. Das haengt
// bewusst nicht daran, ob der Deserializer den Konstruktor ausfuehrt: ein fehlendes
// Feld kommt so oder so als Standardwert oder als 0 an.
//
// Layer: 4_World.
//==============================================================================

//! Genau der Inhalt von Farming.json. Die Feldnamen sind die JSON-Schluessel.
class ChefZ_FarmingSettingsData
{
	float HoneySecondsPerFrame;
	float MushroomGrowSeconds;

	void ChefZ_FarmingSettingsData()
	{
		HoneySecondsPerFrame = ChefZ_FarmingSettings.DEFAULT_HONEY_SECONDS_PER_FRAME;
		MushroomGrowSeconds = ChefZ_FarmingSettings.DEFAULT_MUSHROOM_GROW_SECONDS;
	}
}

class ChefZ_FarmingSettings
{
	//! Vier Stunden je Raehmchen. Zehn Raehmchen sind damit vierzig Stunden -
	//! die laengste Wartezeit des Projekts, und sie soll es sein: Honig ist der
	//! einzige Suessstoff der Kette.
	static const float DEFAULT_HONEY_SECONDS_PER_FRAME = 14400.0;

	//! 0 = keine Vorgabe, jede Pilzart nimmt ihre eigene Zeit aus der Config.
	static const float DEFAULT_MUSHROOM_GROW_SECONDS = 0.0;

	private static bool s_Loaded;
	private static float s_HoneySecondsPerFrame = DEFAULT_HONEY_SECONDS_PER_FRAME;
	private static float s_MushroomGrowSeconds = DEFAULT_MUSHROOM_GROW_SECONDS;

	static string SettingsFile()
	{
		return ChefZ_ProfilePaths.Root() + "\\Farming.json";
	}

	static string ReadmeFile()
	{
		return ChefZ_ProfilePaths.Root() + "\\Farming.README.txt";
	}

	static float HoneySecondsPerFrame()
	{
		EnsureLoaded();
		return s_HoneySecondsPerFrame;
	}

	//! 0 = keine Vorgabe, der Aufrufer nimmt die Zeit der Pilzart.
	static float MushroomGrowSeconds()
	{
		EnsureLoaded();
		return s_MushroomGrowSeconds;
	}

	//! Einmal je Serverstart. Ein Client liest nichts und legt nichts an: sein
	//! $profile: ist %localappdata%\dayz.
	static void EnsureLoaded()
	{
		if (s_Loaded)
			return;
		if (!g_Game || !g_Game.IsServer())
			return;
		s_Loaded = true;

		MakeDirectory(ChefZ_ProfilePaths.Root());
		WriteMissingFiles();
		ReadSettings();

		ChefZ_Log.Banner("Farming.json: HoneySecondsPerFrame=" + s_HoneySecondsPerFrame.ToString() + " MushroomGrowSeconds=" + s_MushroomGrowSeconds.ToString());
	}

	private static void WriteMissingFiles()
	{
		string path = SettingsFile();
		if (!FileExist(path))
		{
			ChefZ_FarmingSettingsData defaults = new ChefZ_FarmingSettingsData();
			string saveError;
			if (!JsonFileLoader<ChefZ_FarmingSettingsData>.SaveFile(path, defaults, saveError))
				ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, "Farming.json konnte nicht angelegt werden: " + saveError);
		}

		string readme = ReadmeFile();
		if (FileExist(readme))
			return;

		FileHandle fh = OpenFile(readme, FileMode.WRITE);
		if (fh == 0)
			return;

		FPrintln(fh, "ChefZ - Farming.json");
		FPrintln(fh, "====================");
		FPrintln(fh, "");
		FPrintln(fh, "Alle Zeiten in Sekunden Serverlaufzeit. Steht der Server, vergeht keine Zeit.");
		FPrintln(fh, "Aenderungen gelten ab dem naechsten Serverstart.");
		FPrintln(fh, "");
		FPrintln(fh, "  HoneySecondsPerFrame  Bis ein Raehmchen im Bienenstock voll ist.");
		FPrintln(fh, "                        Standard 14400 (4 Stunden). Die Raehmchen fuellen sich");
		FPrintln(fh, "                        nacheinander, zehn Raehmchen dauern also das Zehnfache.");
		FPrintln(fh, "");
		FPrintln(fh, "  MushroomGrowSeconds   Bis die Pilze im Anzuchtkasten stehen, fuer ALLE Arten.");
		FPrintln(fh, "                        Standard 0: jede Art behaelt ihre eigene Zeit");
		FPrintln(fh, "                        (2 bis 8 Stunden).");
		FPrintln(fh, "");
		FPrintln(fh, "Der Takt laeuft alle 10 Sekunden; kuerzer als das geht es nicht.");
		FPrintln(fh, "Eine fehlende Zahl oder ein Wert kleiner gleich 0 ergibt den Standardwert.");
		CloseFile(fh);
	}

	private static void ReadSettings()
	{
		ChefZ_FarmingSettingsData data = new ChefZ_FarmingSettingsData();
		string loadError;
		if (!JsonFileLoader<ChefZ_FarmingSettingsData>.LoadFile(SettingsFile(), data, loadError))
		{
			ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, "Farming.json nicht lesbar, es gelten die Standardwerte: " + loadError);
			return;
		}

		if (data.HoneySecondsPerFrame > 0.0)
			s_HoneySecondsPerFrame = data.HoneySecondsPerFrame;

		if (data.MushroomGrowSeconds > 0.0)
			s_MushroomGrowSeconds = data.MushroomGrowSeconds;
	}
}
