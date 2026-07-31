#include <dynalo/dynalo.hpp>
#define TCORE_USE_CPP_WRAPPER
#include <TCore.h>
#include <UnitTestSystem.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCUnitTest)

int LoadCore()
{
	auto core = dynalo::open("TCore.dll");
	if (!core)
	{
		printf("Failed to load core library");
		return 1;
	}
	auto entryPoint =
		dynalo::get_function<TCORE_PLUGIN_ENTRY_POINT_FUNC_SIGNATURE>(core, TCORE_PLUGIN_ENTRY_POINT_NAME);
	if (!entryPoint)
	{
		printf("Failed to find entry point the core library");
		return 1;
	}
	TCPluginInfo coreInfo{};
	TCPluginFunctions coreFuncs{};
	auto res = entryPoint(TC, &coreInfo, &coreFuncs);
	if (res != TC_RESULTSTATE_SUCCESS)
	{
		printf("Failed to run entry point of core library");
		return 1;
	}
	if (auto res = coreFuncs.Initialize((const void**)&TC); res != TC_RESULTSTATE_SUCCESS)
	{
		printf("Failed to initialize core library");
		return 1;
	}
	return 0;
};

const char* GPluginsToTest[] = {"TCoreVirtualMemory.dll",
								"TCoreAllocator.dll",
								"TCoreFileSystem.dll",
								"TCoreString.dll",
								"TCoreLogger.dll",
								"TCoreProfiler.dll",
								"TCoreWindowing.dll"};
int main()
{
	if (auto res = LoadCore(); res)
		return res;

	const TCPluginInfo* unitTestInfo{};
	if (auto res = TC->LoadPlugin("TCoreUnitTest.dll", nullptr); res != TC_RESULTSTATE_SUCCESS)
	{
		printf("Failed to load unit test system");
		return 1;
	}

	if (auto res =
			TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, &unitTestInfo, (const void**)&TCUnitTest);
		res != TC_RESULTSTATE_SUCCESS)
	{
		printf("Failed to get unit test system");
		return 1;
	}

	for (TU4 pluginIdx = 0; pluginIdx < sizeof(GPluginsToTest) / sizeof(sizeof(GPluginsToTest[0])); pluginIdx++)
	{
		auto plugin = GPluginsToTest[pluginIdx];
		if (auto res = TC->LoadPlugin(plugin, nullptr); res != TC_RESULTSTATE_SUCCESS)
		{
			printf("Failed to load plugin; %s", plugin);
			return 2;
		}
	}

	TCUnitTest->RunAllTests();
	return 0;
}