#pragma once
#include <thread>
#include "..\HeadersAndUtilities\Functions.h"
#include "..\HeadersAndUtilities\Settings.h"
std::wstring CurrentTask = L"";
std::wstring MinecraftDir = L"";
std::wstring ProfilesFile = L"";
std::vector<Profile> Profiles;
int ProfileIndex = 0;
bit7z::Bit7zLibrary lib{ "7z.dll" };
bit7z::BitFileExtractor SevenZip{ lib, bit7z::BitFormat::Zip };
void Initialization()
{
	std::thread([=]()
	{
		MinecraftDir = GetAppData() + L"\\.minecraft";
		ProfilesFile = MinecraftDir + L"\\launcher_profiles.json";
		Settings.GlobalObjectTypes = GetAvailableObjectTypes(iniReader);
		Profiles = GetProfiles();
		CleanLog();
	}).detach();
}