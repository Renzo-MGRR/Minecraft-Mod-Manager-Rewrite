#include "..\HeadersAndUtilities\Functions.h"
#include <thread>
#include "..\HeadersAndUtilities\Settings.h"

void ProfilesTab()
{
	if (CurrentTask != L"")
	{
		ImGui::Text("%s", wstring2string(CurrentTask).c_str()); // Show current task (such as "Loading profiles") from thread 
	// instead of not showing anything or freezing
	}
	if (DownloadProgress > 0.0f)
	{
		ImGui::ProgressBar(DownloadProgress);
	}
	if (ImGui::Button("Open Fabric Installer"))
	{
		std::thread NewThread(OpenFabricInstaller);
		NewThread.detach();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload profiles"))
	{
		std::thread NewThread(ReloadProfiles, std::ref(Profiles), std::ref(ProfileIndex));
		NewThread.detach();
		return;
	}
	ImGui::Text("Current Profile:");
	if (!Profiles.empty())
	{
		if (DisableProfileSelection)
		{
			ImGui::BeginDisabled();
		}
		ImGui::SameLine();
		ProfileCombo(Profiles, 0, ProfileIndex);
		if (ImGui::Button("Remove profile"))
		{
			ImGui::OpenPopup("Removeprofile");
		}
		if (ImGui::BeginPopup("Removeprofile"))
		{
			ImGui::Text("Are you sure you want to remove this profile?");
			if (ImGui::Button("Remove##Profile"))
			{
				RemoveJsonObject(Profiles[ProfileIndex].Id, ProfilesFile);
				RemoveProfile(Profiles, ProfileIndex);
				ImGui::CloseCurrentPopup();
				return;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel##Profile"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::Button("Rename profile"))
		{
			ImGui::OpenPopup("Renameprofile");
		}
		if (ImGui::BeginPopup("Renameprofile"))
		{
			static std::string ProfileName = wstring2string(Profiles[ProfileIndex].Name);
			static bool ShowError = false;
			if (TextInput(L"New name: ", ProfileName) || ImGui::Button("OK"))
			{
				if (ProfileName != "")
				{
					ShowError = false;
					if (ProfileName != wstring2string(Profiles[ProfileIndex].Name))
					{
						std::ifstream LauncherProfiles(wstring2string(ProfilesFile));
						json data = json::parse(LauncherProfiles);
						LauncherProfiles.close();
						data["profiles"][Profiles[ProfileIndex].Id]["name"] = ProfileName;
						data["profiles"][Profiles[ProfileIndex].Id]["gameDir"] = wstring2string(MinecraftDir) + "\\profiles\\" + ProfileName;
						change_key(data["profiles"], Profiles[ProfileIndex].Id, ProfileName);
						Profiles[ProfileIndex].Name = string2wstring(ProfileName);
						Profiles[ProfileIndex].Id = ProfileName;
						Profiles[ProfileIndex].NameAndVersion = CombineNameAndVersion(Profiles[ProfileIndex]);
						fs::rename(Profiles[ProfileIndex].Directory, MinecraftDir + L"\\profiles\\" + string2wstring(ProfileName));
						Profiles[ProfileIndex].Directory = MinecraftDir + L"\\profiles\\" + string2wstring(ProfileName);
						RewriteProfilesFile(data, ProfilesFile);
					}
					ImGui::CloseCurrentPopup();
				}
				else
				{
					ShowError = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel##renameprofile"))
			{
				ImGui::CloseCurrentPopup();
			}
			if (ShowError)
			{
				ImGui::Text("The new name can't be empty!");
			}
			ImGui::EndPopup();
		}
		ManageObjects(Profiles, ProfileIndex, 0);
		if (DisableProfileSelection)
		{
			ImGui::EndDisabled();
		}
	}
}

void SettingsTab()
{
	if (ImGui::CollapsingHeader("Objects to show per profile"))
	{
		for (int i = 0; i < Settings.GlobalObjectTypes.size(); i++)
		{
			ImGui::Text("%s", (wstring2string(AllObjectTypes[i])).c_str());
			ImGui::SameLine();
			if (ImGui::Checkbox(("##" + std::to_string(i)).c_str(), &Settings.GlobalObjectTypes[i].IsEnabled))
			{
				ChangeObjectTypeAvailability(Settings.GlobalObjectTypes[i]);
			}
		}
	}
}
void ServersTab()
{
	if (ImGui::Button("Add Server"))
	{
		std::thread NewThread(AddServer);
		NewThread.detach();
		ImGui::OpenPopup("##ServerInstallPopup");
	}
	ServerPopup();
	if (CurrentTask != L"")
	{
		ImGui::Text("%s", wstring2string(CurrentTask).c_str());
	}
	if (DownloadProgress > 0.0f)
	{
		ImGui::ProgressBar(DownloadProgress);
	}
	ImGui::Text("Current Server:");
	if (!Servers.empty())
	{
		if (DisableServerSelection)
		{
			ImGui::BeginDisabled();
		}
		ImGui::SameLine();
		ProfileCombo(Servers, 1, ServerIndex);
		if (ImGui::Button("Reload servers"))
		{
			std::thread NewThread(ReloadProfiles, std::ref(Servers), std::ref(ServerIndex));
			NewThread.detach();
			return;
		}
		if (ImGui::Button("Remove server"))
		{
			ImGui::OpenPopup("Removeserver");
		}
		if (ImGui::BeginPopup("Removeserver"))
		{
			ImGui::Text("Are you sure you want to remove this server?");
			if (ImGui::Button("Remove##Server"))
			{
				RemoveProfile(Servers, ServerIndex);
				ImGui::CloseCurrentPopup();
				return;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel##Server"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::Button("Rename server"))
		{
			ImGui::OpenPopup("Renameserver");
		}
		if (ImGui::BeginPopup("Renameserver"))
		{
			static std::string ServerName = wstring2string(Servers[ServerIndex].Name);
			static bool ShowServerError = false;
			if (TextInput(L"New name: ##Server", ServerName) || ImGui::Button("OK##Server"))
			{
				if (ServerName != "")
				{
					ShowServerError = false;
					if (ServerName != wstring2string(Profiles[ProfileIndex].Name))
					{
						Servers[ServerIndex].Name = string2wstring(ServerName);
						Servers[ServerIndex].Id = ServerName;
						Servers[ServerIndex].NameAndVersion = CombineNameAndVersion(Servers[ServerIndex]);
						fs::rename(Servers[ServerIndex].Directory, MinecraftDir + L"\\servers\\" + string2wstring(ServerName));
						Servers[ServerIndex].Directory = MinecraftDir + L"\\servers\\" + string2wstring(ServerName);
					}
					ImGui::CloseCurrentPopup();
				}
				else
				{
					ShowServerError = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel##renameserver"))
			{
				ImGui::CloseCurrentPopup();
			}
			if (ShowServerError)
			{
				ImGui::Text("The new name can't be empty!");
			}
			ImGui::EndPopup();
		}
		ManageObjects(Servers, ServerIndex, 1);
		if (DisableServerSelection)
		{
			ImGui::EndDisabled();
		}
	}
}