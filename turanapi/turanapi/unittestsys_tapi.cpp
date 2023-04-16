#include "unittestsys_tapi.h"

#include <stdio.h>
#include <stdlib.h>

#include "ecs_tapi.h"

unittestsys_tapi_type* utsys;

typedef struct unittestsys_tapi_d {
  unittest_interface_tapi* tests;
} unittestsys_tapi_d;

void tapi_utAdd(const wchar_t* name, unsigned int classification_flag,
                 unittest_interface_tapi test_interface) {
  const wchar_t* test_result = L"No result text";
  unsigned char returnValue = test_interface.test(test_interface.data, &test_result);
  wprintf(L"Test Name: %s\n Return value: %u, Result text: %s\n", name, ( unsigned int )returnValue,
         test_result);
}

void tapi_utRemove(const wchar_t* name, unsigned int classification_flag) {}

void tapi_utRunTests(unsigned int classification_flag, const wchar_t* name,
                     void (*testoutput_visualization)(unsigned char unittest_result,
                                                      const wchar_t*   output_string)) {}

#define TAPI_UNITTEST_CLASSFLAG_UT 0
TAPI_UNITTEST_FUNC(tapi_unitTest_bitset0, input, outputStr) {
  if ((( UNITTEST_TAPI_PLUGIN_LOAD_TYPE )(( ecs_tapi* )input)->getSystem(UNITTEST_TAPI_PLUGIN_NAME)) !=
      utsys) {
    *outputStr = L"Default unit test failed!";
    return 0;
  }
  *outputStr = L"Default unit test succeeded";
  return 1;
}

ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  utsys        = ( unittestsys_tapi_type* )malloc(sizeof(unittestsys_tapi_type));
  utsys->data  = ( unittestsys_tapi_d* )malloc(sizeof(unittestsys_tapi_d));
  utsys->funcs = ( unittestsys_tapi* )malloc(sizeof(unittestsys_tapi));

  utsys->funcs->add_unittest    = &tapi_utAdd;
  utsys->funcs->remove_unittest = &tapi_utRemove;
  utsys->funcs->run_tests       = &tapi_utRunTests;

  ecsSys->addSystem(UNITTEST_TAPI_PLUGIN_NAME, 0, utsys);

  // Test the system itself
  {
    unittest_interface_tapi ut;
    ut.data = ecsSys;
    ut.test = tapi_unitTest_bitset0;
    utsys->funcs->add_unittest(L"Unit Test Default", TAPI_UNITTEST_CLASSFLAG_UT, ut);
  }
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) {
  free(utsys->data);
  free(utsys->funcs);
  free(utsys);
}
