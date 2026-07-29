#pragma once
#include <vector>
#include <string>
#include "..\nlohmann\json.hpp"
#include "..\bit7z\bitarchivereader.hpp"
#include "..\bit7z\bitfileextractor.hpp"
#include "..\ImGui\imgui.h"
#include "..\ImGui\imfilebrowser.h"
#include "..\ImGui\imgui_stdlib.h"
extern std::wstring CurrentTask;
extern bit7z::Bit7zLibrary lib;
extern bit7z::BitFileExtractor SevenZip;
using json = nlohmann::json;
namespace fs = std::filesystem;
extern inline int FileBrowserFlags = ImGuiFileBrowserFlags_MultipleSelection | ImGuiFileBrowserFlags_CloseOnEsc;
struct Object {
	std::wstring FileName;
	std::wstring Name;
	bool IsEnabled = true;
	std::wstring Authors;
	std::wstring Description;
	std::wstring Version;
};
struct ProfileDirectories {
	std::wstring ProfileDir;
	std::wstring DisabledModsDir;
	std::wstring DisabledResourcePacksDir;
	std::wstring DisabledShaderPacksDir;
	std::wstring ModsDir;
	std::wstring ResourcePacksDir;
	std::wstring ShaderPacksDir;
};
struct ObjectStruct
{
	std::vector<Object> ObjectVector;
	std::wstring ObjectType; //Objects can be Mods, Resource Packs or Shaders
};
struct Profile {
	std::wstring Id;
	std::wstring Name;
	std::wstring Version;
	std::wstring NameAndVersion;
	std::wstring WsDate;
	ProfileDirectories ProfileDirs;
	std::wstring ModLoader;
	std::vector<ObjectStruct> ObjectStruct; //We store all objects into a vector of vectors
	int Date = 0;
	std::vector<ImGui::FileBrowser> FileBrowserVector;
};
extern std::vector<Profile> Profiles;
extern std::wstring MinecraftDir;
extern std::wstring ProfilesFile;
extern int ProfileIndex;