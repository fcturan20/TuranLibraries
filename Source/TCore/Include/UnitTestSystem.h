#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCUnitTest, "tcUnitTestSystem", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

// Unit Test System
// System that allows reserve, commit and free operations across platforms

typedef struct TCUnitTestDescription
{
	// Name of the unit test. Owner plugin's name is prefixed to the unit test name.
	// So, if a plugin named "MyPlugin" registers a unit test named "MyTest", the full name of the
	// unit test will be "MyPlugin.MyTest". This ensures that unit test names are unique across
	// different plugins.
	const char* Name;
	// Classification of the unit test, can be used to group the test with tests of other plugins.
	const char* GlobalCategoryName;
	TCReadBuffer Data; // Data that will be passed to the test function
	// If test returns 0, test is successful
	TCResult (*Test)(TCReadBuffer inputData);
	// Interactive tests will run either first or last
	TBool IsInteractive;
} TCUnitTestDescription;

typedef struct ITCUnitTest
{
	void (*RegisterTest)(const TCUnitTestDescription* desc);
	void (*UnregisterTest)(const char* name);

	void (*RunAllTests)();
	void (*RunTests)(const char* CategoryName);
	// Run a single test by its name. If the test is not found, it will return an error.
	// If inputData is not empty, it will be passed to the test function. Otherwise default data will
	// be used. If the test returns 0, it is successful. Otherwise, it is failed.
	void (*RunTest)(const char* name, TCReadBuffer inputData);

	// Interactive test helpers
	char* (*WaitForInput)(TBool warningBell, const char* printf, ...);
} ITCUnitTest;

TCORE_END_C_LINKAGE