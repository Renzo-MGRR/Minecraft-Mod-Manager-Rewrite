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
				Profiles = GetProfiles();
			}).detach();
		}
		Profile& CurrentProfile = Profiles[ProfileIndex];
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