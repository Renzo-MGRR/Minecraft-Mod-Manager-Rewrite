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
				std::wstring NewAvailableObjectTypes;
				for (Profile& CurrentProfile : Profiles)
				{
					for (ObjectStruct& CurrentObjectStruct : CurrentProfile.ObjectStruct)
					{
						if (Settings.GlobalObjectTypes[i].Type == CurrentObjectStruct.ObjectType.Type)
						{
							CurrentObjectStruct.ObjectType.IsEnabled = Settings.GlobalObjectTypes[i].IsEnabled;
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
		}
	}
}