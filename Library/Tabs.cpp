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
	if (ImGui::Button("Open Fabric Installer"))
	{
		OpenFabricInstaller();
	}
	ImGui::Text("Current Profile:");
	if (!Profiles.empty())
	{
		ImGui::SameLine();
		ProfileCombo(Profiles, 0, ProfileIndex);
		if (ImGui::Button("Reload profiles"))
		{
			std::thread([=]()
			{
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