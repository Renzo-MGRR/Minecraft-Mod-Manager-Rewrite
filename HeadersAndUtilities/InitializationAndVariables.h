#pragma once
#include <vector>
#include <string>
#include "..\nlohmann\json.hpp"
#include "..\bit7z\bitarchivereader.hpp"
#include "..\bit7z\bitfileextractor.hpp"
#include "..\ImGui\imgui.h"
#include "..\ImGui\imfilebrowser.h"
#include "..\ImGui\imgui_stdlib.h"
#include "Settings.h"
using json = nlohmann::json;
extern inline bool DisableProfileSelection = false;
extern inline bool DisableServerSelection = false;
extern std::wstring CurrentTask;
extern bit7z::Bit7zLibrary lib;
extern bit7z::BitFileExtractor SevenZip;
extern inline std::vector<std::wstring> AllObjectTypes = { L"Mods", L"Resource Packs", L"Shader Packs" };
extern inline std::wstring wsAllObjectTypes = L"Mods,Resource Packs,Shader Packs";
namespace fs = std::filesystem;
extern inline int FileBrowserFlags = ImGuiFileBrowserFlags_MultipleSelection | ImGuiFileBrowserFlags_CloseOnEsc;
struct Object {
	std::wstring FileName;
	std::wstring Name;
	bool IsEnabled;
	std::wstring Authors;
	std::wstring Description;
	std::wstring Version;
};
struct Version {
	std::string Id;
	std::string Name;
	std::string VersionNumber;
	std::string FileURL;
	std::vector<std::string> GameVersions;
};
struct Project {
	std::string ProjectID;
	std::string Title;
	std::wstring Description;
	std::string Icon;
	std::string VersionsLink;
	json VersionsJson;
	int SelectedVersionIndex;
	std::vector<Version> Versions;
	bool GetInstalled;
	bool IsAlreadyInstalled;
	bool IsDependency;
};
extern std::atomic<float> DownloadProgress;
struct ObjectStruct
{
	std::vector<Object> ObjectVector;
	ObjectType ObjectType;
	bool ShowModrinthTab;
};
struct Profile {
	std::string Id;
	std::wstring Name;
	std::wstring Version;
	std::wstring NameAndVersion;
	std::wstring WsDate;
	std::wstring Directory;
	std::wstring ModLoader;
	json JsonObject;
	std::vector<ObjectStruct> ObjectStructVector; //We store all object data into a vector of object structs
	std::chrono::system_clock::time_point Date;
	std::vector<ImGui::FileBrowser> FileBrowserVector;
	std::vector<Project> ModrinthProjects;
	std::vector<Project> ModrinthProjectsToInstall;
	int SearchOffset;
};
extern std::vector<Profile> Profiles;
extern std::vector<Profile> Servers;
extern std::wstring MinecraftDir;
extern std::wstring ProfilesFile;
extern int ProfileIndex;
extern int ServerIndex;
extern json ProfilesJsonData;