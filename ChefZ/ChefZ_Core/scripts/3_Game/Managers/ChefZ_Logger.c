class ChefZ_Logger
{
	protected static int s_ChefZ_LastLogTime;
	protected static bool s_ChefZ_DirectoriesEnsured;
	protected static string s_ChefZ_CurrentLogFile;
	protected static string s_ChefZ_CurrentLogFileName;
	protected static string s_ChefZ_CurrentDebugLogFile;
	protected static string s_ChefZ_CurrentDebugLogFileName;
	protected static bool s_ChefZ_OldLogsCleanedUp;
	protected static bool s_ChefZ_OldDebugLogsCleanedUp;
	protected static bool s_ChefZ_Configured;
	protected static int s_ChefZ_EnableLogging;
	protected static int s_ChefZ_EnableDebugLogging;
	protected static int s_ChefZ_KeepLogs;

	static void Configure(int enableLogging, int enableDebugLogging, int keepLogs)
	{
		if (enableLogging != 0)
			enableLogging = 1;

		if (enableDebugLogging != 1)
			enableDebugLogging = 0;

		if (keepLogs < 1)
			keepLogs = ChefZ_CoreConstants.DEFAULT_KEEP_LOGS;

		s_ChefZ_EnableLogging = enableLogging;
		s_ChefZ_EnableDebugLogging = enableDebugLogging;
		s_ChefZ_KeepLogs = keepLogs;
		s_ChefZ_Configured = true;
	}

	static void Log(string message, bool bypassRateLimit = false)
	{
		if (!g_Game)
			return;

		if (!IsLogsEnabled())
			return;

		EnsureDirectoriesExist();

		if (s_ChefZ_CurrentLogFile == string.Empty)
		{
			s_ChefZ_CurrentLogFileName = ChefZ_CoreConstants.LOG_FILE_PREFIX + GetTimestampForFile() + ".log";
			s_ChefZ_CurrentLogFile = ChefZ_CoreConstants.GetLogsPath(s_ChefZ_CurrentLogFileName);
			CleanupOldLogs(ChefZ_CoreConstants.LOG_FILE_PREFIX, s_ChefZ_CurrentLogFileName, true);
		}

		int currentTime = g_Game.GetTickTime() * 1000;
		if (!bypassRateLimit && currentTime - s_ChefZ_LastLogTime < ChefZ_CoreConstants.LOG_INTERVAL_MS)
			return;

		s_ChefZ_LastLogTime = currentTime;
		WriteLine(s_ChefZ_CurrentLogFile, message);
	}

	static void LogDebug(string message)
	{
		if (!g_Game)
			return;

		if (!IsDebugEnabled())
			return;

		EnsureDirectoriesExist();

		if (s_ChefZ_CurrentDebugLogFile == string.Empty)
		{
			s_ChefZ_CurrentDebugLogFileName = ChefZ_CoreConstants.DEBUG_LOG_FILE_PREFIX + GetTimestampForFile() + ".log";
			s_ChefZ_CurrentDebugLogFile = ChefZ_CoreConstants.GetLogsPath(s_ChefZ_CurrentDebugLogFileName);
			CleanupOldLogs(ChefZ_CoreConstants.DEBUG_LOG_FILE_PREFIX, s_ChefZ_CurrentDebugLogFileName, false);
		}

		WriteLine(s_ChefZ_CurrentDebugLogFile, message);
	}

	static void LogError(string message)
	{
		Log("[ERROR] " + message, true);
		Print("[ChefZ] " + message);
	}

	static void LogWarning(string message)
	{
		Log("[WARNING] " + message, true);
	}

	static void EnsureDirectoriesExist()
	{
		if (s_ChefZ_DirectoriesEnsured)
			return;

		ChefZ_CoreConstants.EnsureLogsDirectory();
		s_ChefZ_DirectoriesEnsured = true;
	}

	protected static bool IsLogsEnabled()
	{
		if (!s_ChefZ_Configured)
			return ChefZ_CoreConstants.DEFAULT_ENABLE_LOGGING != 0;

		return s_ChefZ_EnableLogging != 0;
	}

	protected static bool IsDebugEnabled()
	{
		if (!s_ChefZ_Configured)
			return ChefZ_CoreConstants.DEFAULT_ENABLE_DEBUG_LOGGING == 1;

		return s_ChefZ_EnableDebugLogging == 1;
	}

	protected static int GetKeepLogCount()
	{
		if (!s_ChefZ_Configured || s_ChefZ_KeepLogs < 1)
			return ChefZ_CoreConstants.DEFAULT_KEEP_LOGS;

		return s_ChefZ_KeepLogs;
	}

	protected static void WriteLine(string filePath, string message)
	{
		FileHandle logFile = OpenFile(filePath, FileMode.APPEND);
		if (logFile == 0)
		{
			Print("[ChefZ] ERROR: Failed to open log file: " + filePath);
			return;
		}

		FPrintln(logFile, "[" + GetDateAndTime() + "] [" + GetSideLabel() + "] " + message);
		CloseFile(logFile);
	}

	protected static string GetSideLabel()
	{
		if (!g_Game)
			return "Unknown";

		if (g_Game.IsDedicatedServer())
			return "Server";

		if (g_Game.IsClient())
			return "Client";

		if (g_Game.IsServer())
			return "Server";

		return "Unknown";
	}

	protected static string GetDateAndTime()
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		return year.ToString() + "-" + FormatWithLeadingZero(month) + "-" + FormatWithLeadingZero(day) + " " + FormatWithLeadingZero(hour) + ":" + FormatWithLeadingZero(minute) + ":" + FormatWithLeadingZero(second);
	}

	protected static string GetTimestampForFile()
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		return FormatWithLeadingZero(day) + "_" + FormatWithLeadingZero(month) + "_" + year.ToString() + "_" + FormatWithLeadingZero(hour) + "_" + FormatWithLeadingZero(minute) + "_" + FormatWithLeadingZero(second);
	}

	protected static string FormatWithLeadingZero(int value)
	{
		if (value < 10)
			return "0" + value.ToString();

		return value.ToString();
	}

	protected static void CleanupOldLogs(string filePrefix, string currentFileName, bool isMainLog)
	{
		if (isMainLog)
		{
			if (s_ChefZ_OldLogsCleanedUp)
				return;

			s_ChefZ_OldLogsCleanedUp = true;
		}
		else
		{
			if (s_ChefZ_OldDebugLogsCleanedUp)
				return;

			s_ChefZ_OldDebugLogsCleanedUp = true;
		}

		int keepCount = GetKeepLogCount();
		if (keepCount <= 0)
			return;

		TStringArray logFiles = CollectLogFiles(filePrefix);
		if (!logFiles || logFiles.Count() == 0)
			return;

		TStringArray sortedFiles = SortLogFilesOldestFirst(logFiles);
		int filesToDelete = sortedFiles.Count() - keepCount + 1;
		if (filesToDelete <= 0)
			return;

		string logsFolder = ChefZ_CoreConstants.GetLogsPath("");
		int deletedCount = 0;

		for (int i = 0; i < sortedFiles.Count() && deletedCount < filesToDelete; i++)
		{
			string fileName = sortedFiles.Get(i);
			if (fileName == currentFileName)
				continue;

			DeleteFile(logsFolder + fileName);
			deletedCount++;
		}
	}

	protected static TStringArray CollectLogFiles(string filePrefix)
	{
		TStringArray logFiles = new TStringArray;
		string fileName;
		FileAttr fileAttr;
		string logsFolder = ChefZ_CoreConstants.GetLogsPath("");
		FindFileHandle findHandle = FindFile(logsFolder + filePrefix + "*.log", fileName, fileAttr, 0);
		if (!findHandle)
			return logFiles;

		while (fileName != string.Empty)
		{
			bool skipDebugFile = false;
			if (filePrefix == ChefZ_CoreConstants.LOG_FILE_PREFIX)
			{
				if (fileName.IndexOf(ChefZ_CoreConstants.DEBUG_LOG_FILE_PREFIX) == 0)
					skipDebugFile = true;
			}

			if (!skipDebugFile)
				logFiles.Insert(fileName);

			if (!FindNextFile(findHandle, fileName, fileAttr))
				break;
		}

		CloseFindFile(findHandle);
		return logFiles;
	}

	protected static TStringArray SortLogFilesOldestFirst(TStringArray fileNames)
	{
		TStringArray sortedEntries = new TStringArray;
		TStringArray sortedFiles = new TStringArray;

		for (int i = 0; i < fileNames.Count(); i++)
		{
			string fileName = fileNames.Get(i);
			sortedEntries.Insert(GetLogFileSortKey(fileName) + "|" + fileName);
		}

		sortedEntries.Sort();

		for (int j = 0; j < sortedEntries.Count(); j++)
		{
			string entry = sortedEntries.Get(j);
			int separatorIndex = entry.IndexOf("|");
			if (separatorIndex < 0)
				continue;

			sortedFiles.Insert(entry.Substring(separatorIndex + 1, entry.Length() - separatorIndex - 1));
		}

		return sortedFiles;
	}

	protected static string GetLogFileSortKey(string fileName)
	{
		TStringArray parts = new TStringArray;
		string parsedName = fileName;
		parsedName.Replace(".log", "");
		parsedName.Split("_", parts);

		if (parts.Count() < 7)
			return parsedName;

		int dayIndex = 1;
		if (fileName.IndexOf(ChefZ_CoreConstants.DEBUG_LOG_FILE_PREFIX) >= 0)
			dayIndex = 2;

		string year = parts.Get(dayIndex + 2);
		string month = parts.Get(dayIndex + 1);
		string day = parts.Get(dayIndex);
		string hour = parts.Get(dayIndex + 3);
		string minute = parts.Get(dayIndex + 4);
		string second = "00";

		if (parts.Count() > dayIndex + 5)
			second = parts.Get(dayIndex + 5);

		return year + month + day + hour + minute + second;
	}
}
