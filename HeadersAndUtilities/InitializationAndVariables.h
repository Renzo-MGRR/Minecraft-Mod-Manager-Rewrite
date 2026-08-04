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
extern std::wstring CurrentTask;
extern bit7z::Bit7zLibrary lib;
extern bit7z::BitFileExtractor SevenZip;
extern inline std::vector<std::wstring> AllObjectTypes = { L"Mods", L"Resource Packs", L"Shader Packs" };
extern inline std::wstring wsAllObjectTypes = L"Mods,Resource Packs,Shader Packs";
using json = nlohmann::json;
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
struct ObjectStruct
{
	std::vector<Object> ObjectVector;
	ObjectType ObjectType;
};
struct Profile {
	std::wstring Id;
	std::wstring Name;
	std::wstring Version;
	std::wstring NameAndVersion;
	std::wstring WsDate;
	std::wstring Directory;
	std::wstring ModLoader;
	std::wstring JsonObject;
	std::vector<ObjectStruct> ObjectStruct; //We store all objects into a vector of vectors
	//int Date;
	std::vector<ImGui::FileBrowser> FileBrowserVector;
};
extern std::vector<Profile> Profiles;
extern std::wstring MinecraftDir;
extern std::wstring ProfilesFile;
extern int ProfileIndex;
extern json ProfilesJsonData;