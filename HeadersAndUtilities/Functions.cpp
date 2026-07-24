#include <thread>
#include <string>
#include <fstream>
#include <ShlObj.h>
#include <curl/curl.h>
#include <vector>
#include <filesystem>
#include "Shlwapi.h"
#include "..\bit7z\bitarchivereader.hpp"
#include "..\bit7z\bitfileextractor.hpp"
#include "..\ImGui\imgui.h"
#include "..\ImGui\imgui_stdlib.h"
#include "Settings.h"
namespace fs = std::filesystem;

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
	}
}

bool IsFileAccessible(const std::string& filename)
{
	std::ifstream file(filename);
	return file.good();
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
std::vector<std::wstring> GetProfiles()
{
	CurrentTask = L"Loading profiles...";
	std::vector<std::wstring> Profiles = getInDirectoryW(GetAppData() + L"\\.minecraft\\profiles", true);
	CurrentTask = L"";
	return Profiles;
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