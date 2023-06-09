#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define UNITTEST_TAPI_PLUGIN_NAME "tapi_unittest"
#define UNITTEST_TAPI_PLUGIN_LOAD_TYPE struct tapi_unitTestSys_type*

// UnitTests are pretty much new to me so this interface is very abstract
// output_string: Test results may be displayed differently according to how unittestsys_tapi is
// compiled, so return at least a string.
struct tapi_unitTest_interface {
  void* data;
  unsigned char (*test)(void* data, const wchar_t** output_string);
};
#define TAPI_UNITTEST_FUNC(funcName, inputDataName, outputStringName) \
  unsigned char funcName(void* inputDataName, const wchar_t** outputStringName)

struct tapi_unitTestSys {
  // name: Name of the unit test
  // classification_flag: Classify your unit tests to seperate types of your tests
  void (*add_unittest)(const wchar_t* name, unsigned int classification_flag,
                        struct tapi_unitTest_interface test_interface);

  void (*remove_unittest)(const wchar_t* name, unsigned int classification_flag);

  // Run tests with the specified classification_flag
  // classification_flag: Unit tests with classified flags are run. If value is zero, name is used.
  // name: If classification_flag is 0, unit tests with matching name are run.
  // testoutput_visualization: Implementation may use it to display test results...
  //   Each unittest is run after the previous one finished.
  void (*run_tests)(unsigned int classification_flag, const wchar_t* name,
                    void (*testoutput_visualization)(unsigned char  unittest_result,
                                                     const wchar_t* output_string));
};

// Unit Test System
// System that allows reserve, commit and free operations across platforms
struct tapi_unitTestSys_type {
  // RegistrySys Documentation: Storing Functions for Systems
  struct tapi_unitTestSys* funcs;
  // RegistrySys Documentation: Storing Data for Systems
  struct tapi_unitTestSys_d* data;
};

#ifdef __cplusplus
}
#endif