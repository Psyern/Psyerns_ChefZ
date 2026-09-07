class ChefZ_CoreConstants
{
	static const string PROFILE_FOLDER = "ChefZ_Config";
	static const string SERVER_CONFIG_FILE = "Server_Config.json";
	static const string CLIENT_CONFIG_FILE = "Client_Config.json";
	static const string LOGS_FOLDER = "Logs";
	static const string LOG_FILE_PREFIX = "ChefZ_";
	static const string DEBUG_LOG_FILE_PREFIX = "ChefZ_Debug_";
	static const int CONFIG_VERSION = 1;
	static const int DEFAULT_ENABLE_LOGGING = 1;
	static const int DEFAULT_ENABLE_DEBUG_LOGGING = 1;
	static const int DEFAULT_KEEP_LOGS = 7;
	static const int LOG_INTERVAL_MS = 5000;

	static string GetProfilePath(string fileName)
	{
		return "$profile:" + PROFILE_FOLDER + "/" + fileName;
	}

	static string GetServerConfigPath()
	{
		return GetProfilePath(SERVER_CONFIG_FILE);
	}

	static string GetClientConfigPath()
	{
		return GetProfilePath(CLIENT_CONFIG_FILE);
	}

	static string GetLogsPath(string fileName)
	{
		return "$profile:" + PROFILE_FOLDER + "/" + LOGS_FOLDER + "/" + fileName;
	}

	static void EnsureProfileDirectory()
	{
		string profileFolder = "$profile:" + PROFILE_FOLDER + "/";
		if (!FileExist(profileFolder))
			MakeDirectory(profileFolder);
	}

	static void EnsureLogsDirectory()
	{
		EnsureProfileDirectory();

		string logsFolder = GetLogsPath("");
		if (!FileExist(logsFolder))
			MakeDirectory(logsFolder);
	}
}
