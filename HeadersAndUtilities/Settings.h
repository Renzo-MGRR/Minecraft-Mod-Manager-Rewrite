#pragma once
#include "..\IniReader\IniReader.h"
extern inline CIniReader iniReader("Settings.ini");
struct ObjectType {
	std::wstring Type;
	bool IsEnabled;
};
struct IniSettings {
	bool EnableLog = iniReader.ReadBoolean("Settings", "EnableLog", true);
	bool ResetLog = iniReader.ReadBoolean("Settings", "ResetLogOnStartup", true);
	std::wstring LogName = L"Log.log";
	std::vector<ObjectType> GlobalObjectTypes;
};
extern IniSettings Settings;