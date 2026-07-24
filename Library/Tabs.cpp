#include "..\ImGui\imgui.h"
#include "..\HeadersAndUtilities\InitializationAndVariables.h"
#include "..\HeadersAndUtilities\Functions.h"
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
		Combo(Profiles, 0);
	}
	ImGui::EndTabItem();
}