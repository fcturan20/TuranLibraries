// External
#include <assert.h>
#include <stdarg.h>
#include <wchar.h>

#include <codecvt>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define TCORE_USE_CPP_WRAPPER
#include "Logger.h"

#if defined(T_ENVWINDOWS)
#include <Windows.h>
#endif

#include "ECS.h"
#include "FileSystem.h"
#include "TString.h"
#include "Allocator.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCLog)
TCORE_PLUGIN_INIT(TCString)
TCORE_PLUGIN_INIT(TCFileSystem)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCLog)
TCORE_PLUGIN_ENTRY_POINT_END()

struct TCLogRecord
{
	std::string Text;
	TCLogLevel Type;
};

struct TCLogContext* GContext = nullptr;
static constexpr const char* GLogStatusNames[] = {"STATUS", "WARNING", "ERROR", "NOT CODED", "CRASHING"};
static constexpr int GLogConsoleColors[] = {2, 6, 12, 14, 64};
struct TCLogContext
{
	std::filesystem::path mainFilePath;
	std::vector<TCLogRecord> logList;
#if defined(T_ENVWINDOWS)
	HANDLE hConsole;
#endif

	static void Initialize(const char* logFilePath)
	{
#if defined(T_ENVWINDOWS)
		GContext->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		if (logFilePath)
			GContext->mainFilePath = logFilePath;
		else
			GContext->mainFilePath = "TCoreLog.txt";
		GContext->logList.clear();
	}

	static void Save(TCLogLevel logType, const char* filePath)
	{
		if (GContext->logList.size() == 0 || !TCFileSystem)
			return;

		TCore::String textData = std::string("abd");

		for (unsigned int i = 0; i < GContext->logList.size(); i++)
		{
			const TCLogRecord& log = GContext->logList[i];
			// If logType is INT32_MAX, this is the main log file save
			if (logType == INT32_MAX)
				textData += log.Text;
			else if (logType == log.Type)
				textData += log.Text;
		}
		TCFileSystem->WriteTextFile(textData.CStr(), (const char*)GContext->mainFilePath.u8string().c_str(), false);
		GContext->logList.clear();
	}

	static void Log(TCLogLevel type, size_t time_point, const char* owner, const char* text, const char* details)
	{
		GContext->logList.push_back(TCLogRecord());
		TCLogRecord& log = GContext->logList[GContext->logList.size() - 1];
		log.Text = text;
		log.Type = type;
#if defined(T_ENVWINDOWS)
		SetConsoleTextAttribute(GContext->hConsole, GLogConsoleColors[type]);
		printf("%s: %s\n", GLogStatusNames[type], text);
		SetConsoleTextAttribute(GContext->hConsole, 7);
#else
		printf("%s: %s\n", GLogStatusNames[type], text);
#endif
	}
};

TCResult TCLog_Initialize(const void** outPluginAPI)
{
	if (!outPluginAPI)
	{
		return TC_RESULT_FAILURE;
	}
	GContext = new TCLogContext();
	auto services = new ITCLog;
	services->Initialize = TCLogContext::Initialize;
	services->Save = TCLogContext::Save;
	services->Log = TCLogContext::Log;
	TCLog = services;
	*outPluginAPI = TCLog;
	return TC_RESULT_SUCCESS;
}

TCResult TCLog_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCLog_Shutdown()
{
	if (GContext)
	{
		delete GContext;
		GContext = nullptr;
	}
	return TC_RESULT_SUCCESS;
}
void TCLog_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}