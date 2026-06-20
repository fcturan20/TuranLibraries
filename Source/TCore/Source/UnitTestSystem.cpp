// External
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define TCORE_USE_CPP_WRAPPER
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCUnitTest)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCUnitTest)
TCORE_PLUGIN_ENTRY_POINT_END()

// TCore APIs
#include "Logger.h"

struct TCUnitTestContext
{
	static void RegisterTest(const TCUnitTestDescription* desc) { printf("Registered unit test: %s\n", desc->Name); }

	static void UnregisterTest(const char* name) {}

	static void RunTest(const char* name, TCReadBuffer inputData) {}
};
TCUnitTestContext* Context = nullptr;

TCResult TCUnitTest_Initialize(const void** outPluginAPI)
{
	Context = new TCUnitTestContext();
	auto* sys = new ITCUnitTest();
	sys->RegisterTest = TCUnitTestContext::RegisterTest;
	sys->UnregisterTest = TCUnitTestContext::UnregisterTest;
	sys->RunAllTests = nullptr;
	sys->RunTests = nullptr;
	sys->RunTest = TCUnitTestContext::RunTest;

	TCUnitTest = sys;
	*outPluginAPI = TCUnitTest;
	return TC_RESULT_SUCCESS;
}

TCResult TCUnitTest_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCUnitTest_Shutdown()
{
	if (Context)
	{
		delete Context;
		Context = nullptr;
	}
	return TC_RESULT_SUCCESS;
}

void TCUnitTest_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}