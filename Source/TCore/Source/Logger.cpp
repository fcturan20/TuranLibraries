// External
#include <assert.h>
#include <stdarg.h>
#include <wchar.h>
#include <codecvt>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

#define TCORE_USE_CPP_WRAPPER
#include "Logger.h"

#if defined(T_ENVWINDOWS)
#include <Windows.h>
#endif

#include "FileSystem.h"
#include "TString.h"
#include "Allocator.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCLog)
TCORE_PLUGIN_INIT(TCStringSys)
TCORE_PLUGIN_INIT(TCFileSystem)
TCORE_PLUGIN_INIT(TCUnitTest)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCLog)
TCORE_PLUGIN_HARD_DEPENDENCY(TCStringSys, TCStringSys_PLUGIN_VERSION)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Log
{

struct Entry
{
	std::string Text;
	TCLogLevel Type;
};

struct TCLogContext* GContext = nullptr;
static constexpr const char* GLogStatusNames[] = {"STATUS", "WARNING", "ERROR", "NOT CODED", "CRASHING"};
static constexpr int GLogConsoleColors[] = {2, 6, 12, 14, 64};
struct TCLogContext
{
	std::filesystem::path MainFilePath;
	std::vector<Entry> LogList;
#if defined(T_ENVWINDOWS)
	HANDLE ConsoleHandle = NULL;
#endif

	TCLogContext() { GContext = this; }

	~TCLogContext() { GContext = nullptr; }

	static void Initialize(const char* logFilePath)
	{
#if defined(T_ENVWINDOWS)
		GContext->ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		if (logFilePath)
			GContext->MainFilePath = logFilePath;
		else
			GContext->MainFilePath = "TCoreLog.txt";
		GContext->LogList.clear();
	}

	static void Save(TCLogLevel logType, const char* filePath)
	{
		if (GContext->LogList.size() == 0 || !TCFileSystem)
			return;

		TCore::String textData;

		for (unsigned int i = 0; i < GContext->LogList.size(); i++)
		{
			const Entry& log = GContext->LogList[i];
			if (logType == TC_LOG_LEVEL_ALL)
				textData += GLogStatusNames[log.Type] + std::string(": ") + log.Text;
			else if (logType == log.Type)
				textData += log.Text;
		}

		if (filePath)
			TCFileSystem->WriteTextFile(textData.CStr(), filePath, false);
		else
			TCFileSystem->WriteTextFile(textData.CStr(), (const char*)GContext->MainFilePath.u8string().c_str(), false);
	}

	static void Log(TCLogLevel type, size_t time_point, const char* owner, const char* text, const char* details)
	{
		GContext->LogList.push_back(Entry());
		Entry& log = GContext->LogList[GContext->LogList.size() - 1];
		log.Text = text;
		log.Type = type;
#if defined(T_ENVWINDOWS)
		SetConsoleTextAttribute(GContext->ConsoleHandle, GLogConsoleColors[type]);
		printf("%s: %s\n", GLogStatusNames[type], text);
		SetConsoleTextAttribute(GContext->ConsoleHandle, 7);
#else
		printf("%s: %s\n", GLogStatusNames[type], text);
#endif
	}
};

class UnitTest
{
public:
	static void Register()
	{
		// TEST 1 - Check if it is possible to log a message and save it file
		{
			TCUnitTestDescription desc{};
			desc.Name = "TCLog_UnitTest_1";
			desc.GlobalCategoryName = "TCore";
			desc.Test = [](TCReadBuffer inputData) -> TCResult {
				const char* initPath = "UnitTestLog.txt";
				TCFileSystem->DeleteFile(initPath);
				TCLog->Initialize(initPath);
				TCLog->Log(TC_LOG_LEVEL_STATUS, 0, "UnitTest", "This is a test log entry.", nullptr);

				auto saveAndCheck = [initPath](const char* path) -> TCResult {
					TCLog->Save(TC_LOG_LEVEL_ALL, path);
					auto data = (const char*)TCFileSystem->ReadTextFile(path ? path : initPath, nullptr);
					if (!data)
						return {TC_RESULTSTATE_FAILURE, 0};
					auto res = strcmp(data, "STATUS: This is a test log entry.\n") == 0 ? TC_RESULTSTATE_SUCCESS
																						: TC_RESULTSTATE_FAILURE;
					delete[] data;
					return {TC_RESULTSTATE_SUCCESS, 0};
				};

				// Check to see if the log entry was saved correctly to init file
				if (auto res = saveAndCheck(nullptr); res != TC_RESULTSTATE_SUCCESS)
					return res;

				if (auto res = saveAndCheck("OveridenLogPath.txt"); res != TC_RESULTSTATE_SUCCESS)
					return res;

				return {TC_RESULTSTATE_SUCCESS, 0};
			};
			TCUnitTest->RegisterTest(&desc);
		}
		// TEST 2 - Test C++ API
		{
			TCUnitTestDescription desc{};
			desc.Name = "TCLog_UnitTest_2";
			desc.GlobalCategoryName = "TCore";
			desc.Test = [](TCReadBuffer inputData) -> TCResult {
				TCore::tl(TC_LOG_LEVEL_STATUS) << "Fine, i'll do it myself!" << td() << "In infinity war ofc :)";
				TCLog->Save(TC_LOG_LEVEL_ALL, nullptr);
				return {TC_RESULTSTATE_SUCCESS, 0};
			};
			TCUnitTest->RegisterTest(&desc);
		}
	}
};
} // namespace Log
} // namespace TCore

TCResult TCLog_Initialize(const void** outPluginAPI)
{
	if (!outPluginAPI)
		return {TC_RESULTSTATE_FAILURE, 0};

	new TCore::Log::TCLogContext();
	auto services = new ITCLog;
	services->Initialize = TCore::Log::TCLogContext::Initialize;
	services->Save = TCore::Log::TCLogContext::Save;
	services->Log = TCore::Log::TCLogContext::Log;
	TCLog = services;
	*outPluginAPI = TCLog;

	if (TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, nullptr, (const void**)&TCUnitTest) ==
		TC_RESULTSTATE_SUCCESS)
		TCore::Log::UnitTest::Register();

	TC->GetPlugin(TCFileSystem_PLUGIN_NAME, TCFileSystem_PLUGIN_VERSION, nullptr, (const void**)&TCFileSystem);

	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCLog_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCLog_Shutdown()
{
	if (TCore::Log::GContext)
	{
		delete TCore::Log::GContext;
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}
void TCLog_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}