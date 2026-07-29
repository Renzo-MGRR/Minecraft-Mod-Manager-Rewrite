#include "..\HeadersAndUtilities\Functions.h"
#include <thread>
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
	ImGui::EndTabItem();
}