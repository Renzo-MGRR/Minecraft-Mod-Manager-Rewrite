#pragma once
#include <string>
#include <curl/curl.h>
#include "..\bit7z\bitarchivereader.hpp"
#include "..\bit7z\bitfileextractor.hpp"
#include "InitializationAndVariables.h"
#include "..\IniReader\IniReader.h"
#include "Settings.h"

void OpenFabricInstaller();

void ChangeObjectTypeAvailability(ObjectType& ObjectType);

std::vector<ObjectType> GetAvailableObjectTypes(CIniReader& IniReaderObject);

std::wstring GetCurrentFolder();

void WriteToLog(const std::wstring& LoggingLine);

bool IsFileAccessible(const std::string& filename);

std::wstring string2wstring(const std::string& str);

std::string wstring2string(const std::wstring& wstr);

static size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream); //Writes file to a void pointer

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

std::vector<std::wstring> getInDirectoryW(const std::wstring& directory, bool getFolder);

std::vector<std::wstring> getAllInDirectoryW(const std::wstring& directory);

void DownloadFile(std::wstring file, std::string url);

std::vector<bit7z::byte_t> ExtractToMemory(std::wstring extractFrom, std::wstring fileToGet);

void ExtractToDisk(std::wstring extractFrom, std::wstring fileToGet);

void DownloadBar();

void Combo(std::vector<std::wstring> Vector, int id, int VectorSelectedIndex = 0);

void ProfileCombo(std::vector<Profile> Vector, int id, int VectorSelectedIndex = 0);

bool TextInput(std::wstring Description, std::string& StringHandler);

void Execute(const std::wstring& cmdLine, const std::wstring& RunFrom, bool Silent, bool WaitTillFinish);

std::wstring GetAppData();

bool LoadModdedProfileData(Profile& CurrentProfile, json& ProfileJson, std::string Id);

std::vector<Profile> GetProfiles();

bool ProfilesFound(std::wstring ProfilesFile);

bool ParseVersion(Profile& CurrentProfile, json& profile);

std::wstring CombineNameAndVersion(Profile& CurrentProfile);

std::wstring JoinAuthorsFromJson(const json& ModJson);

ProfileDirectories CalculateProfileDirectories(json& ProfileJson);

void DisableObject(ProfileDirectories& ProfileDirs, ObjectStruct& ObjectVector, int ObjectIndex);

void ObjectList(Profile& CurrentProfile);

void CleanLog();