#include "unittestsys_tapi.h"

#include <stdio.h>
#include <stdlib.h>

#include "ecs_tapi.h"

struct tlUnitTestPriv {
  tlUnitTest*            sys;
  tlUnitTestDescription* v_tests;

  static void add(const wchar_t* name, unsigned int classification_flag,
                  tlUnitTestDescription test_interface) {
    const wchar_t* test_result = L"No result text";
    unsigned char  returnValue = test_interface.test(test_interface.data, &test_result);
    wprintf(L"Test Name: %s\n Return value: %u, Result text: %s\n", name,
            ( unsigned int )returnValue, test_result);
  }

  static void remove(const wchar_t* name, unsigned int classification_flag) {}

  static void run(unsigned int classification_flag, const wchar_t* name,
                  void (*testoutput_visualization)(unsigned char  unittest_result,
                                                   const wchar_t* output_string)) {}
};
tlUnitTestPriv* priv;

#define TAPI_UNITTEST_CLASSFLAG_UT 0
tlUnitTestFnc(tapi_unitTest_bitset0, input, outputStr) {
  if ((( UNITTEST_TAPI_PLUGIN_LOAD_TYPE )(( struct tlECS* )input)
         ->getSystem(UNITTEST_TAPI_PLUGIN_NAME)) != priv->sys) {
    *outputStr = L"Default unit test failed!";
    return 0;
  }
  *outputStr = L"Default unit test succeeded";
  return 1;
}

ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  priv              = ( tlUnitTestPriv* )malloc(sizeof(tlUnitTestPriv));
  priv->sys         = ( tlUnitTest* )malloc(sizeof(tlUnitTest));
  priv->sys->d      = priv;
  priv->sys->add    = &tlUnitTestPriv::add;
  priv->sys->remove = &tlUnitTestPriv::remove;
  priv->sys->run    = &tlUnitTestPriv::run;

  ecsSys->addSystem(UNITTEST_TAPI_PLUGIN_NAME, 0, priv->sys);

  // Test the system itself
  {
    tlUnitTestDescription ut;
    ut.data = ecsSys;
    ut.test = tapi_unitTest_bitset0;
    priv->sys->add(L"Unit Test Default", TAPI_UNITTEST_CLASSFLAG_UT, ut);
  }
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) {
  free(priv->sys);
  free(priv);
}
