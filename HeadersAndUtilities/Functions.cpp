#include <thread>
#include <string>
#include <fstream>
#include <ShlObj.h>
#include <curl/curl.h>
#include <vector>
#include <filesystem>
#include "Shlwapi.h"
#include "Settings.h"
#include "InitializationAndVariables.h"

namespace fs = std::filesystem;
std::wstring string2wstring(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string wstring2string(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

bool IsTypeEnabled(std::wstring& Type, std::vector<ObjectType> ObjectTypesVector)
{
	for (int i = 0; i < ObjectTypesVector.size(); i++)
	{
		if (ObjectTypesVector[i].Type == Type)
		{
			return true;
		}
	}
	return false;
}

std::vector<ObjectType> GetAvailableObjectTypes(CIniReader& IniReaderObject)
{
	std::wstring wsObjectTypes = string2wstring(IniReaderObject.ReadString("Settings", "EnabledObjectTypes", wstring2string(wsAllObjectTypes)));
	int PreviousSeparatorIndex = 0;
	ObjectType TempObjectType;
	std::vector<ObjectType> ObjectTypesVector;
	for (int i = 0; i < wsObjectTypes.size(); i++)
	{
		if (wsObjectTypes[i] == L',')
		{
			if (PreviousSeparatorIndex == 0)
			{
				TempObjectType.Type = wsObjectTypes.substr(PreviousSeparatorIndex, i - PreviousSeparatorIndex);
				ObjectTypesVector.push_back(TempObjectType);
			}
			else
			{
				TempObjectType.Type = wsObjectTypes.substr(PreviousSeparatorIndex + 1, i - PreviousSeparatorIndex - 1); //We make it not include the , character
				ObjectTypesVector.push_back(TempObjectType);
			}
			PreviousSeparatorIndex = i;
		}
		if (i == wsObjectTypes.size() - 1)
		{
			TempObjectType.Type = wsObjectTypes.substr(PreviousSeparatorIndex + 1, i - PreviousSeparatorIndex);
			ObjectTypesVector.push_back(TempObjectType);
		}
	}
	std::vector<ObjectType> AvailableObjectTypesVector;
	for (int i = 0; i < AllObjectTypes.size(); i++)
	{
		ObjectType NewObjectType;
		NewObjectType.Type = AllObjectTypes[i];
		NewObjectType.IsEnabled = IsTypeEnabled(NewObjectType.Type, ObjectTypesVector);
		AvailableObjectTypesVector.push_back(NewObjectType);
	}
	return AvailableObjectTypesVector;
}

std::wstring GetCurrentFolder()
{
	wchar_t CurrentFolder[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, CurrentFolder);
	std::wstring WStrCurrentDirectory = CurrentFolder;
	return WStrCurrentDirectory;
}

void WriteToLog(const std::wstring& LoggingLine)
{
	if (Settings.EnableLog)
	{
		std::wofstream Log(GetCurrentFolder() + L"\\" + Settings.LogName, std::ios_base::app);
		auto CurrentTimeA = std::chrono::system_clock::now();
		auto localzone = std::chrono::current_zone();
		auto LocalTime = std::chrono::zoned_time{ localzone, CurrentTimeA };
		std::wstring formatedTime = std::format(L"{:%Y-%m-%d %H:%M:%S}", LocalTime);
		formatedTime.insert(0, L"[");
		formatedTime.insert(formatedTime.size(), L"]");
		std::wstring HourAndLogLine = formatedTime + L" " + LoggingLine;
		Log << std::setw(4) << HourAndLogLine << std::endl;
		Log.close();
	}
}

bool IsFileAccessible(const std::string& filename)
{
	std::ifstream file(filename);
	return file.good();
}

static size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream) { //Writes file to a void pointer
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

struct DownloadStatus {
	std::atomic<float> progress{ 0.0f };
	std::atomic<bool> isDownloading{ false };
	std::wstring currentFileName;
	std::atomic<bool> isCopying{ false };
};
DownloadStatus g_Status;

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	if (dltotal > 0) {
		g_Status.progress = (float)dlnow / (float)dltotal;
	}
	return 0;
}

std::vector<std::wstring> getInDirectoryW(const std::wstring& directory, bool getFolder)
{
	std::vector<std::wstring> files;
	std::wstring searchPath = directory + L"\\*";

	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return files;

	do {
		bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if (isDir == getFolder &&
			wcscmp(fd.cFileName, L".") != 0 &&
			wcscmp(fd.cFileName, L"..") != 0)
		{
			files.push_back(fd.cFileName);
		}
	} while (FindNextFileW(hFind, &fd));

	FindClose(hFind);

	return files;
}

std::vector<std::wstring> getAllInDirectoryW(const std::wstring& directory)
{
	std::vector<std::wstring> files;
	std::wstring searchPath = directory + L"\\*";

	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return files;

	do {
		if (wcscmp(fd.cFileName, L".") != 0 &&
			wcscmp(fd.cFileName, L"..") != 0)
		{
			files.push_back(fd.cFileName);
		}
	} while (FindNextFileW(hFind, &fd));

	FindClose(hFind);

	return files;
}

void DownloadFile(std::wstring file, std::string url)
{
	if (fs::exists(file))
	{
		CURL* curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			FILE* fp = _wfopen((file).c_str(), L"wb");
			if (fp == NULL) {
				perror("File open failed");
				return;
			}
			if (fp) {
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
				CURLcode res = curl_easy_perform(curl);
				g_Status.isDownloading = true;
				g_Status.currentFileName = file;
				if (res != CURLE_OK) {
					if (fs::exists(file))
					{
						fs::remove(file);
					}
				}
				fclose(fp);
			}
			curl_easy_cleanup(curl);
		}
	}
}

std::vector<bit7z::byte_t> ExtractToMemory(std::wstring extractFrom, std::wstring fileToGet)
{
	bit7z::Bit7zLibrary lib{ "7z.dll" };
	std::vector<bit7z::byte_t> Buffer;
	bit7z::BitFileExtractor extractor{ lib, bit7z::BitFormat::Zip };
	try {
		extractor.extractMatching(wstring2string(extractFrom), wstring2string(fileToGet), Buffer);
	}
	catch (const bit7z::BitException& except)
	{
		WriteToLog(string2wstring(except.what()));
	}
	return Buffer;
}

void ExtractToDisk(std::wstring extractFrom, std::wstring fileToGet)
{
	bit7z::Bit7zLibrary lib{ "7z.dll" };
	bit7z::BitFileExtractor extractor{ lib, bit7z::BitFormat::Zip };
	try {
		extractor.extract(wstring2string(extractFrom), wstring2string(fileToGet));
	}
	catch (const bit7z::BitException& except)
	{
		WriteToLog(string2wstring(except.what()));
	}
}

void DownloadBar()
{
	if (g_Status.isDownloading) {
		ImGui::Text("%s", wstring2string(g_Status.currentFileName).c_str());
		ImGui::ProgressBar(g_Status.progress);
	}
}

void Combo(std::vector<std::wstring> Vector, int id, int VectorSelectedIndex)
{
	if (ImGui::BeginCombo((wstring2string(L"##Combo" + std::to_wstring(id))).c_str(), (wstring2string(Vector[VectorSelectedIndex])).c_str()))
	{
		for (int i = 0; i < Vector.size(); ++i)
		{
			const bool isSelected = (VectorSelectedIndex == i);
			if (ImGui::Selectable((wstring2string(Vector[i])).c_str(), isSelected))
			{
				VectorSelectedIndex = i;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void ProfileCombo(std::vector<Profile> Vector, int id, int VectorSelectedIndex)
{
	if (ImGui::BeginCombo((wstring2string(L"##Combo" + std::to_wstring(id))).c_str(), (wstring2string(Vector[VectorSelectedIndex].NameAndVersion)).c_str()))
	{
		for (int i = 0; i < Vector.size(); ++i)
		{
			const bool isSelected = (VectorSelectedIndex == i);
			if (ImGui::Selectable((wstring2string(Vector[i].NameAndVersion)).c_str(), isSelected))
			{
				VectorSelectedIndex = i;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

bool TextInput(std::wstring Description, std::string& StringHandler)
{
	ImGui::Text("%s", (wstring2string(Description)).c_str());
	ImGui::SameLine();
	if (ImGui::InputText("", &StringHandler, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Execute(const std::wstring& cmdLine, const std::wstring& RunFrom, bool Silent, bool WaitTillFinish)
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	STARTUPINFOW si = { 0 };
	DWORD dwCreationFlags = 0;

	if (Silent)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		dwCreationFlags |= CREATE_NO_WINDOW;
	}
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	LPWSTR CmdL = const_cast<LPWSTR>(cmdLine.c_str());
	if (CreateProcessW(
		nullptr,
		CmdL,
		NULL,
		NULL,
		TRUE,
		dwCreationFlags,
		NULL,
		RunFrom.c_str(),
		&si,
		&pi) == 0)
	{
		DWORD Error = GetLastError();
		WriteToLog(L"Error code:" + std::to_wstring(GetLastError()));
	}
	if (WaitTillFinish)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

std::wstring GetAppData()
{
	wchar_t appdata[MAX_PATH];
	GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
	return appdata;
}
bool ProfilesFound(std::wstring ProfilesFile) 
{
	if (fs::exists(ProfilesFile) && !fs::is_empty(ProfilesFile)) // We check if launcher_profiles.json exists
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ParseVersion(Profile& CurrentProfile, json& ProfileJson)
{
	CurrentProfile.Version = string2wstring(ProfileJson.value("lastVersionId", ""));
	if (CurrentProfile.Version.find(L"fabric") != std::wstring::npos)
	{
		CurrentProfile.ModLoader = L"Fabric";
	}
	else if (CurrentProfile.Version.find(L"forge") != std::wstring::npos)
	{
		CurrentProfile.ModLoader = L"Forge";
	}
	else
	{
		return false; //We skip non-modded profiles
	}
	if (CurrentProfile.Version.find(L"+build") != std::wstring::npos) //Trim down to only game version
	{
		CurrentProfile.Version.erase(0, 31);
	}
	else
	{
		CurrentProfile.Version.erase(0, 21); 
	}
	return true;
};

std::wstring CombineNameAndVersion(Profile& CurrentProfile)
{
	std::wstring NameAndVersion = L"";
	NameAndVersion = CurrentProfile.Name;
	NameAndVersion.insert(NameAndVersion.size(), L" (");
	NameAndVersion = NameAndVersion + CurrentProfile.Version;
	NameAndVersion.insert(NameAndVersion.size(), L")");
	// Example: fabric-loader-0.19.2-26.2 (26.2)
	return NameAndVersion;
}

std::wstring JoinAuthorsFromJson(const json& ModJson)
{
	std::vector<std::string> authorStr;
	std::string authorNJ = "";
	if (ModJson.contains("authors") && ModJson["authors"].is_array())
	{
		for (auto& item : ModJson["authors"])
		{
			if (item.is_string())
			{
				authorStr.push_back(item.get<std::string>());
			}
			else if (item.is_object() && item.contains("name"))
			{
				authorStr.push_back(item["name"].get<std::string>());
			}
		}
	}
	for (size_t i = 0; i < authorStr.size(); ++i)
	{
		authorNJ += authorStr[i];
		if (i < authorStr.size() - 1)
		{
			authorNJ += ", ";
		}
	}
	return string2wstring(authorNJ);
}

std::wstring ParseJsonTextArray(json& JsonArray)
{
	std::wstring Description;
	for (auto& [id, item] : JsonArray.items())
	{
		if (!item.is_string())
		{
			Description = Description + string2wstring(item.value("text", ""));
		}
	}
	return Description;
}
std::wstring GetResourcePackDescription(json& JsonObject)
{
	std::wstring Description;
	if (!JsonObject.is_array())
	{
		if (JsonObject.value("fallback", "") == "")
		{
			Description = string2wstring(JsonObject.value("translate", ""));
		}
		else
		{
			Description = string2wstring(JsonObject.value("fallback", ""));
		}
	}
	else
	{
		Description = ParseJsonTextArray(JsonObject);
	}
	return Description;
}

Object GetObjectInfo(std::wstring& ObjectFilename, ProfileDirectories& ProfileDirs, std::wstring& ObjectType, bool Enabled)
{
	Object NewObject;
	NewObject.FileName = ObjectFilename;
	NewObject.IsEnabled = Enabled;
	std::vector<bit7z::byte_t> ObjectBuffer;
	if (ObjectType == L"Resource Packs")
	{
		if (NewObject.IsEnabled)
		{
			SevenZip.extractMatching(wstring2string(ProfileDirs.ResourcePacksDir + L"\\" + NewObject.FileName), "pack.mcmeta", ObjectBuffer);
		}
		else
		{
			SevenZip.extractMatching(wstring2string(ProfileDirs.DisabledResourcePacksDir + L"\\" + NewObject.FileName), "pack.mcmeta", ObjectBuffer);
		}
		json ResourcePackJson = json::parse(ObjectBuffer);
		NewObject.Name = string2wstring(ResourcePackJson["pack"].value("name", ""));
		NewObject.Description = GetResourcePackDescription(ResourcePackJson["pack"]["description"]);
	}
	else if (ObjectType == L"Mods")
	{
		if (NewObject.IsEnabled)
		{
			SevenZip.extractMatching(wstring2string(ProfileDirs.ModsDir + L"\\" + NewObject.FileName), "fabric.mod.json", ObjectBuffer);
		}
		else
		{
			SevenZip.extractMatching(wstring2string(ProfileDirs.DisabledModsDir + L"\\" + NewObject.FileName), "fabric.mod.json", ObjectBuffer);
		}
		json ModJson = json::parse(ObjectBuffer);
		NewObject.Name = string2wstring(ModJson.value("name", ""));
		NewObject.Description = string2wstring(ModJson.value("description", ""));
		NewObject.Version = string2wstring(ModJson.value("version", ""));
		NewObject.Authors = JoinAuthorsFromJson(ModJson);
	}
	else // Shader Packs
	{

	}
	if (NewObject.Name.empty())
	{
		NewObject.Name = NewObject.FileName;
	}
	return NewObject;
}

ObjectStruct GetObjects(ProfileDirectories& ProfileDirs, ObjectType& ObjectType)
{
	CurrentTask = L"Getting profile objects...";
	ObjectStruct Objects;
	Objects.ObjectType = ObjectType;
	std::vector<std::wstring> ObjectFileList;
	std::vector<std::wstring> ObjectDisabledFileList;
	if (Objects.ObjectType.Type == L"Mods")
	{
		ObjectFileList = getInDirectoryW(ProfileDirs.ModsDir, false);
		ObjectDisabledFileList = getInDirectoryW(ProfileDirs.DisabledModsDir, false);
	}
	else if (Objects.ObjectType.Type == L"Resource Packs")
	{
		ObjectFileList = getInDirectoryW(ProfileDirs.ResourcePacksDir, false);
		ObjectDisabledFileList = getInDirectoryW(ProfileDirs.DisabledResourcePacksDir, false);
	}
	else // Shader Packs
	{
		ObjectFileList = getInDirectoryW(ProfileDirs.ShaderPacksDir, false);
		ObjectDisabledFileList = getInDirectoryW(ProfileDirs.DisabledShaderPacksDir, false);
	}
	ObjectFileList.reserve(ObjectFileList.size() + ObjectDisabledFileList.size());
	ObjectFileList.insert(ObjectFileList.end(), ObjectDisabledFileList.begin(), ObjectDisabledFileList.end());
	for (std::wstring& CurrentFile : ObjectFileList)
	{
		Objects.ObjectVector.push_back(GetObjectInfo(CurrentFile, ProfileDirs, Objects.ObjectType.Type, true));
	}
	return Objects;
}

std::vector<ObjectStruct> GetObjectStruct(Profile& CurrentProfile)
{
	CurrentTask = L"Getting profile objects...";
	std::vector<ObjectStruct> ObjectStruct;
	for (ObjectType& CurrentObjectType : Settings.GlobalObjectTypes)
	{
		ObjectStruct.push_back(GetObjects(CurrentProfile.ProfileDirs, CurrentObjectType));
	}
	return ObjectStruct;
}

ProfileDirectories CalculateProfileDirectories(json& ProfileJson)
{
	CurrentTask = L"Calculating profile directories...";
	ProfileDirectories ProfileDirs;
	ProfileDirs.ProfileDir = string2wstring(ProfileJson.value("gameDir", ""));
	ProfileDirs.ModsDir = ProfileDirs.ProfileDir + L"\\mods";
	ProfileDirs.DisabledModsDir = ProfileDirs.ModsDir + L"\\Disabled Mods";
	ProfileDirs.ResourcePacksDir = ProfileDirs.ProfileDir + L"\\resourcepacks";
	ProfileDirs.DisabledResourcePacksDir = ProfileDirs.ResourcePacksDir + L"\\Disabled Resource Packs";
	ProfileDirs.ShaderPacksDir = ProfileDirs.ProfileDir + L"\\shaderpacks";
	ProfileDirs.DisabledShaderPacksDir = ProfileDirs.ShaderPacksDir + L"\\Disabled Shader Packs";
	return ProfileDirs;
}
std::vector<ImGui::FileBrowser> GetFileBrowserVector(Profile& CurrentProfile)
{
	std::vector<ImGui::FileBrowser> FileBrowserVector;
	for (int i = 0; i < CurrentProfile.ObjectStruct.size(); i++)
	{
		ImGui::FileBrowser FileDialog(FileBrowserFlags);
		FileDialog.SetTitle(("Add " + wstring2string(CurrentProfile.ObjectStruct[i].ObjectType.Type)).c_str());
		if (CurrentProfile.ObjectStruct[i].ObjectType.Type == L"Mods")
		{
			FileDialog.SetTypeFilters({ ".jar" });
		}
		else
		{
			FileDialog.SetTypeFilters({ ".zip" });
		}
		FileBrowserVector.push_back(FileDialog);
	}
	return FileBrowserVector;
}
bool LoadModdedProfileData(Profile& CurrentProfile, json& ProfileJson, std::string Id)
{
	CurrentTask = L"Loading profile info...";
	if (!ParseVersion(CurrentProfile, ProfileJson))
	{
		return false; //We skip non-modded profiles
	}
	CurrentProfile.Id = string2wstring(std::string(Id));
	CurrentProfile.Name = string2wstring(ProfileJson.value("name", ""));
	CurrentProfile.WsDate = string2wstring(ProfileJson.value("lastUsed", ""));
	CurrentProfile.ProfileDirs = CalculateProfileDirectories(ProfileJson);
	CurrentProfile.NameAndVersion = CombineNameAndVersion(CurrentProfile);
	CurrentProfile.ObjectStruct = GetObjectStruct(CurrentProfile);
	CurrentProfile.FileBrowserVector = GetFileBrowserVector(CurrentProfile);
	return true;
}

std::vector<Profile> GetProfiles()
{
	CurrentTask = L"Loading profiles...";
	std::vector<Profile> ProfilesVector;
	if (ProfilesFound(ProfilesFile))
	{
		std::ifstream LauncherProfiles(wstring2string(ProfilesFile));
		Profile CurrentProfile;
		const std::string format = "%Y-%m-%d %H:%M:%S";
		json data = json::parse(LauncherProfiles);
		for (auto& [Id, ProfileJson] : data["profiles"].items())
		{
			if (!LoadModdedProfileData(CurrentProfile, ProfileJson, Id))
			{
				continue; // Non-modded profile
			}
			ProfilesVector.push_back(CurrentProfile);
		}		
	}
	CurrentTask = L"";
	return ProfilesVector;
}

void MoveObjectByState(ProfileDirectories& ProfileDirs, ObjectStruct& ObjectStruct, int ObjectIndex)
{
	if (ObjectStruct.ObjectVector[ObjectIndex].IsEnabled)
	{
		if (ObjectStruct.ObjectType.Type == L"Mods")
		{
			fs::rename(ProfileDirs.DisabledModsDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.ModsDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
		else if (ObjectStruct.ObjectType.Type == L"Resource Packs")
		{
			fs::rename(ProfileDirs.DisabledResourcePacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.ResourcePacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
		else // Shader Pack
		{
			fs::rename(ProfileDirs.DisabledShaderPacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.ShaderPacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
	}
	else
	{
		if (ObjectStruct.ObjectType.Type == L"Mods")
		{
			fs::rename(ProfileDirs.ModsDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.DisabledModsDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
		else if (ObjectStruct.ObjectType.Type == L"Resource Packs")
		{
			fs::rename(ProfileDirs.ResourcePacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.DisabledResourcePacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
		else // Shader Pack
		{
			fs::rename(ProfileDirs.ShaderPacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
				ProfileDirs.DisabledShaderPacksDir + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
		}
	}
}
void ObjectList(Profile& CurrentProfile)
{
	std::vector<ObjectStruct>& ProfileObjectStruct = CurrentProfile.ObjectStruct;
	std::vector<ImGui::FileBrowser>& ProfileFileBrowserVector = CurrentProfile.FileBrowserVector;
	for (int POSIndex = 0; POSIndex < ProfileObjectStruct.size(); POSIndex++) //It should repeat only 3 times since there are only 3 objects but still
	{
		std::wstring& ObjectType = ProfileObjectStruct[POSIndex].ObjectType.Type;
		if (ProfileObjectStruct[POSIndex].ObjectType.IsEnabled)
		{
			if (ImGui::CollapsingHeader((wstring2string(ObjectType) + "##" + std::to_string(POSIndex)).c_str()))
			{
				for (int i = 0; i < ProfileObjectStruct[POSIndex].ObjectVector.size(); i++) //We display each vector item data (Mods, Resource Packs, Shader Packs)
				{
					ImGui::Text("%s", (wstring2string(ProfileObjectStruct[POSIndex].ObjectVector[i].Name)).c_str());
					ImGui::SameLine();
					if (ImGui::Checkbox(("Enabled##" + wstring2string(ObjectType) + std::to_string(i)).c_str(), &ProfileObjectStruct[POSIndex].ObjectVector[i].IsEnabled))
					{
						MoveObjectByState(CurrentProfile.ProfileDirs, ProfileObjectStruct[POSIndex], i);
					}
					ImGui::SameLine();
					if (ImGui::Button(("Details##" + wstring2string(ObjectType) + std::to_string(i)).c_str()))
					{
						ImGui::OpenPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(i)).c_str());
					}
					if (ImGui::BeginPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(i)).c_str()))
					{
						ImGui::Text("Description: %s", (wstring2string(ProfileObjectStruct[POSIndex].ObjectVector[i].Description).c_str()));
						ImGui::Text("Version: %s", (wstring2string(ProfileObjectStruct[POSIndex].ObjectVector[i].Version).c_str()));
						ImGui::Text("Author(s): %s", (wstring2string(ProfileObjectStruct[POSIndex].ObjectVector[i].Authors).c_str()));
						ImGui::EndPopup();
					}
					if (i != ProfileObjectStruct[POSIndex].ObjectVector.size() - 1)
					{
						ImGui::Separator();
					}
				}
				if (ImGui::Button(("Add " + wstring2string(ObjectType) + "##" + std::to_string(POSIndex)).c_str()))
				{
					ProfileFileBrowserVector[POSIndex].Open();
				}
				ProfileFileBrowserVector[POSIndex].Display();
				if (ProfileFileBrowserVector[POSIndex].HasSelected())
				{
					for (fs::path& CurrentFile : ProfileFileBrowserVector[POSIndex].GetMultiSelected())
					{
						std::wstring wsFile = CurrentFile.wstring();
						std::wstring Filename = CurrentFile.filename().wstring();
						Object NewObject;
						if (ObjectType == L"Mods")
						{
							fs::copy(wsFile, CurrentProfile.ProfileDirs.ModsDir + L"\\" + Filename, fs::copy_options::skip_existing);

						}
						else if (ObjectType == L"Resource Packs")
						{
							fs::copy(wsFile, CurrentProfile.ProfileDirs.ResourcePacksDir + L"\\" + Filename, fs::copy_options::skip_existing);
						}
						else //Shader Packs
						{
							fs::copy(wsFile, CurrentProfile.ProfileDirs.ShaderPacksDir + L"\\" + Filename, fs::copy_options::skip_existing);
						}
						NewObject = GetObjectInfo(Filename, CurrentProfile.ProfileDirs, ObjectType, true);
						ProfileObjectStruct[POSIndex].ObjectVector.insert(ProfileObjectStruct[POSIndex].ObjectVector.begin(), NewObject);
						//We insert the object into the vector to not fully reload profiles
						ProfileFileBrowserVector[POSIndex].ClearSelected();
					}
				}
			}
		}
	}
}

void CleanLog()
{
	if (Settings.ResetLog)
	{
		CurrentTask = L"Cleaning Log...";
		if (fs::exists(GetCurrentFolder() + L"\\" + Settings.LogName))
		{
			fs::remove(GetCurrentFolder() + L"\\" + Settings.LogName);
		}
		CurrentTask = L"";
	}
}