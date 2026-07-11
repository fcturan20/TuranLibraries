#define T_INCLUDE_PLATFORM_LIBS
#include "Bitset.h"

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

#include "ECS.h"
#include "String.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCBitset)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCBitset)
TCORE_PLUGIN_ENTRY_POINT_END()

struct TCBitset
{
	bool* Array;
	unsigned int ByteLength;
};

struct TCBitsetContext
{
	static TCBitsetHnd CreateBitset(unsigned int byte_length)
	{
		TCBitsetHnd bitset = (TCBitsetHnd)malloc(sizeof(TCBitsetHnd) + (sizeof(bool) * byte_length));
		bitset->Array = (bool*)(bitset + 1);
		memset(bitset->Array, 0, byte_length);
		bitset->ByteLength = byte_length;
		return bitset;
	}
	static void DestroyBitset(TCBitsetHnd hnd)
	{
		TCBitsetHnd bitset = (TCBitsetHnd)hnd;
		free(bitset);
	}
	static void SetBit(TCBitsetHnd set, unsigned int index, unsigned char set_true)
	{
		if (index / 8 > set->ByteLength - 1)
		{
			std::cout << "There is no such bit, maximum bit index: " << (set->ByteLength * 8) - 1 << std::endl;
			return;
		}
		char& byte = ((char*)set->Array)[index / 8];
		unsigned int bitindex = (index % 8);
		if (set_true)
		{
			byte = (byte | char(1 << bitindex));
		}
		else
		{
			switch (bitindex)
			{
			case 0: byte = (byte & 254); break;
			case 1: byte = (byte & 253); break;
			case 2: byte = (byte & 251); break;
			case 3: byte = (byte & 247); break;
			case 4: byte = (byte & 239); break;
			case 5: byte = (byte & 223); break;
			case 6: byte = (byte & 191); break;
			case 7: byte = (byte & 127); break;
			}
		}
	}
	static unsigned char GetBitValue(const TCBitsetHnd set, unsigned int index)
	{
		unsigned char byte = ((unsigned char*)set->Array)[index / 8];
		unsigned char bitindex = (index % 8);
		if (byte & (1 << bitindex))
		{
			return true;
		}
		return false;
	}
	static unsigned int GetByteLength(const TCBitsetHnd set) { return set->ByteLength; }
	static unsigned int GetFirstBitIndx(const TCBitsetHnd set, unsigned char find_true)
	{
		if (find_true)
		{
			unsigned int byteindex = 0;
			for (unsigned int byte_value = 0; byte_value == 0; byteindex++)
			{
				byte_value = ((char*)set->Array)[byteindex];
			}
			byteindex--;
			bool found = false;
			unsigned int bitindex = 0;
			for (bitindex = 0; !found; bitindex++)
			{
				found = GetBitValue(set, (byteindex * 8) + bitindex);
			}
			bitindex--;
			return (byteindex * 8) + bitindex;
		}
		else
		{
			unsigned int byteindex = 0;
			for (unsigned int byte_value = 255; byte_value == 255; byteindex++)
			{
				byte_value = ((char*)set->Array)[byteindex];
			}
			byteindex--;
			bool found = false;
			unsigned int bitindex = 0;
			for (bitindex = 0; !found; bitindex++)
			{
				found = !GetBitValue(set, (byteindex * 8) + bitindex);
			}
			bitindex--;
			return (byteindex * 8) + bitindex;
		}
	}

	static void ClearBitset(TCBitsetHnd set, unsigned char set_true) { memset(set->Array, set_true, set->ByteLength); }

	static void ExpandBitset(TCBitsetHnd set, unsigned int expand_size)
	{
		bool* newBlock = new bool[expand_size + set->ByteLength];
		if (newBlock)
		{
			// This is a little bit redundant because all memory initialized with 0 at start
			// And when a memory block is deleted, all bytes are 0
			// But that doesn't hurt too much I think
			memset(newBlock, 0, expand_size + set->ByteLength);

			memcpy(newBlock, set->Array, set->ByteLength);
			set->ByteLength += expand_size;
			delete[] set->Array;
			set->Array = newBlock;
		}
		else
		{
			std::cout << "Bitset expand has failed because Allocator has failed!\n";
			int x;
			std::cin >> x;
		}
	}
};

/////////////////////////////////////////////// Unit Tests
#include <vector>

struct TCBitsetUnitTests
{
	static TCResult FindFirstBitset(TCReadBuffer input_data);
	static TCResult SetBitset(TCReadBuffer input_data);
	static void Register();
};
uint32_t FindFirst(std::vector<bool>& stdBitset, bool isTrue)
{
	for (uint32_t i = 0; i < stdBitset.size(); i++)
	{
		if (stdBitset[i] == isTrue)
		{
			return i;
		}
	}
	return UINT32_MAX;
}

TCResult TCBitsetUnitTests::FindFirstBitset(TCReadBuffer input_data)
{
	static constexpr uint32_t bitsetByteLength = 10 << 10;
	std::vector<bool> stdBitset(bitsetByteLength * 8, false);
	TCBitsetHnd bitset = TCBitset->CreateBitset(bitsetByteLength);

	time_t t;
	srand((unsigned)time(&t));
	for (uint32_t i = 0; i < bitsetByteLength * 8; i++)
	{
		uint32_t bit = rand() % (bitsetByteLength * 8);
		bool v = rand() % 2;
		stdBitset[bit] = v;
		TCBitset->SetBit(bitset, bit, v);
	}
	if (FindFirst(stdBitset, true) != TCBitset->GetFirstBitIndx(bitset, true) ||
		FindFirst(stdBitset, false) != TCBitset->GetFirstBitIndx(bitset, false))
		return TC_RESULT_FAILURE;
	for (uint32_t i = 0; i < bitsetByteLength * 8; i++)
	{
		unsigned char tapiV = TCBitset->GetBitValue(bitset, i);
		unsigned char stdV = stdBitset[i];
		if (stdV != tapiV)
			return TC_RESULT_FAILURE;
	}
	return TC_RESULT_SUCCESS;
}

TCResult TCBitsetUnitTests::SetBitset(TCReadBuffer input_data)
{
	static constexpr uint32_t bitsetByteLength = 10 << 10;
	std::vector<bool> stdBitset(bitsetByteLength * 8, false);
	TCBitsetHnd bitset = TCBitset->CreateBitset(bitsetByteLength);

	time_t t;
	srand((unsigned)time(&t));
	for (uint32_t i = 0; i < bitsetByteLength * 8; i++)
	{
		uint32_t bit = rand() % (bitsetByteLength * 8);
		bool v = rand() % 2;
		stdBitset[bit] = v;
		TCBitset->SetBit(bitset, bit, v);
	}
	for (uint32_t i = 0; i < bitsetByteLength * 8; i++)
	{
		unsigned char tapiV = TCBitset->GetBitValue(bitset, i);
		unsigned char stdV = stdBitset[i];
		if (stdV != tapiV)
			return TC_RESULT_FAILURE;
	}
	return TC_RESULT_SUCCESS;
}

void TCBitsetUnitTests::Register()
{
	if (TCUnitTest = (ITCUnitTest*)TC->GetPlugin("tcUnitTestSystem", 0, nullptr, nullptr); !TCUnitTest)
		return;

	TCUnitTestDescription desc1;
	desc1.Name = "FindFirstBitset";
	desc1.GlobalCategoryName = "Bitset";
	desc1.Test = FindFirstBitset;
	desc1.Data = {nullptr, 0};
	TCUnitTest->RegisterTest(&desc1);

	TCUnitTestDescription desc2;
	desc2.Name = "SetBitset";
	desc2.GlobalCategoryName = "Bitset";
	desc2.Test = SetBitset;
	desc2.Data = {nullptr, 0};
	TCUnitTest->RegisterTest(&desc2);
}
TCResult TCBitset_Initialize(const void** outBitsetAPI)
{
	// Fill plugin API struct with function pointers
	{
		auto services = new ITCBitset;
		services->CreateBitset = TCBitsetContext::CreateBitset;
		services->DestroyBitset = TCBitsetContext::DestroyBitset;
		services->SetBit = TCBitsetContext::SetBit;
		services->GetBitValue = TCBitsetContext::GetBitValue;
		services->GetByteLength = TCBitsetContext::GetByteLength;
		services->GetFirstBitIndx = TCBitsetContext::GetFirstBitIndx;
		services->ClearBitset = TCBitsetContext::ClearBitset;
		services->Expand = TCBitsetContext::ExpandBitset;

		TCBitset = services;
		*outBitsetAPI = TCBitset;
	}

	// Register unit tests
	TCBitsetUnitTests::Register();

	return TC_RESULT_SUCCESS;
}

void TCBitset_OnPreShutdown(const TCPluginInfo* pluginInfo, bool isLoaded) {}

TCResult TCBitset_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCBitset_Shutdown()
{
	return TC_RESULT_SUCCESS;
}

void TCBitset_OnPluginLoadStateChange(const TCPluginInfo* info, TBool isLoaded) {}