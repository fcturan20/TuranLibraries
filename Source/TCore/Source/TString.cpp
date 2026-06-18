#include "TString.h"

// TCore
#include "ECS.h"
#include "Allocator.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCString)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCString)
TCORE_PLUGIN_ENTRY_POINT_END()

// External
#include <string>

struct TCStringContext
{
	static TCStringHandle Create(const char* str)
	{
		return (TCStringHandle)TCVectorManager->Create(NULL, 1, sizeof(char) * (strlen(str) + 1), 0);
	}

	static void Destroy(TCStringHandle str) { TCVectorManager->Destroy((TCVectorHandle)str); }

	static void Append(TCStringHandle str, const char* str_to_append)
	{
		size_t newLength = 0;
		if (str)
			newLength += strlen((const char*)str);
		if (str_to_append)
			newLength += strlen(str_to_append);
		if (!newLength)
			return;
		newLength += 1; // For null terminator
		size_t oldCapacity = TCVectorManager->Capacity((TCVectorHandle)str);
		// If the new length exceeds the current capacity, resize the string
		if (newLength > oldCapacity)
		{
			str = (TCStringHandle)TCVectorManager->Resize((TCVectorHandle)str, newLength);
		}
		strcat((char*)str, str_to_append);
	}

	static void Clear(TCStringHandle str) { TCVectorManager->Destroy((TCVectorHandle)str); }

	static void Set(TCStringHandle str, const char* new_str)
	{
		size_t newLength = strlen(new_str) + 1;
		str = (TCStringHandle)TCVectorManager->Resize((TCVectorHandle)str, newLength);
		strcpy((char*)str, new_str);
	}

	static const char* C_str(TCStringHandle str) { return (const char*)str; }

	static void Resize(TCStringHandle str, size_t new_capacity)
	{
		size_t oldLength = TCVectorManager->Size((TCVectorHandle)str);
		str = (TCStringHandle)TCVectorManager->Resize((TCVectorHandle)str, new_capacity);
		if (oldLength > new_capacity)
			((char*)str)[new_capacity - 1] = '\0'; // Truncate the string if new capacity is smaller
	}

	static TCStringHandle Substring(TCStringHandle str, size_t start_index, size_t end_index)
	{
		size_t length = end_index - start_index;
		TCStringHandle substr = (TCStringHandle)TCVectorManager->Create(NULL, 1, sizeof(char) * (length + 1), 0);
		strncpy((char*)substr, ((const char*)str) + start_index, length);
		((char*)substr)[length] = '\0';
		return substr;
	}
};

struct TCStringUnitTests
{
	static unsigned int FirstTest(TCReadBuffer input_data);
	static void Register();
};

TCResult TCString_Initialize(const void** outPluginAPI)
{
	if (!outPluginAPI)
		return TC_RESULT_INVALID_ARGUMENT;
	auto api = new ITCString{&TCStringContext::Create,
							 &TCStringContext::Destroy,
							 &TCStringContext::Append,
							 &TCStringContext::Clear,
							 &TCStringContext::Set,
							 &TCStringContext::C_str,
							 &TCStringContext::Resize,
							 &TCStringContext::Substring};
	TCString = api;
	*outPluginAPI = TCString;

	if (TCUnitTest)
		TCStringUnitTests::Register();

	return TC_RESULT_SUCCESS;
}

TCResult TCString_Shutdown()
{
	delete TCString;
	TCString = nullptr;
	return TC_RESULT_SUCCESS;
}

TCResult TCString_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

void TCString_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {}

#pragma region Unit Tests

unsigned int TCStringUnitTests::FirstTest(TCReadBuffer input_data)
{
	TCStringHandle str = TCString->Create("Hello");
	TCString->Append(str, " World");
	const char* cstr = TCString->C_str(str);
	unsigned int result = 0;
	if (strcmp(cstr, "Hello World") != 0)
	{
		printf("Test failed: Expected 'Hello World', got '%s'\n", cstr);
		result = 1;
	}
	else
		printf("Test passed: '%s'\n", cstr);
	return result;
}

void TCStringUnitTests::Register()
{
	if (!TCUnitTest)
		return;

	{
		TCUnitTestDescription desc{};
		desc.Name = "TCString_UnitTest_0";
		desc.GlobalCategoryName = "TCore";
		desc.Test = FirstTest;
		desc.Data = {nullptr, 0};
		TCUnitTest->RegisterTest(&desc);
	}
}