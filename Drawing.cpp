#include "Drawing.h"
#include "HeadersAndUtilities\Settings.h"
#include "HeadersAndUtilities\Tabs.h"
#include <chrono>
#include <thread>
#include <string>
#include "ImGui\imgui_stdlib.h"
#include "ImGui\imfilebrowser.h"

LPCSTR Drawing::lpWindowName = "Minecraft Mod Manager";
ImGuiWindowFlags Drawing::WindowFlags = 0;
bool Drawing::bDraw = true;
void Drawing::Active() { bDraw = true; }
bool Drawing::isActive() { return bDraw == true; }

bool Init = true;

float x = iniReader.ReadFloat("Settings", "WindowXSize", 500);
float y = iniReader.ReadFloat("Settings", "WindowYSize", 500);
ImVec2 Drawing::vWindowSize = ImVec2(x, y);
void Drawing::Draw()
{
	if (Init)
	{
		Initialization();
		Init = false;
	}
	if (isActive())
	{
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		if (ImGui::Begin(lpWindowName, &bDraw, WindowFlags))
		{
			ImVec2 currentSize = ImGui::GetWindowSize();
			if (vWindowSize.x != currentSize.x)
			{
				iniReader.WriteFloat("Settings", "WindowXSize", currentSize.x);
			}
			if (vWindowSize.y != currentSize.y)
			{
				iniReader.WriteFloat("Settings", "WindowYSize", currentSize.y);
			}
			if (ImGui::BeginTabBar("NOTITLE", ImGuiTabBarFlags_NoTooltip))
			{
				if (ImGui::BeginTabItem("Profiles"))
				{
					ProfilesTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Settings"))
				{
					SettingsTab();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}
}
