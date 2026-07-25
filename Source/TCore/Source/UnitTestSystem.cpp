// External
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string>
#include <vector>
#if defined(T_ENVWINDOWS)
#include <Windows.h>
#endif

#define TCORE_USE_CPP_WRAPPER
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCUnitTest)
TCORE_PLUGIN_ENTRY_POINT_END()

// TCore APIs
#include "Logger.h"

namespace TCore
{
namespace UnitTest
{

struct UnitTest
{
	std::string Name;
	std::string Category;
	TCReadBuffer Data;
	TCResult (*Test)(TCReadBuffer inputData);
};

struct TCUnitTestContext* GContext = nullptr;
struct TCUnitTestContext
{
	UnitTest Tests[1024];
	TU4 TestCount = 0;
	static void Initialize() { GContext = new TCore::UnitTest::TCUnitTestContext(); }
	static void RegisterTest(const TCUnitTestDescription* desc)
	{
		auto& test = GContext->Tests[GContext->TestCount++];
		test.Name = desc->Name;
		if (desc->GlobalCategoryName)
			test.Category = test.Category;
		test.Test = desc->Test;
		test.Data = desc->Data;
	}

	static void UnregisterTest(const char* name) {}

	static void RunTest(const char* name, TCReadBuffer inputData) {}

	static void RunAllTests()
	{
		for (TU4 indx = 0; indx < GContext->TestCount; indx++)
		{
			auto& test = GContext->Tests[indx];
			if (auto res = test.Test(test.Data); res != TC_RESULTSTATE_SUCCESS)
				printf("Test failed: %s\n", test.Name.c_str());
			else
				printf("Test successful: %s\n", test.Name.c_str());
		}
	}

	static char* WaitForInput(TBool warningBell, const char* print, ...)
	{
#ifdef T_ENVWINDOWS
		if (warningBell)
			Beep(1000, 200);
#else
		printf("\a");
		fflush(stdout);
#endif

		if (!print)
			print = "";

		// First compute required buffer size
		va_list args;
		va_start(args, print);
		int required = std::vsnprintf(nullptr, 0, print, args);
		va_end(args);

		std::vector<char> buffer;
		if (required > 0)
		{
			// allocate required + 1 for null terminator
			buffer.resize((size_t)required + 1);
			va_start(args, print);
			std::vsnprintf(buffer.data(), buffer.size(), print, args);
			va_end(args);
		}
		else
		{
			// fallback to empty string
			buffer.resize(1);
			buffer[0] = '\0';
		}

		printf(print);

		static constexpr TU8 maxLength = 4096;
		char* b = new char[maxLength];
		if (!fgets(b, maxLength, stdin))
			return nullptr;

		return b;
	}
};

} // namespace UnitTest
} // namespace TCore

TCResult TCUnitTest_Initialize(const void** outPluginAPI)
{
	TCore::UnitTest::TCUnitTestContext::Initialize();

	auto* sys = new ITCUnitTest();
	sys->RegisterTest = TCore::UnitTest::TCUnitTestContext::RegisterTest;
	sys->UnregisterTest = TCore::UnitTest::TCUnitTestContext::UnregisterTest;
	sys->RunAllTests = TCore::UnitTest::TCUnitTestContext::RunAllTests;
	sys->RunTests = nullptr;
	sys->RunTest = TCore::UnitTest::TCUnitTestContext::RunTest;
	sys->WaitForInput = TCore::UnitTest::TCUnitTestContext::WaitForInput;

	TCUnitTest = sys;
	*outPluginAPI = TCUnitTest;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCUnitTest_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCUnitTest_Shutdown()
{
	if (TCore::UnitTest::GContext)
	{
		delete TCore::UnitTest::GContext;
		TCore::UnitTest::GContext = nullptr;
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCUnitTest_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}