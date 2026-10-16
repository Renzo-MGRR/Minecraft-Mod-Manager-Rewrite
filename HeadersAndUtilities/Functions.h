#pragma once
#include <string>
#include <curl/curl.h>
#include "..\bit7z\bitarchivereader.hpp"
#include "..\bit7z\bitfileextractor.hpp"
#include "InitializationAndVariables.h"
#include "..\IniReader\IniReader.h"
#include "Settings.h"

void RemoveJsonObject(std::string& Id, std::wstring& ProfilesFile);

void RewriteProfilesFile(json& JsonData, std::wstring& ProfilesFile);

void ReloadProfiles(std::vector<Profile>& ProfilesVector, int& Index);

void ManageObjects(std::vector<Profile>& Vector, int& Index, int Id);

void RemoveProfile(std::vector<Profile>& Vector, int& Index);

void OpenFabricInstaller();

void ChangeObjectTypeAvailability(ObjectType& ObjectType);

std::vector<ObjectType> GetAvailableObjectTypes(CIniReader& IniReaderObject);

std::wstring GetCurrentFolder();

void WriteToLog(const std::wstring& LoggingLine);

std::wstring string2wstring(const std::string& str);

std::string wstring2string(const std::wstring& wstr);

static size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream); //Writes file to a void pointer

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

std::vector<std::wstring> getInDirectoryW(const std::wstring& directory, bool getFolder);

std::vector<std::wstring> getAllInDirectoryW(const std::wstring& directory);

void DownloadFile(std::string URL, std::wstring File, std::string& MemoryBuffer);

void ProfileCombo(std::vector<Profile> Vector, int id, int &VectorSelectedIndex);

bool TextInput(std::wstring Description, std::string& StringHandler);

void Execute(const std::wstring& cmdLine, const std::wstring& RunFrom, bool Silent, bool WaitTillFinish);

std::wstring GetAppData();

void ServerPopup();

void AddServer();

bool LoadModdedProfileData(Profile& CurrentProfile, json& ProfileJson, std::string Id, json& JsonData);

void GetServers(std::vector<Profile>& ServersVector);

void GetProfiles(std::vector<Profile>& ProfilesVector);

bool ProfilesFound(std::wstring ProfilesFile);

bool ParseVersion(Profile& CurrentProfile, json& profile);

std::wstring CombineNameAndVersion(Profile& CurrentProfile);

std::wstring JoinAuthorsFromJson(const json& ModJson);

void ObjectList(Profile& CurrentProfile);

void AddModPacks(Profile& CurrentProfile, std::vector<fs::path> ModPackList);

void CleanLog();