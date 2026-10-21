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
		ManageObjects(Servers, ServerIndex, 1);
		if (DisableServerSelection)
		{
			ImGui::EndDisabled();
		}
	}
}