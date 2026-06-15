#include "UnitTestSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

TCORE_PLUGIN_INIT(TS, TC)
TCORE_PLUGIN_INIT(TSUT, TCUnitTest)

struct TCUnitTestContext {
};
TCUnitTestContext* TSUTContext = nullptr;

struct TCUnitTestServices{
  static void RegisterTest(const TCUnitTestDescription* desc) {
    printf("Registered unit test: %s\n", desc->Name);
  }

  static void UnregisterTest(const char* name) {}

  static void RunTest(const char* name, TCReadBuffer inputData) {}
};

void BindPluginFunctions(TCPluginFunctions* outPluginFunctions){
  outPluginFunctions->Initialize = [](const void** outPluginAPI) -> TCResult {
    TSUTContext = new TCUnitTestContext();
    auto sys = new TCUnitTest();
    sys->RegisterTest = TCUnitTestServices::RegisterTest;
    sys->UnregisterTest = TCUnitTestServices::UnregisterTest;
    sys->RunAllTests = nullptr;
    sys->RunTests   = nullptr;
    sys->RunTest    = TCUnitTestServices::RunTest;

    TSUT = sys;
    *outPluginAPI = TSUT;
    return TC_RESULT_SUCCESS;
  };
  outPluginFunctions->OnPreShutdown = []() -> TCResult {
    delete TSUTContext;
    TSUTContext = nullptr;
    return TC_RESULT_SUCCESS;
  };
  outPluginFunctions->Shutdown            = []() -> TCResult { return TC_RESULT_SUCCESS; };
  outPluginFunctions->OnPluginLoadStateChange = [](const TCPluginInfo* pluginInfo, bool isLoaded) {};
}

TCORE_PLUGIN_ENTRY_POINT_START(TSUT)
TCORE_PLUGIN_ENTRY_POINT_END()