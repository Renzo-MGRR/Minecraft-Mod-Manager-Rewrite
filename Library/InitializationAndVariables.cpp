#include "..\HeadersAndUtilities\InitializationAndVariables.h"
#include "..\HeadersAndUtilities\Settings.h"
#include "..\HeadersAndUtilities\Functions.h"
#include <thread>
std::vector<std::wstring> Profiles;
std::wstring CurrentTask = L"";
void Initialization()
{
	std::thread([=]()
	{
		Profiles = GetProfiles();
		CleanLog();
		
	}).detach();
}