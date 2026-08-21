#include <chrono>
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
#include <errno.h>
namespace fs = std::filesystem;

std::wstring GetAppData()
{
	wchar_t appdata[MAX_PATH];
	GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
	return std::wstring(appdata);
}
std::wstring GetCurrentFolder()
{
	wchar_t CurrentFolder[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, CurrentFolder);
	return std::wstring(CurrentFolder);
}
std::wstring GetDownloadsFolder()
{
	std::wstring DownloadFolder;
	wchar_t userfolder[MAX_PATH];
	GetEnvironmentVariableW(L"USERPROFILE", userfolder, MAX_PATH);
	DownloadFolder = std::wstring(userfolder) + L"\\Downloads";
	return DownloadFolder;
}

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

void ProfileCombo(std::vector<Profile> Vector, int id, int &VectorSelectedIndex)
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
		if (!JsonObject.is_string()) //Some resource packs have a translation in their descriptions. Most of them don't
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
			Description = string2wstring(JsonObject.get<std::string>());
		}
	}
	else
	{
		Description = ParseJsonTextArray(JsonObject);
	}
	return Description;
}
Object GetObjectInfo(std::wstring& ObjectFilename, ObjectType& ObjectType, bool Enabled)
{
	Object NewObject;
	NewObject.FileName = ObjectFilename;
	NewObject.IsEnabled = Enabled;
	std::string FileToExtract;
	std::vector<bit7z::byte_t> ObjectBuffer;
	if (ObjectType.Type != L"Shader Packs")
	{
		if (ObjectType.Type == L"Mods")
		{
			FileToExtract = "fabric.mod.json";
		}
		else // Resource Packs
		{
			FileToExtract = "pack.mcmeta";
		}
		if (NewObject.IsEnabled)
		{
			SevenZip.extractMatching(wstring2string(ObjectType.Folder + L"\\" + NewObject.FileName), FileToExtract, ObjectBuffer);
		}
		else
		{
			SevenZip.extractMatching(wstring2string(ObjectType.Folder + L"\\Disabled\\" + NewObject.FileName), FileToExtract, ObjectBuffer);
		}
		//We need to erase end of line characters in order to avoid parsing errors. Idk why really
		for (int i = 0; i < ObjectBuffer.size(); i++)
		{
			if (ObjectBuffer[i] == '\n')
			{
				ObjectBuffer.erase(ObjectBuffer.begin() + i);
			}
		}
		json Json = json::parse(ObjectBuffer);
		if (ObjectType.Type == L"Mods")
		{
			NewObject.Name = string2wstring(Json.value("name", ""));
			NewObject.Description = string2wstring(Json.value("description", ""));
			NewObject.Version = string2wstring(Json.value("version", ""));
			NewObject.Authors = JoinAuthorsFromJson(Json);
		}
		else if (ObjectType.Type == L"Resource Packs")
		{
			if (Json["pack"].contains("name"))
			{
				NewObject.Name = string2wstring(Json["pack"].value("name", ""));
			}
			NewObject.Description = GetResourcePackDescription(Json["pack"]["description"]);
		}
	}
	if (NewObject.Name.empty())
	{
		NewObject.Name = NewObject.FileName;
	}
	return NewObject;
}
std::wstring GetObjectFolder(ObjectType& ObjectType, Profile& CurrentProfile)
{
	std::wstring Folder;
	if (ObjectType.Type == L"Mods")
	{
		Folder = L"mods";
	}
	else if (ObjectType.Type == L"Resource Packs")
	{
		Folder = L"resourcepacks";
	}
	else // Shader Packs
	{
		Folder = L"shaderpacks";
	}
	Folder = CurrentProfile.Directory + L"\\" + Folder;
	return Folder;
}
std::wstring GetObjectType(std::wstring& Folder)
{
	std::wstring Type;
	if (Folder == L"mods")
	{
		Type = L"Mods";
	}
	else if (Folder == L"resourcepacks")
	{
		Type = L"Resource Packs";
	}
	else // Shader Packs
	{
		Type = L"Shader Packs";
	}
	return Type;
}
ObjectStruct GetObjects(ObjectType& ObjectType, Profile& CurrentProfile)
{
	CurrentTask = L"Getting " + ObjectType.Type + L" data on " + CurrentProfile.Name + L"...";
	ObjectStruct Objects;
	Objects.ObjectType = ObjectType;
	Objects.ObjectType.Folder = GetObjectFolder(ObjectType, CurrentProfile);
	std::vector<std::wstring> ObjectFileList;
	std::vector<std::wstring> ObjectDisabledFileList;
	ObjectFileList = getInDirectoryW(Objects.ObjectType.Folder, false);
	ObjectDisabledFileList = getInDirectoryW(Objects.ObjectType.Folder + L"\\Disabled", false);
	for (std::wstring& CurrentFile : ObjectFileList)
	{
		if (Objects.ObjectType.Type == L"Resource Packs" || Objects.ObjectType.Type == L"Shader Packs")
		{
			if (fs::path(CurrentFile).extension() == ".zip")
			{
				Objects.ObjectVector.push_back(GetObjectInfo(CurrentFile, Objects.ObjectType, true));
			}
		}
		else
		{
			if (Objects.ObjectType.Type == L"Mods")
			{
				if (fs::path(CurrentFile).extension() == ".jar")
				{
					Objects.ObjectVector.push_back(GetObjectInfo(CurrentFile, Objects.ObjectType, true));
				}
			}
		}
	}
	for (std::wstring& CurrentDisabledFile : ObjectDisabledFileList)
	{
		if (Objects.ObjectType.Type == L"Resource Packs" || Objects.ObjectType.Type == L"Shader Packs")
		{
			if (fs::path(CurrentDisabledFile).extension() == ".zip")
			{
				Objects.ObjectVector.push_back(GetObjectInfo(CurrentDisabledFile, Objects.ObjectType, false));
			}
		}
		else
		{
			if (Objects.ObjectType.Type == L"Mods")
			{
				if (fs::path(CurrentDisabledFile).extension() == ".jar")
				{
					Objects.ObjectVector.push_back(GetObjectInfo(CurrentDisabledFile, Objects.ObjectType, false));
				}
			}
		}
	}
	return Objects;
}

std::vector<ObjectStruct> GetObjectStructVector(Profile& CurrentProfile)
{
	CurrentTask = L"Getting profile objects...";
	std::vector<ObjectStruct> ObjectStruct;
	for (ObjectType& CurrentObjectType : Settings.GlobalObjectTypes)
	{
		ObjectStruct.push_back(GetObjects(CurrentObjectType, CurrentProfile));
	}
	return ObjectStruct;
}

void ChangeObjectTypeAvailability(ObjectType& ObjectType)
{
	std::wstring NewAvailableObjectTypes;
	for (Profile& CurrentProfile : Profiles)
	{
		for (ObjectStruct& CurrentObjectStruct : CurrentProfile.ObjectStructVector)
		{
			if (ObjectType.Type == CurrentObjectStruct.ObjectType.Type)
			{
				CurrentObjectStruct.ObjectType.IsEnabled = ObjectType.IsEnabled;
			}
		}
	}
	for (int j = 0; j < Settings.GlobalObjectTypes.size(); j++)
	{
		if (Settings.GlobalObjectTypes[j].IsEnabled)
		{
			if (j != Settings.GlobalObjectTypes.size() - 1)
			{
				NewAvailableObjectTypes = NewAvailableObjectTypes + Settings.GlobalObjectTypes[j].Type + L','; //If it's the last item, skip the ',' character
			}
			else
			{
				NewAvailableObjectTypes = NewAvailableObjectTypes + Settings.GlobalObjectTypes[j].Type;
			}
		}
	}
	iniReader.WriteString("Settings", "EnabledObjectTypes", wstring2string(NewAvailableObjectTypes));
}

void CreateIfNotExists(std::wstring& Folder)
{
	if (!fs::exists(Folder))
	{
		fs::create_directory(Folder);
	}
}

std::vector<ImGui::FileBrowser> GetFileBrowserVector(Profile& CurrentProfile) //File Browsers don't have ids like most ImGui objects do, 
//so we have to make a vector
{
	std::vector<ImGui::FileBrowser> FileBrowserVector;
	for (int i = 0; i < CurrentProfile.ObjectStructVector.size(); i++)
	{
		ImGui::FileBrowser FileDialog(FileBrowserFlags);
		FileDialog.SetTitle(("Add " + wstring2string(CurrentProfile.ObjectStructVector[i].ObjectType.Type)).c_str());
		FileDialog.SetDirectory(GetDownloadsFolder());
		if (CurrentProfile.ObjectStructVector[i].ObjectType.Type == L"Mods")
		{
			FileDialog.SetTypeFilters({ ".jar" });
		}
		else
		{
			FileDialog.SetTypeFilters({ ".zip" });
		}
		FileBrowserVector.push_back(FileDialog);
	}
	ImGui::FileBrowser ModPackFileDialog(FileBrowserFlags);
	ModPackFileDialog.SetTitle("Add Modpack");
	ModPackFileDialog.SetDirectory(GetDownloadsFolder());
	ModPackFileDialog.SetTypeFilters({ ".zip" , ".mrpack" });
	FileBrowserVector.push_back(ModPackFileDialog);
	return FileBrowserVector;
}
void RewriteProfilesFile(json& JsonData, std::wstring& ProfilesFile)
{
	std::ofstream LauncherProfiles(wstring2string(ProfilesFile));
	LauncherProfiles << std::setw(4) << JsonData << std::endl;
}
std::chrono::system_clock::time_point GFG(const std::string& datetimeString, const std::string& format)
{
	tm tmStruct = {};
	std::istringstream ss(datetimeString);
	ss >> std::get_time(&tmStruct, format.c_str());
	return std::chrono::system_clock::from_time_t(
		mktime(&tmStruct));
}
void LoadModdedProfileData(Profile& CurrentProfile, json& ProfileJson, std::string Id, json& JsonData)
{
	CurrentTask = L"Loading profile info...";
	CurrentProfile.Id = string2wstring(std::string(Id));
	CurrentProfile.Name = string2wstring(ProfileJson.value("name", ""));
	CurrentProfile.WsDate = string2wstring(ProfileJson.value("lastUsed", ""));
	CurrentProfile.WsDate.erase(23, 1);
	CurrentProfile.WsDate.erase(10, 1);
	CurrentProfile.WsDate.insert(10, L" ");
	std::string sDate = wstring2string(CurrentProfile.WsDate);
	const std::string format = "%Y-%m-%d %H:%M:%S";
	CurrentProfile.Date = GFG(sDate, format);
	CurrentProfile.NameAndVersion = CombineNameAndVersion(CurrentProfile);
	CurrentProfile.Directory = string2wstring(ProfileJson.value("gameDir", ""));
	CurrentProfile.JsonObject = ProfileJson;
	if (CurrentProfile.Directory == L"")
	{
		CurrentProfile.Directory = MinecraftDir + L"\\profiles\\" + CurrentProfile.Name;
		ProfileJson["gameDir"] = wstring2string(CurrentProfile.Directory);
		RewriteProfilesFile(JsonData, ProfilesFile);
	}
	CreateIfNotExists(CurrentProfile.Directory);
	CurrentProfile.ObjectStructVector = GetObjectStructVector(CurrentProfile);
	for (int i = 0; i < CurrentProfile.ObjectStructVector.size(); i++)
	{
		CreateIfNotExists(CurrentProfile.ObjectStructVector[i].ObjectType.Folder);
		std::wstring DisabledFolder = CurrentProfile.ObjectStructVector[i].ObjectType.Folder + L"\\Disabled";
		CreateIfNotExists(DisabledFolder);
	}
	CurrentProfile.FileBrowserVector = GetFileBrowserVector(CurrentProfile);
}

void GetProfiles(std::vector<Profile>& ProfilesVector)
{
	CurrentTask = L"Loading profiles...";
	if (ProfilesFound(ProfilesFile))
	{
		std::ifstream LauncherProfiles(wstring2string(ProfilesFile));
		json data = json::parse(LauncherProfiles);
		Profile CurrentProfile;
		for (auto& [Id, ProfileJson] : data["profiles"].items())
		{
			if (ParseVersion(CurrentProfile, ProfileJson))
			{
				LoadModdedProfileData(CurrentProfile, ProfileJson, Id, data);
				ProfilesVector.push_back(CurrentProfile);
			}
		}		
	}
	CurrentTask = L"";
}

void MoveObjectByState(ObjectStruct& ObjectStruct, int ObjectIndex)
{
	if (ObjectStruct.ObjectVector[ObjectIndex].IsEnabled)
	{
		fs::rename(ObjectStruct.ObjectType.Folder + L"\\Disabled\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
			ObjectStruct.ObjectType.Folder + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
	}
	else
	{
		fs::rename(ObjectStruct.ObjectType.Folder + L"\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName,
			ObjectStruct.ObjectType.Folder + L"\\Disabled\\" + ObjectStruct.ObjectVector[ObjectIndex].FileName);
	}
}
void ObjectList(Profile& CurrentProfile)
{
	std::vector<ObjectStruct>& ProfileObjectStructVector = CurrentProfile.ObjectStructVector;
	std::vector<ImGui::FileBrowser>& ProfileFileBrowserVector = CurrentProfile.FileBrowserVector;

	for (int POSIndex = 0; POSIndex < ProfileObjectStructVector.size(); POSIndex++) //It should repeat only 3 times since there are only 3 objects but still
	{
		std::wstring& ObjectType = ProfileObjectStructVector[POSIndex].ObjectType.Type;
		if (ProfileObjectStructVector[POSIndex].ObjectType.IsEnabled)
		{
			if (ImGui::CollapsingHeader((wstring2string(ObjectType) + "##" + std::to_string(POSIndex)).c_str()))
			{
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
						fs::copy(wsFile, ProfileObjectStructVector[POSIndex].ObjectType.Folder + L"\\" + Filename, fs::copy_options::skip_existing);
						NewObject = GetObjectInfo(Filename, ProfileObjectStructVector[POSIndex].ObjectType, true);
						ProfileObjectStructVector[POSIndex].ObjectVector.insert(ProfileObjectStructVector[POSIndex].ObjectVector.begin(), NewObject);
						//We insert the object into the vector to not fully reload profiles
						ProfileFileBrowserVector[POSIndex].ClearSelected();
					}
				}
				if (ImGui::BeginTable(("##" + wstring2string(ObjectType) + "##" + std::to_string(POSIndex)).c_str(), 3))
				{
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableHeadersRow();
					for (int row = 0; row < ProfileObjectStructVector[POSIndex].ObjectVector.size(); row++)
					{
						ImGui::TableNextRow();
						for (int column = 0; column < 3; column++)
						{
							ImGui::TableSetColumnIndex(column);
							if (column == 0)
							{
								ImGui::Text("%s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Name)).c_str());
							}
							else if (column == 1)
							{
								if (ImGui::Checkbox(("##" + wstring2string(ObjectType) + std::to_string(row)).c_str(), &ProfileObjectStructVector[POSIndex].ObjectVector[row].IsEnabled))
								{
									MoveObjectByState(ProfileObjectStructVector[POSIndex], row);
								}
							}
							else
							{
								if (ImGui::Button(("...##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::OpenPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(row)).c_str());
								}
								if (ImGui::BeginPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::Text("Description: %s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Description).c_str()));
									ImGui::Text("Version: %s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Version).c_str()));
									ImGui::Text("Author(s): %s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Authors).c_str()));
									ImGui::EndPopup();
								}
							}
						}
					}
					ImGui::EndTable();
				}
			}
		}
	}
}
std::string extractBetweenDelimiters(const std::string& str, const std::string& start_delim, const std::string& end_delim) {
	size_t first_delim_pos = str.find(start_delim);

	if (first_delim_pos == std::string::npos) {
		return "";
	}
	size_t start_pos = first_delim_pos + start_delim.length();
	size_t second_delim_pos = str.find(end_delim, start_pos);
	if (second_delim_pos == std::string::npos) {
		return "";
	}
	size_t length = second_delim_pos - start_pos;
	return str.substr(start_pos, length);
}
static size_t WriteToMemory(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	std::string* buf = static_cast<std::string*>(userp);
	buf->append(static_cast<char*>(contents), realsize);
	return realsize;
}
static size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

FILE* SetCURLOptions(CURL* curlobj, std::wstring File, std::string* MemoryBuffer, std::string& URL)
{
	curl_easy_setopt(curlobj, CURLOPT_URL, URL.c_str());
	curl_easy_setopt(curlobj, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
	curl_easy_setopt(curlobj, CURLOPT_NOPROGRESS, 0L);
	std::string sFile = wstring2string(File);
	FILE* FileObj = NULL;
	if (File != L"")
	{
		FileObj = fopen(sFile.c_str(), "wb");
		if (FileObj == NULL) {
			WriteToLog(string2wstring(strerror(errno)));
			return FileObj;
		}
		curl_easy_setopt(curlobj, CURLOPT_WRITEFUNCTION, WriteData);
		curl_easy_setopt(curlobj, CURLOPT_WRITEDATA, FileObj);
	}
	else
	{
		curl_easy_setopt(curlobj, CURLOPT_WRITEFUNCTION, WriteToMemory);
		curl_easy_setopt(curlobj, CURLOPT_WRITEDATA, MemoryBuffer);
	}
	curl_easy_setopt(curlobj, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curlobj, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curlobj, CURLOPT_SSL_VERIFYPEER, 0L);
	return FileObj;
}

void DownloadFile(std::string URL, std::wstring File, std::string& MemoryBuffer)
{
	CURL* curl = curl_easy_init();
	if (curl) {
		if (File != L"")
		{
			bool Download = false;
			if (!fs::exists(GetCurrentFolder() + L"\\" + File))
			{
				Download = true;
			}
			else if (fs::is_empty(GetCurrentFolder() + L"\\" + File))
			{
				Download = true;
			}
			if (Download)
			{
				std::string sFile = wstring2string(File);
				FILE* FileObj = SetCURLOptions(curl, File, &MemoryBuffer, URL);
				if (FileObj != NULL)
				{
					CURLcode res = curl_easy_perform(curl);
					fclose(FileObj);
					if (res != CURLE_OK) {
						if (fs::exists(File))
						{
							fs::remove(File);
						}
					}
					
				}
				curl_easy_cleanup(curl);
			}
		}
		else
		{
			SetCURLOptions(curl, File, &MemoryBuffer, URL);
			CURLcode res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
		}
	}
}

std::vector<std::string> GetFabricInstallerVersions(std::string Filecontents)
{
	std::istringstream File(Filecontents);
	std::vector<std::string> FabricInstallerVersions;
	std::string currin;
	while (std::getline(File, currin)) {
		if (currin.find("href=") != std::string::npos && currin.find("<h1>") == std::string::npos && currin.find("maven") == std::string::npos)
		{
			FabricInstallerVersions.push_back(currin);
		}
	}
	return FabricInstallerVersions;
}

void OpenFabricInstaller()
{
	std::thread([=]()
		{
			std::string FileInMemory = "";
			DownloadFile("https://maven.fabricmc.net/net/fabricmc/fabric-installer/", L"", FileInMemory);
			std::vector<std::string> FabricInstallerVersions = GetFabricInstallerVersions(FileInMemory);
			FileInMemory = ""; //We clean the file in memory just in case
			if (!FabricInstallerVersions.empty())
			{
				std::string VersionToInstall = extractBetweenDelimiters(FabricInstallerVersions[FabricInstallerVersions.size() - 1], R"(")", R"(/)");
				std::string FabricInstaller = "fabric-installer-" + VersionToInstall + ".exe";
				std::string URL = "https://maven.fabricmc.net/net/fabricmc/fabric-installer/" + VersionToInstall + "/fabric-installer-" + VersionToInstall + ".exe";
				if (fs::exists(FabricInstaller))
				{
					std::vector<std::wstring> DirFiles = getInDirectoryW(GetCurrentFolder(), false);
					for (std::wstring File : DirFiles)
					{
						if (File.find(L"fabric-installer") != std::wstring::npos && File != string2wstring(FabricInstaller)) //Check for old fabric installers
						{
							fs::remove(File);
							DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
						}
						if (File == string2wstring(FabricInstaller))
						{
							if (fs::is_empty(FabricInstaller))
							{
								fs::remove(File);
								DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
							}
						}
					}
				}
				else
				{
					DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
				}
				Execute(string2wstring(FabricInstaller), GetCurrentFolder(), false, false);
			}
		}).detach();
}

void AddModPacks(Profile& CurrentProfile, std::vector<fs::path> ModPackList)
{
	int ModsObjectStructIndex = 0;
	int ResourcePacksObjectStructIndex = 0;
	int ShaderPacksObjectStructIndex = 0;
	for (int i = 0; i < CurrentProfile.ObjectStructVector.size(); i++)
	{
		if (CurrentProfile.ObjectStructVector[i].ObjectType.Type == L"Mods")
		{
			ModsObjectStructIndex = i;
			continue;
		}
		else if (CurrentProfile.ObjectStructVector[i].ObjectType.Type == L"Resource Packs")
		{
			ResourcePacksObjectStructIndex = i;
			continue;
		}
		else //Shader Object Struct
		{
			ShaderPacksObjectStructIndex = i;
			continue;
		}
	}
	for (fs::path ModPack : ModPackList)
	{
		if (!fs::is_directory(ModPack)) // Zip modpacks
		{
			std::wstring wsModPackFolder = ModPack.parent_path().wstring() + L"\\" + ModPack.stem().wstring(); //I needed to create this cause CreateIfNotExists only takes pointers to optimize memory usage
			if (!fs::exists(wsModPackFolder)) // If it already exists, I assume it's already extracted
			{
				fs::create_directory(wsModPackFolder);
				std::string sModPackFolder = wstring2string(wsModPackFolder);
				SevenZip.extract(ModPack.string(), sModPackFolder);
			}
			if (fs::exists(wsModPackFolder + L"\\modrinth.index.json")) //Modrinth modpack
			{
				std::ifstream ModpackObjects(wsModPackFolder + L"\\modrinth.index.json");
				json data = json::parse(ModpackObjects);
				std::string DownloadNJ = "";
				std::wstring Path = L"";
				std::wstring FullPath = L"";
				std::wstring Name = L"";
				std::wstring DownloadTo = L"";
				for (auto& file : data["files"])
				{
					if (file.contains("downloads") && file["downloads"].is_array()) {
						for (auto& item : file["downloads"]) {
							if (item.is_string()) {
								DownloadNJ = item.get<std::string>();
							}
						}
					}
					if (file.contains("path") && file["path"].is_string())
					{
						int pos = file.value("path", "").find("/");
						Path = string2wstring(file.value("path", "").substr(0, pos));
						Name = string2wstring(file.value("path", "").substr(pos + 1, file.value("path", "").size()));
						FullPath = string2wstring(file.value("path", ""));
						std::wstring NeededPath = wsModPackFolder + L"\\overrides\\" + Path;
						CreateIfNotExists(NeededPath);
						DownloadTo = NeededPath + L"\\" + Name;
					}
					if (DownloadNJ != "" && Path != L"" && !fs::exists(DownloadTo))
					{
						std::string MemoryBuffer = "";
						CurrentTask = L"Downloading file: " + Name;
						DownloadFile(DownloadNJ, DownloadTo, MemoryBuffer);
						CurrentTask = L"";
					}
				}

				for (std::wstring& FileOrFolder : getAllInDirectoryW(wsModPackFolder + L"\\overrides"))
				{	
					CurrentTask = L"Copying item: " + FileOrFolder;
					if (fs::is_directory(wsModPackFolder + L"\\overrides\\" + FileOrFolder))
					{
						for (std::wstring& CurrentObject : getInDirectoryW(wsModPackFolder + L"\\overrides\\" + FileOrFolder, false))
						{
							if (FileOrFolder == L"mods" || FileOrFolder == L"resourcepacks" || FileOrFolder == L"shaderpacks")
							{
								ObjectType NewObjectType;
								NewObjectType.Folder = wsModPackFolder + L"\\overrides\\" + FileOrFolder;
								NewObjectType.IsEnabled = true;
								NewObjectType.Type = GetObjectType(FileOrFolder);
								Object NewObject = GetObjectInfo(CurrentObject, NewObjectType, true);
								if (FileOrFolder == L"mods")
								{
									CurrentProfile.ObjectStructVector[ModsObjectStructIndex].ObjectVector.push_back(NewObject);
								}
								else if (FileOrFolder == L"resourcepacks")
								{
									CurrentProfile.ObjectStructVector[ResourcePacksObjectStructIndex].ObjectVector.push_back(NewObject);
								}
								else //Shader Packs
								{
									CurrentProfile.ObjectStructVector[ShaderPacksObjectStructIndex].ObjectVector.push_back(NewObject);
								}
							}
							std::wstring ParentFolder = CurrentProfile.Directory + L"\\" + FileOrFolder;
							CreateIfNotExists(ParentFolder);
							fs::copy(wsModPackFolder + L"\\overrides\\" + FileOrFolder + L"\\" + CurrentObject, ParentFolder + L"\\" + CurrentObject, fs::copy_options::skip_existing);
						}
					}
					else
					{
						fs::copy(wsModPackFolder + L"\\overrides\\" + FileOrFolder, CurrentProfile.Directory + L"\\" + FileOrFolder);
					}
				}
				CurrentTask = L"";
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