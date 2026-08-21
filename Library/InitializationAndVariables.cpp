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
bool datecmp(Profile Profile1, Profile Profile2)
{
	return Profile1.Date > Profile2.Date;
}
void Initialization()
{	
	std::thread([=]()
	{
		MinecraftDir = GetAppData() + L"\\.minecraft";
		ProfilesFile = MinecraftDir + L"\\launcher_profiles.json";
		Settings.GlobalObjectTypes = GetAvailableObjectTypes(iniReader);
		GetProfiles(Profiles);
		std::sort(Profiles.begin(), Profiles.end(), datecmp);
		CleanLog();
	}).detach();
}