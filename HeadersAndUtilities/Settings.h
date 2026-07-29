#pragma once
#include "..\IniReader\IniReader.h"
extern inline CIniReader iniReader("Settings.ini");
struct IniSettings {
	bool EnableLog = iniReader.ReadBoolean("Settings", "EnableLogs", true);
	bool ResetLog = iniReader.ReadBoolean("Settings", "ResetLogOnStartup", true);
	std::wstring LogName = L"Log.log";
};
extern IniSettings Settings;