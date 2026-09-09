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
		OpenFabricInstaller();
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
		if (ImGui::Button("Reload profiles"))
		{
			std::thread([=]()
			{
				ProfileIndex = 0;
				Profiles.clear();
				GetProfiles(Profiles);
			}).detach();
		}
		Profile& CurrentProfile = Profiles[ProfileIndex];
		ImGui::FileBrowser& ModPackFileBrowser = CurrentProfile.FileBrowserVector.back();
		if (ImGui::Button("Add Modpacks")) //They're called modpacks but they can include all object types, not just mods
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
	std::vector<std::wstring> FabricServerVersions;
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
			std::thread([=]()
				{
					ServerIndex = 0;
					Servers.clear();
					GetServers(Servers);
				}).detach();
		}
		Profile& CurrentServer = Servers[ServerIndex];
		ImGui::FileBrowser& ModPackFileBrowser = CurrentServer.FileBrowserVector.back();
		if (ImGui::Button("Add Modpacks##Servers"))
		{
			ModPackFileBrowser.Open();
		}
		ModPackFileBrowser.Display();
		if (ModPackFileBrowser.HasSelected())
		{
			std::vector<fs::path> ModPackList = ModPackFileBrowser.GetMultiSelected();
			std::thread NewThread(AddModPacks, std::ref(CurrentServer), ModPackList);
			NewThread.detach();
			ModPackFileBrowser.ClearSelected();
		}
		ObjectList(CurrentServer);
		if (DisableServerSelection)
		{
			ImGui::EndDisabled();
		}
	}
}