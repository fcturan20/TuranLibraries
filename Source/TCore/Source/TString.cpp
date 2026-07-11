#define TCORE_USE_CPP_WRAPPER
#include "TString.h"

// TCore
#include "ECS.h"
#include "Allocator.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCString)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_MEMORY_BLOCK_INIT()
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCString)
TCORE_PLUGIN_HARD_DEPENDENCY(TCAllocator, TCAllocator_PLUGIN_VERSION);
TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(1ull << 30ull);
TCORE_PLUGIN_ENTRY_POINT_END()

// External
#include <string>
struct TCStringContext
{
	// Helper: next power of two >= v
	static size_t NextPowerOfTwo(size_t v)
	{
		size_t p = 1;
		while (p < v)
			p <<= 1;
		return p;
	}

	static TCStringHnd Create(const char* str)
	{
		size_t len = str ? std::strlen(str) : 0;
		// Choose a comfortable initial capacity (power of two), at least 64 bytes
		const size_t minCap = 64;
		size_t capacity = NextPowerOfTwo(std::max(minCap, len + 1));

		TCVectorHnd vec = TCVectorManager->Create(TCore::GSuperMemoryBlock, 1u, capacity, 0);
		if (!vec)
			return nullptr;

		// Push characters including null terminator so internal Count is correct
		if (str)
		{
			for (size_t i = 0; i < len; ++i)
				TCVectorManager->PushBack(vec, &str[i]);
		}
		char nullc = '\0';
		TCVectorManager->PushBack(vec, &nullc);

		return (TCStringHnd)vec;
	}

	static void Destroy(TCStringHnd str) { TCVectorManager->Destroy((TCVectorHnd)str); }

	static void Append(TCStringHnd str, const char* str_to_append)
	{
		if (!str || !str_to_append || !*str_to_append)
			return;

		size_t oldLen = std::strlen((const char*)str);
		size_t addLen = std::strlen(str_to_append);
		size_t capacity = TCVectorManager->Capacity((TCVectorHnd)str);

		// Available space for new chars (excluding null terminator)
		if (capacity <= oldLen + 1)
			return; // no space at all; avoid overflow

		size_t available = capacity - oldLen - 1;
		size_t toCopy = std::min(available, addLen);

		// Append up to available characters
		*((char*)str + oldLen) = str_to_append[0];
		for (size_t i = 1; i < toCopy; ++i)
			TCVectorManager->PushBack((TCVectorHnd)str, &str_to_append[i]);

		// Ensure null terminator at the end (if there is still space, PushBack appended the bytes but we need a final
		// '\0')
		if (oldLen + toCopy + 1 <= capacity)
		{
			char nullc = '\0';
			TCVectorManager->PushBack((TCVectorHnd)str, &nullc);
		}
		else
		{
			// If exactly filled capacity with no room for terminator, replace last byte with '\0'
			((char*)str)[capacity - 1] = '\0';
		}
	}

	static void Clear(TCStringHnd str)
	{
		if (!str)
			return;

		// If there is at least one element committed, overwrite first char with '\0'.
		// If no commitment yet, attempt to push a '\0'.
		size_t cap = TCVectorManager->Capacity((TCVectorHnd)str);
		if (cap == 0)
			return;

		((char*)str)[0] = '\0';

		// If the vector currently has a size of 0, push null terminator to ensure Count becomes 1.
		if (TCVectorManager->Size((TCVectorHnd)str) == 0)
		{
			char nullc = '\0';
			TCVectorManager->PushBack((TCVectorHnd)str, &nullc);
		}
	}

	static void Set(TCStringHnd str, const char* new_str)
	{
		if (!str)
			return;
		if (!new_str)
		{
			Clear(str);
			return;
		}

		size_t newLen = std::strlen(new_str);
		size_t cap = TCVectorManager->Capacity((TCVectorHnd)str);

		// If new string fits into capacity, overwrite in-place.
		if (newLen + 1 <= cap)
		{
			// Overwrite characters
			std::memcpy(str, new_str, newLen);
			// Ensure null terminator
			((char*)str)[newLen] = '\0';

			// If vector's Count is smaller than newLen+1, push remaining chars to update Count.
			size_t curSize = TCVectorManager->Size((TCVectorHnd)str);
			if (curSize < newLen + 1)
			{
				for (size_t i = curSize; i < newLen + 1; ++i)
				{
					char c = ((char*)str)[i];
					TCVectorManager->PushBack((TCVectorHnd)str, &c);
				}
			}
			// If curSize > newLen+1 we cannot shrink Count via public API here; left as-is.
		}
		else
		{
			// New string is larger than capacity — copy as much as fits (truncate) and ensure null terminator.
			size_t toCopy = (cap > 0) ? (cap - 1) : 0;
			if (toCopy)
				std::memcpy(str, new_str, toCopy);
			if (cap > 0)
				((char*)str)[cap - 1] = '\0';
		}
	}

	static const char* CStr(TCStringHnd str) { return (const char*)str; }

	static void Resize(TCStringHnd str, size_t new_capacity)
	{
		// API does not allow safely relocating the buffer (no handle update), so only allow shrinking/truncation
		if (!str)
			return;

		size_t oldLen = TCVectorManager->Size((TCVectorHnd)str);
		size_t curCap = TCVectorManager->Capacity((TCVectorHnd)str);

		if (new_capacity == 0 || new_capacity == curCap)
			return;

		// If new_capacity < current capacity, just truncate the C-string if needed.
		if (new_capacity < curCap)
		{
			size_t cpyLen = std::min((size_t)new_capacity - 1, std::strlen((const char*)str));
			((char*)str)[cpyLen] = '\0';
			// Cannot reliably shrink internal Count through available API; leave Count as-is.
		}
		// If new_capacity > curCap we can't relocate the block without returning a new handle to the caller —
		// leave as-is to avoid corrupting external handle.
	}

	static TCStringHnd Substring(TCStringHnd str, size_t start_index, size_t end_index)
	{
		if (!str || start_index >= end_index)
			return Create("");

		size_t length = end_index - start_index;
		TCStringHnd substr = (TCStringHnd)TCVectorManager->Create(TCore::GSuperMemoryBlock, 1, length + 1, 0);
		if (!substr)
			return nullptr;

		// Fill substr using PushBack so Count is correct
		const char* src = ((const char*)str) + start_index;
		for (size_t i = 0; i < length; ++i)
			TCVectorManager->PushBack((TCVectorHnd)substr, &src[i]);
		char nullc = '\0';
		TCVectorManager->PushBack((TCVectorHnd)substr, &nullc);

		return substr;
	}
};

struct TCStringUnitTests
{
	static TCResult FirstTest(TCReadBuffer input_data);
	static TCResult SecondTest(TCReadBuffer input_data);
	static TCResult ThirdTest(TCReadBuffer input_data);
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
							 &TCStringContext::CStr,
							 &TCStringContext::Resize,
							 &TCStringContext::Substring};
	TCString = api;
	*outPluginAPI = TCString;

	TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, nullptr, (const void**)&TCUnitTest);
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

void TCString_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded) {}

#pragma region Unit Tests

TCResult TCStringUnitTests::FirstTest(TCReadBuffer input_data)
{
	TCStringHnd str = TCString->Create("Hello");
	TCString->Append(str, " World");
	const char* cstr = TCString->CStr(str);
	unsigned int result = 0;
	if (strcmp(cstr, "Hello World") != 0)
		return TC_RESULT_FAILURE;
	return TC_RESULT_SUCCESS;
}

TCResult TCStringUnitTests::SecondTest(TCReadBuffer input_data)
{
	TCStringHnd str = TCString->Create("Hello");
	TCString->Set(str, "New String");
	const char* cstr = TCString->CStr(str);
	if (strcmp(cstr, "New String") != 0)
		return TC_RESULT_FAILURE;
	return TC_RESULT_SUCCESS;
}

TCResult TCStringUnitTests::ThirdTest(TCReadBuffer input_data)
{
	TCStringHnd str = TCString->Create("Hello");
	TCString->Resize(str, 10);
	const char* cstr = TCString->CStr(str);
	if (strlen(cstr) != 5 || strcmp(cstr, "Hello") != 0)
		return TC_RESULT_FAILURE;
	return TC_RESULT_SUCCESS;
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
	{
		TCUnitTestDescription desc{};
		desc.Name = "TCString_UnitTest_1";
		desc.GlobalCategoryName = "TCore";
		desc.Test = SecondTest;
		desc.Data = {nullptr, 0};
		TCUnitTest->RegisterTest(&desc);
	}
	{
		TCUnitTestDescription desc{};
		desc.Name = "TCString_UnitTest_2";
		desc.GlobalCategoryName = "TCore";
		desc.Test = ThirdTest;
		desc.Data = {nullptr, 0};
		TCUnitTest->RegisterTest(&desc);
	}
}