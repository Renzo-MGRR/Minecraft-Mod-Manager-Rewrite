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

void CreateIfNotExists(std::wstring& Folder)
{
	if (!fs::exists(Folder))
	{
		fs::create_directory(Folder);
	}
}

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

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) 
{
	if (dltotal > 0) 
	{
		DownloadProgress = (float)dlnow / (float)dltotal;
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
void Combo(std::vector<std::wstring>& Vector, int id, int& VectorSelectedIndex)
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
void Combo(std::vector<Version>& Vector, int id, int& VectorSelectedIndex)
{
	if (ImGui::BeginCombo(("##Combo" + std::to_string(id)).c_str(), (Vector[VectorSelectedIndex].VersionNumber).c_str()))
	{
		for (int i = 0; i < Vector.size(); ++i)
		{
			const bool isSelected = (VectorSelectedIndex == i);
			if (ImGui::Selectable((Vector[i].VersionNumber).c_str(), isSelected))
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

std::string extractBetweenDelimiters(const std::string& str, const std::string& start_delim, const std::string& end_delim)
{
	size_t first_delim_pos = str.find(start_delim);
	if (first_delim_pos == std::string::npos)
	{
		return "";
	}
	size_t start_pos = first_delim_pos + start_delim.length();
	size_t second_delim_pos = str.find(end_delim, start_pos);
	if (second_delim_pos == std::string::npos)
	{
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

static size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
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
					if (res != CURLE_OK)
					{
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
		std::wstring ObjectFile = L"";
		std::wstring ObjectJsonFile = ObjectType.Folder + L"\\Metadata\\" + NewObject.FileName + L".json";
		if (NewObject.IsEnabled)
		{
			ObjectFile = ObjectType.Folder + L"\\" + NewObject.FileName;
			if (!fs::exists(ObjectJsonFile))
			{
				SevenZip.extractMatching(wstring2string(ObjectFile), FileToExtract, wstring2string(ObjectType.Folder));
				std::wstring MetadataFolder = ObjectType.Folder + L"\\Metadata";
				CreateIfNotExists(MetadataFolder);
				fs::rename(ObjectType.Folder + L"\\" + string2wstring(FileToExtract), ObjectJsonFile);
			}
		}
		else
		{
			ObjectFile = ObjectType.Folder + L"\\Disabled\\" + NewObject.FileName;
			if (!fs::exists(ObjectJsonFile))
			{
				SevenZip.extractMatching(wstring2string(ObjectFile), FileToExtract, wstring2string(ObjectType.Folder + L"\\Disabled"));
				std::wstring MetadataFolder = ObjectType.Folder + L"\\Metadata";
				CreateIfNotExists(MetadataFolder);
				fs::rename(ObjectType.Folder + L"\\Disabled\\" + string2wstring(FileToExtract), ObjectJsonFile);
			}
		}
		std::ifstream ObjectJson(wstring2string(ObjectJsonFile));
		//We need to erase end of line characters in order to avoid parsing errors. Idk why really
		std::string ObjectJsonString = "";
		std::string line = "";
		while (std::getline(ObjectJson, line))
		{
			ObjectJsonString += line;
		}
		ObjectJson.close();
		for (int i = 0; i < ObjectJsonString.size(); i++)
		{
			if (ObjectJsonString[i] == '\n')
			{
				ObjectJsonString.erase(ObjectJsonString.begin() + i);
			}
		}
		json Json = json::parse(ObjectJsonString);
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

void AddModPacks(Profile& CurrentProfile, std::vector<fs::path> ModPackList)
{
	DisableProfileSelection = true;
	DisableServerSelection = true;
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
		std::wstring wsModPackFolder = ModPack.parent_path().wstring() + L"\\" + ModPack.stem().wstring();
		if (!fs::is_directory(ModPack)) // Zip modpacks
		{
			if (!fs::exists(wsModPackFolder)) // If it already exists, I assume it's already extracted
			{
				CurrentTask = L"Extracting modpack: " + std::wstring(ModPack);
				fs::create_directory(wsModPackFolder);
				std::string sModPackFolder = wstring2string(wsModPackFolder);
				SevenZip.extract(ModPack.string(), sModPackFolder);
				CurrentTask = L"";
			}
		}
		if (fs::exists(wsModPackFolder + L"\\modrinth.index.json")) //Modrinth modpack
		{
			std::ifstream ModpackObjects(wsModPackFolder + L"\\modrinth.index.json");
			json data = json::parse(ModpackObjects);
			std::string DownloadURL = "";
			std::wstring Path = L"";
			std::wstring FullPath = L"";
			std::wstring Name = L"";
			std::wstring DownloadTo = L"";
			for (auto& file : data["files"])
			{
				if (file.contains("downloads") && file["downloads"].is_array())
				{
					for (auto& item : file["downloads"])
					{
						if (item.is_string())
						{
							DownloadURL = item.get<std::string>();
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
				if (DownloadURL != "" && DownloadTo != L"" && !fs::exists(DownloadTo))
				{
					std::string MemoryBuffer = "";
					CurrentTask = L"Downloading file: " + Name;
					DownloadFile(DownloadURL, DownloadTo, MemoryBuffer);
					CurrentTask = L"";
				}
			}
			ModpackObjects.close();
		}
		for (std::wstring& FileOrFolder : getAllInDirectoryW(wsModPackFolder + L"\\overrides"))
		{
			if (fs::is_directory(wsModPackFolder + L"\\overrides\\" + FileOrFolder))
			{
				for (std::wstring& CurrentObject : getInDirectoryW(wsModPackFolder + L"\\overrides\\" + FileOrFolder, false))
				{
					CurrentTask = L"Copying item: " + FileOrFolder + LR"(\)" + CurrentObject;
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
				CurrentTask = L"Copying item: " + FileOrFolder;
				fs::copy(wsModPackFolder + L"\\overrides\\" + FileOrFolder, CurrentProfile.Directory + L"\\" + FileOrFolder);
			}
		}
		CurrentTask = L"";
	}
	DisableProfileSelection = false;
	DisableServerSelection = true;
	DownloadProgress = 0.0f;
}

void AddObject(Profile& CurrentProfile, std::vector<ObjectStruct>& ProfileObjectStructVector, std::vector<std::filesystem::path> Selection, int POSIndex)
{
	for (fs::path& CurrentFile : Selection)
	{
		std::wstring wsFile = CurrentFile.wstring();
		std::wstring Filename = CurrentFile.filename().wstring();
		Object NewObject;
		CurrentTask = L"Copying " + Filename;
		fs::copy(wsFile, ProfileObjectStructVector[POSIndex].ObjectType.Folder + L"\\" + Filename, fs::copy_options::skip_existing);
		NewObject = GetObjectInfo(Filename, ProfileObjectStructVector[POSIndex].ObjectType, true);
		ProfileObjectStructVector[POSIndex].ObjectVector.insert(ProfileObjectStructVector[POSIndex].ObjectVector.begin(), NewObject);
		//We insert the object into the vector to not fully reload profiles
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

void GetProjectsInfo(std::wstring& ObjectType, Profile& CurrentProfile)
{
	CurrentTask = L"Getting modrinth projects info...";
	//Request Example: https://api.modrinth.com/v2/search?facets=[["project_type:mod"],["versions:1.16.5"]]
	std::string ProjectType = "";
	if (ObjectType == L"Mods")
	{
		ProjectType = "mod";
	}
	else if (ObjectType == L"Resource Packs")
	{
		ProjectType = "resourcepack";
	}
	else if (ObjectType == L"Shaders")
	{
		ProjectType = "shader";
	}
	else //Modpacks
	{
		ProjectType = "modpack";
	}
	std::string Request = "https://api.modrinth.com/v2/search?facets=%5B%5B%22project_type:" + 
		ProjectType + "%22%5D,%5B%22categories:fabric%22%5D,%5B%22versions:" + 
		wstring2string(CurrentProfile.Version) + "%22%5D%5D";
	std::string Projects = "";
	DownloadFile(Request, L"", Projects);
	json Objects = json::parse(Projects);
	for (auto& item : Objects["hits"])
	{
		Project CurrentProject;
		CurrentProject.ProjectID = item.value("project_id", "");
		CurrentProject.Description = item.value("description", "");
		CurrentProject.Title = item.value("title", "");
		std::string Request = "https://api.modrinth.com/v2/project/" +
			CurrentProject.ProjectID + "/version?loaders=%5B%22fabric%22%5D&game_versions=%5B%22" +
			wstring2string(CurrentProfile.Version) + "%22%5D";
		std::string VersionData = "";
		DownloadFile(Request, L"", VersionData);
		json VersionsJson = json::parse(VersionData);
		for (auto& CVersion : VersionsJson)
		{
			Version CurrentVersion;
			CurrentVersion.Name = CVersion.value("name", "");
			CurrentVersion.VersionNumber = CVersion.value("version_number", "");
			for (auto& file : CVersion["files"]) 
			{
				ModrinthFile NewFile;
				NewFile.URL = file.value("url", "");
				NewFile.GetInstalled = false;
				CurrentVersion.MainFile = NewFile;
				break; // Versions only contain one main file
			}
			CurrentProject.Versions.push_back(CurrentVersion);
		}
		CurrentProfile.ModrinthProjects.push_back(CurrentProject);
		CurrentTask = L"";
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
				if (ImGui::Button(("Add " + wstring2string(ObjectType) + " (Modrinth)##" + std::to_string(POSIndex)).c_str()))
				{
					CurrentProfile.ModrinthProjects.clear();
					std::thread ProjectsThread(GetProjectsInfo, std::ref(ObjectType), std::ref(CurrentProfile));
					ProjectsThread.detach();
					int MonX = GetSystemMetrics(SM_CXSCREEN);
					int MonY = GetSystemMetrics(SM_CYSCREEN);
					ImVec2 center = ImGui::GetMainViewport()->GetCenter();
					ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					ImGui::SetNextWindowSize(ImVec2(750, 750));
					ImGui::OpenPopup("Projects");
				}
				if (ImGui::BeginPopupModal("Projects"))
				{
					for (int p = 0; p < CurrentProfile.ModrinthProjects.size(); p++)
					{
						ImGui::Text("%s", CurrentProfile.ModrinthProjects[p].Title.c_str());
						ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 500);
						ImGui::TextWrapped("%s", CurrentProfile.ModrinthProjects[p].Description.c_str());
						ImGui::PopTextWrapPos();
						ImGui::SetNextItemWidth(300);
						ImGui::Checkbox(("##GetInstalled" + std::to_string(p)).c_str(), 
							&CurrentProfile.ModrinthProjects[p].Versions[CurrentProfile.ModrinthProjects[p].SelectedVersion].MainFile.GetInstalled);
						Combo(CurrentProfile.ModrinthProjects[p].Versions, 10 + p, CurrentProfile.ModrinthProjects[p].SelectedVersion);
						if (p != CurrentProfile.ModrinthProjects.size() - 1)
						{
							ImGui::Separator();
						}
					}
					if (ImGui::Button("Cancel##Projects"))
					{
						CurrentProfile.ModrinthProjects.clear();
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				ProfileFileBrowserVector[POSIndex].Display();
				if (ProfileFileBrowserVector[POSIndex].HasSelected())
				{
					std::vector<std::filesystem::path> Selection = ProfileFileBrowserVector[POSIndex].GetMultiSelected();
					std::thread NewThread(AddObject, std::ref(CurrentProfile), std::ref(ProfileObjectStructVector), Selection, POSIndex);
					NewThread.detach();
					ProfileFileBrowserVector[POSIndex].ClearSelected();
				}
				ImGui::SameLine();
				if (ImGui::Button(("Remove all## " + wstring2string(ObjectType) + "Popup" + std::to_string(POSIndex)).c_str()))
				{
					ImGui::OpenPopup(("Remove all## " + wstring2string(ObjectType) + "Popup" + std::to_string(POSIndex)).c_str());
				}
				if (ImGui::BeginPopup(("Remove all## " + wstring2string(ObjectType) + "Popup" + std::to_string(POSIndex)).c_str()))
				{
					ImGui::Text(("Are you sure you want to remove all " + wstring2string(ObjectType) + "?").c_str());
					if (ImGui::Button(("Remove all##Confirm" + wstring2string(ObjectType) + std::to_string(POSIndex)).c_str()))
					{
						for (int t = 0; t < ProfileObjectStructVector[POSIndex].ObjectVector.size(); t++)
						{
							fs::remove(CurrentProfile.Directory + L"\\" + ObjectType + L"\\" + ProfileObjectStructVector[POSIndex].ObjectVector[t].FileName);
							fs::remove(CurrentProfile.Directory + L"\\" + ObjectType + L"\\Metadata\\" + ProfileObjectStructVector[POSIndex].ObjectVector[t].FileName + L".json");;
							ProfileObjectStructVector[POSIndex].ObjectVector.erase(ProfileObjectStructVector[POSIndex].ObjectVector.begin() + t);
							t--;
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button(("Cancel##" + wstring2string(ObjectType) + std::to_string(POSIndex)).c_str()))
					{
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (ImGui::BeginTable(("##" + wstring2string(ObjectType) + "##" + std::to_string(POSIndex)).c_str(), 4))
				{
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableHeadersRow();
					for (int row = 0; row < ProfileObjectStructVector[POSIndex].ObjectVector.size(); row++)
					{
						ImGui::TableNextRow();
						for (int column = 0; column < 4; column++)
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
							else if (column == 2)
							{
								if (ImGui::Button(("...##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::OpenPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(row)).c_str());
								}
								if (ImGui::BeginPopup(("DetailsPopup##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 300.0f);
									ImGui::Text("%s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Description).c_str()));
									ImGui::Text("Version: %s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Version).c_str()));
									ImGui::Text("Author(s): %s", (wstring2string(ProfileObjectStructVector[POSIndex].ObjectVector[row].Authors).c_str()));
									ImGui::PopTextWrapPos();
									ImGui::EndPopup();
								}
							}
							else
							{
								if (ImGui::Button(("Remove##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::OpenPopup(("Remove##Popup" + wstring2string(ObjectType) + std::to_string(row)).c_str());
								}
								if (ImGui::BeginPopup(("Remove##Popup" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
								{
									ImGui::Text("Are you sure you want to remove this item?");
									if (ImGui::Button(("Remove##Confirm" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
									{
										fs::remove(CurrentProfile.Directory + L"\\" + ObjectType + L"\\" + ProfileObjectStructVector[POSIndex].ObjectVector[row].FileName);
										fs::remove(CurrentProfile.Directory + L"\\" + ObjectType + L"\\Metadata\\" + ProfileObjectStructVector[POSIndex].ObjectVector[row].FileName + L".json");
										ProfileObjectStructVector[POSIndex].ObjectVector.erase(ProfileObjectStructVector[POSIndex].ObjectVector.begin() + row);
										ImGui::CloseCurrentPopup();
									}
									ImGui::SameLine();
									if (ImGui::Button(("Cancel##" + wstring2string(ObjectType) + std::to_string(row)).c_str()))
									{
										ImGui::CloseCurrentPopup();
									}
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

void ManageObjects(std::vector<Profile>& Vector, int& Index, int Id)
{
	Profile& CurrentProfile = Vector[Index];
	ImGui::FileBrowser& ModPackFileBrowser = CurrentProfile.FileBrowserVector.back();
	if (ImGui::Button(("Add Modpacks##" + std::to_string(Id)).c_str()))
	{
		ModPackFileBrowser.Open();
	}
	ModPackFileBrowser.Display();
	if (ModPackFileBrowser.HasSelected())
	{
		std::vector<fs::path> ModPackList = ModPackFileBrowser.GetMultiSelected();
		std::thread NewThread(AddModPacks, std::ref(CurrentProfile), ModPackList);
		NewThread.detach();
		ModPackFileBrowser.ClearSelected();
	}
	ObjectList(CurrentProfile);
}

void RemoveProfile(std::vector<Profile>& Vector, int& Index)
{
	fs::remove_all(Vector[Index].Directory);
	Vector.erase(Vector.begin() + Index);
	if (Index != 0)
	{
		Index--;
	}
}

HANDLE Execute(const std::wstring& cmdLine, const std::wstring& RunFrom, bool Silent, bool WaitTillFinish, bool ClosePrHandle)
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
	if (ClosePrHandle)
	{
		CloseHandle(pi.hProcess);
	}
	CloseHandle(pi.hThread);
	return pi.hProcess;
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
	std::ifstream LauncherProfilesI(wstring2string(ProfilesFile));
	json OldData = json::parse(LauncherProfilesI);
	LauncherProfilesI.close();
	std::ofstream BkpLauncherProfiles(wstring2string(ProfilesFile) + ".bkp");
	BkpLauncherProfiles << std::setw(4) << OldData << std::endl;
	BkpLauncherProfiles.close();
	std::ofstream LauncherProfilesO(wstring2string(ProfilesFile));
	LauncherProfilesO << std::setw(4) << JsonData << std::endl;
	LauncherProfilesO.close();
}

void RemoveJsonObject(std::string& Id, std::wstring& ProfilesFile)
{
	if (ProfilesFound(ProfilesFile))
	{
		std::ifstream LauncherProfiles(wstring2string(ProfilesFile));
		json data = json::parse(LauncherProfiles);
		LauncherProfiles.close();
		data["profiles"].erase(data["profiles"].find(Id));
		RewriteProfilesFile(data, ProfilesFile);
	}
}

std::chrono::system_clock::time_point GFG(const std::string& datetimeString, const std::string& format)
{
	tm tmStruct = {};
	std::istringstream ss(datetimeString);
	ss >> std::get_time(&tmStruct, format.c_str());
	return std::chrono::system_clock::from_time_t(mktime(&tmStruct));
}
void CreateProfileFolders(Profile& CurrentProfile)
{
	for (int i = 0; i < CurrentProfile.ObjectStructVector.size(); i++)
	{
		CreateIfNotExists(CurrentProfile.ObjectStructVector[i].ObjectType.Folder);
		std::wstring DisabledFolder = CurrentProfile.ObjectStructVector[i].ObjectType.Folder + L"\\Disabled";
		CreateIfNotExists(DisabledFolder);
	}
}
void LoadModdedProfileData(Profile& CurrentProfile, json& ProfileJson, std::string Id, json& JsonData)
{
	CurrentTask = L"Loading profile info...";
	CurrentProfile.Id = Id;
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
	CreateProfileFolders(CurrentProfile);
	CurrentProfile.FileBrowserVector = GetFileBrowserVector(CurrentProfile);
}

std::vector<std::string> LinesToVector(std::string File)
{
	std::vector<std::string> Vector;
	if (fs::exists(File))
	{
		std::ifstream CurrentFile(File);
		std::string currline;
		if (CurrentFile.is_open())
		{
			while (std::getline(CurrentFile, currline))
			{
				Vector.push_back(currline);
			}
			CurrentFile.close();
		}
	}
	return Vector;
}

std::wstring GetServerVersion(Profile& CurrentServer)
{
	std::wstring Version = L"";
	if (!fs::exists(CurrentServer.Directory + L"\\install.properties"))
	{
		std::string FileToExtract = "install.properties";
		SevenZip.extractMatching(wstring2string(CurrentServer.Directory) + "\\server.jar", FileToExtract, wstring2string(CurrentServer.Directory));
	}
	std::vector<std::string> InstallLines = LinesToVector(wstring2string(CurrentServer.Directory + L"\\install.properties"));
	for (std::string& Line : InstallLines)
	{
		if (Line.find("game-version=") != std::string::npos)
		{
			Version = string2wstring(Line.substr(13));
		}
	}
	return Version;
}

void LoadModdedServerData(Profile& CurrentServer, std::wstring& ServerName)
{
	CurrentTask = L"Loading server info...";
	CurrentServer.Name = ServerName;
	CurrentServer.Directory = MinecraftDir + L"\\servers\\" + ServerName;
	CurrentServer.Version = GetServerVersion(CurrentServer);
	CurrentServer.NameAndVersion = CombineNameAndVersion(CurrentServer);
	CurrentServer.ObjectStructVector = GetObjectStructVector(CurrentServer);
	CreateIfNotExists(CurrentServer.Directory);
	CreateProfileFolders(CurrentServer);
	CurrentServer.FileBrowserVector = GetFileBrowserVector(CurrentServer);
}

void GetServers(std::vector<Profile>& ServersVector)
{
	for (std::wstring CurrentServerName : getInDirectoryW(MinecraftDir + L"\\servers\\", true))
	{
		Profile NewServer;
		LoadModdedServerData(NewServer, CurrentServerName);
		Servers.push_back(NewServer);
	}
	CurrentTask = L"";
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
		LauncherProfiles.close();
	}
	CurrentTask = L"";
}

void ReloadProfiles(std::vector<Profile>& ProfilesVector, int& Index)
{
	Index = 0;
	ProfilesVector.clear();
	GetProfiles(Profiles);
}

std::vector<std::string> GetFabricInstallerVersions(std::string Filecontents)
{
	std::istringstream File(Filecontents);
	std::vector<std::string> FabricInstallerVersions;
	std::string currin;
	while (std::getline(File, currin)) 
	{
		if (currin.find("href=") != std::string::npos && currin.find("<h1>") == std::string::npos && currin.find("maven") == std::string::npos)
		{
			FabricInstallerVersions.push_back(currin);
		}
	}
	return FabricInstallerVersions;
}

void OpenFabricInstaller()
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
					CurrentTask = L"Downloading Fabric Installer...";
					DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
				}
				if (File == string2wstring(FabricInstaller))
				{
					if (fs::is_empty(FabricInstaller))
					{
						fs::remove(File);
						CurrentTask = L"Downloading Fabric Installer...";
						DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
					}
				}
			}
		}
		else
		{
			CurrentTask = L"Downloading Fabric Installer...";
			DownloadFile(URL, string2wstring(FabricInstaller), FileInMemory);
		}
		CurrentTask = L"";
		DownloadProgress = 0.0f;
		Execute(string2wstring(FabricInstaller), GetCurrentFolder(), false, false, true);
	}
}

std::vector<std::wstring> Unit = { L"MB", L"GB" };
int UnitSelectedIndex = 1;
std::vector<std::wstring> FabricServerVersions;
int FabricServerVersionsIndex = 0;
std::string MemBuffer = "";
std::string SelectedLoaderVersion = "";
char ServerName[260] = "";
int MinimumRAM = 2;
int MaximumRAM = 4;
bool ShowRAMError = false;
bool AskNamePrompt = false;
bool NameAlreadyExists = false;
std::string MostRecentFabricInstallerVersion = "";
void AddServer()
{
	if (SelectedLoaderVersion == "")
	{
		CurrentTask = L"Downloading Fabric Loader version list...";
		DownloadFile("https://meta.fabricmc.net/v2/versions/loader", L"", MemBuffer);
		CurrentTask = L"Getting Fabric Loader versions...";
		json fabricloader = json::parse(MemBuffer);
		for (auto& item : fabricloader)
		{
			if (item.contains("version"))
			{
				SelectedLoaderVersion = std::string(item["version"]);
				break;
			}
		}
		CurrentTask = L"";
		MemBuffer = "";
	}
	if (FabricServerVersions.empty())
	{
		CurrentTask = L"Downloading Fabric Server version list...";
		DownloadFile("https://meta.fabricmc.net/v2/versions", L"", MemBuffer);
		CurrentTask = L"Getting Fabric Server versions...";
		json Versions = json::parse(MemBuffer);
		for (auto& curritem : Versions["game"])
		{
			if (curritem.contains("version"))
			{
				FabricServerVersions.push_back(string2wstring(std::string(curritem["version"])));
			}
		}
		MemBuffer = "";
		CurrentTask = L"";
	}
}
void RewriteFile(std::vector<std::string>& Contents, std::string File)
{
	std::ofstream OutFile(File, std::ios::trunc);
	if (OutFile.is_open())
	{
		for (auto& writeline : Contents)
		{
			OutFile << writeline << std::endl;
		}
		OutFile.close();
	}
}
void InstallServer()
{
	CurrentTask = L"Downloading installer versions...";
	DownloadFile("https://meta.fabricmc.net/v2/versions/installer", L"", MemBuffer);
	json jinstallers = json::parse(MemBuffer);
	for (auto& item : jinstallers)
	{
		if (item.contains("version"))
		{
			MostRecentFabricInstallerVersion = std::string(item["version"]);
			break;
		}
	}
	MemBuffer = "";
	CurrentTask = L"Downloading server.jar...";
	Profile NewServer;
	std::wstring wsServerName = string2wstring(std::string(ServerName));
	NewServer.Directory = MinecraftDir + L"\\servers\\" + wsServerName;
	CreateIfNotExists(NewServer.Directory);
	DownloadFile("https://meta.fabricmc.net/v2/versions/loader/" + wstring2string(FabricServerVersions[FabricServerVersionsIndex]) +
		"/" + SelectedLoaderVersion + "/" + MostRecentFabricInstallerVersion + "/server/jar", 
		NewServer.Directory + L"\\server.jar", MemBuffer);
	MemBuffer = "";
	std::wstring cmdLine = L"java -jar " + NewServer.Directory + L"\\server.jar";
	HANDLE ServerHandle = Execute(cmdLine, NewServer.Directory, true, false, false);
	if (ServerHandle != nullptr)
	{
		CurrentTask = L"Installing server...";
		DWORD exitCode;
		while (GetExitCodeProcess(ServerHandle, &exitCode))
		{
			if (exitCode != STILL_ACTIVE)
			{
				break;
			}
		}
		std::vector<std::string> EulaLines = LinesToVector(wstring2string(NewServer.Directory + L"\\eula.txt"));
		if (!EulaLines.empty())
		{
			for (int i = 0; i < EulaLines.size(); i++)
			{
				if (EulaLines[i].find("eula=false") != std::string::npos)
				{
					EulaLines[i] = "eula=true";
					break;
				}
			}
		}
		RewriteFile(EulaLines, wstring2string(NewServer.Directory + L"\\eula.txt"));
		CIniReader RAMini(wstring2string(NewServer.Directory) + "\\RAM.ini");
		RAMini.WriteInteger("Server RAM", "MinimumRAM", MinimumRAM);
		RAMini.WriteInteger("Server RAM", "MaximumRAM", MaximumRAM);
		RAMini.WriteInteger("Server RAM", "SavedUnit", UnitSelectedIndex);
		LoadModdedServerData(NewServer, wsServerName);
		Servers.push_back(NewServer);
	}
	CurrentTask = L"";
}
void ServerPopup()
{
	if (ImGui::BeginPopup("##ServerInstallPopup"))
	{
		if (!FabricServerVersions.empty())
		{
			Combo(FabricServerVersions, 3, FabricServerVersionsIndex);
			ImGui::Text("Server name:");
			ImGui::SameLine();
			ImGui::InputText("##a", ServerName, 260);
			ImGui::Text("Minimum Server RAM:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			ImGui::InputInt("##b", &MinimumRAM);
			ImGui::SameLine();
			ImGui::Text("Maximum Server RAM:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			ImGui::InputInt("##c", &MaximumRAM);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			Combo(Unit, 4, UnitSelectedIndex);
			if (ImGui::Button("Install"))
			{
				if (MinimumRAM < MaximumRAM)
				{
					ShowRAMError = false;
				}
				else
				{
					ShowRAMError = true;
				}
				if (ServerName[0] == '\0')
				{
					AskNamePrompt = true;
				}
				else
				{
					AskNamePrompt = false;
				}
				for (auto& Server : Servers)
				{
					if (wstring2string(Server.Name) == ServerName)
					{
						NameAlreadyExists = true;
						break;
					}
					else
					{
						NameAlreadyExists = false;
					}
				}
				if (!ShowRAMError && !AskNamePrompt && !NameAlreadyExists)
				{
					std::thread InstallThread (InstallServer);
					InstallThread.detach();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			if (NameAlreadyExists)
			{
				ImGui::Text("This server name already exists!");
			}
			if (AskNamePrompt)
			{
				ImGui::Text("Please provide a server name!");
			}
			if (ShowRAMError)
			{
				ImGui::Text("Minimum RAM can't be more or equal than the maximum RAM!");
			}
		}
		ImGui::EndPopup();
	}
}

void CleanLog()
{
	if (Settings.ResetLog)
	{
		CurrentTask = L"Cleaning log...";
		if (fs::exists(GetCurrentFolder() + L"\\" + Settings.LogName))
		{
			fs::remove(GetCurrentFolder() + L"\\" + Settings.LogName);
		}
		CurrentTask = L"";
	}
}