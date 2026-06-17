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
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCBitset)
TCORE_PLUGIN_ENTRY_POINT_END()

struct tlBitset {
  bool*        m_array;
  unsigned int m_byteLength;
};

struct TCBitsetContext {
  static struct tlBitset* CreateBitset(unsigned int byteLength) {
    struct tlBitset* bitset =
      ( struct tlBitset* )malloc(sizeof(struct tlBitset*) + (sizeof(bool) * byteLength));
    bitset->m_array = ( bool* )(bitset + 1);
    memset(bitset->m_array, 0, byteLength);
    bitset->m_byteLength = byteLength;
    return bitset;
  }
  static void DestroyBitset(struct tlBitset* hnd) {
    struct tlBitset* bitset = ( struct tlBitset* )hnd;
    free(bitset);
  }
  static void SetBit(struct tlBitset* set, unsigned int index, unsigned char setTrue) {
    if (index / 8 > set->m_byteLength - 1) {
      std::cout << "There is no such bit, maximum bit index: " << (set->m_byteLength * 8) - 1
                << std::endl;
      return;
    }
    char&        byte     = (( char* )set->m_array)[index / 8];
    unsigned int bitindex = (index % 8);
    if (setTrue) {
      byte = (byte | char(1 << bitindex));
    } else {
      switch (bitindex) {
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
  static unsigned char GetBitValue(const struct tlBitset* set, unsigned int index) {
    unsigned char byte     = (( unsigned char* )set->m_array)[index / 8];
    unsigned char bitindex = (index % 8);
    if (byte & (1 << bitindex)) {
      return true;
    }
    return false;
  }
  static unsigned int GetByteLength(const struct tlBitset* set) { return set->m_byteLength; }
  static unsigned int GetFirstBitIndx(const struct tlBitset* set, unsigned char findTrue) {
    if (findTrue) {
      unsigned int byteindex = 0;
      for (unsigned int byte_value = 0; byte_value == 0; byteindex++) {
        byte_value = (( char* )set->m_array)[byteindex];
      }
      byteindex--;
      bool         found    = false;
      unsigned int bitindex = 0;
      for (bitindex = 0; !found; bitindex++) {
        found = GetBitValue(set, (byteindex * 8) + bitindex);
      }
      bitindex--;
      return (byteindex * 8) + bitindex;
    } else {
      unsigned int byteindex = 0;
      for (unsigned int byte_value = 255; byte_value == 255; byteindex++) {
        byte_value = (( char* )set->m_array)[byteindex];
      }
      byteindex--;
      bool         found    = false;
      unsigned int bitindex = 0;
      for (bitindex = 0; !found; bitindex++) {
        found = !GetBitValue(set, (byteindex * 8) + bitindex);
      }
      bitindex--;
      return (byteindex * 8) + bitindex;
    }
  }

  static void ClearBitset(struct tlBitset* set, unsigned char setTrue) {
    memset(set->m_array, setTrue, set->m_byteLength);
  }

  static void ExpandBitset(struct tlBitset* set, unsigned int expandSize) {
    bool* new_block = new bool[expandSize + set->m_byteLength];
    if (new_block) {
      // This is a little bit redundant because all memory initialized with 0 at start
      // And when a memory block is deleted, all bytes are 0
      // But that doesn't hurt too much I think
      memset(new_block, 0, expandSize + set->m_byteLength);

      memcpy(new_block, set->m_array, set->m_byteLength);
      set->m_byteLength += expandSize;
      delete[] set->m_array;
      set->m_array = new_block;
    } else {
      std::cout << "Bitset expand has failed because Allocator has failed!\n";
      int x;
      std::cin >> x;
    }
  }
};

/////////////////////////////////////////////// Unit Tests
#include <vector>
#define TAPI_UNITTEST_CLASSFLAG_BITSET 1

const tlBitsetSys* tapi_getBitsetSystem(struct tlECS* ecsSys) {
  return ( BITSET_TAPI_PLUGIN_LOAD_TYPE )ecsSys->getSystem(BITSET_TAPI_PLUGIN_NAME);
}

uint32_t findFirst(std::vector<bool>& stdBitset, bool isTrue) {
  for (uint32_t i = 0; i < stdBitset.size(); i++) {
    if (stdBitset[i] == isTrue) {
      return i;
    }
  }
  return UINT32_MAX;
}

tlUnitTestFnc(tapi_unitTest_bitset0, data, outputStr) {
  const tlBitsetSys*        bitsetSys        = tapi_getBitsetSystem(( struct tlECS* )data);
  static constexpr uint32_t bitsetByteLength = 10 << 10;
  std::vector<bool>         stdBitset(bitsetByteLength * 8, false);
  struct tlBitset*          tBitset = bitsetSys->createBitset(bitsetByteLength);

  time_t t;
  srand(( unsigned )time(&t));
  for (uint32_t i = 0; i < bitsetByteLength * 8; i++) {
    uint32_t bit   = rand() % (bitsetByteLength * 8);
    bool     v     = rand() % 2;
    stdBitset[bit] = v;
    bitsetSys->setBit(tBitset, bit, v);
  }
  if (findFirst(stdBitset, true) != bitsetSys->getFirstBitIndx(tBitset, true) ||
      findFirst(stdBitset, false) != bitsetSys->getFirstBitIndx(tBitset, false)) {
    *outputStr = L"Firsts're not matching!";
    return 0;
  }
  for (uint32_t i = 0; i < bitsetByteLength * 8; i++) {
    unsigned char tapiV = bitsetSys->getBitValue(tBitset, i);
    unsigned char stdV  = stdBitset[i];
    if (stdV != tapiV) {
      *outputStr = L"Some elements're not matching!";
      return 0;
    }
  }
  *outputStr = L"Succeded";
  return 1;
}

void tapi_setUnitTests_bitset(struct tlECS* ecsSYS) {
  UNITTEST_TAPI_PLUGIN_LOAD_TYPE utSysLoaded =
    ( UNITTEST_TAPI_PLUGIN_LOAD_TYPE )ecsSYS->getSystem(UNITTEST_TAPI_PLUGIN_NAME);
  // If unit test system is available, add unit tests to it
  if (utSysLoaded) {
    auto utSys = utSysLoaded;

    tlUnitTestDescription u;
    u.test = tapi_unitTest_bitset0;
    u.data = ecsSYS;
    utSys->add(L"Bitset0", TAPI_UNITTEST_CLASSFLAG_BITSET, u);
    /*unittest_interface_tapi x;
    x.test = &ut_0;
    ut_sys->funcs->add_unittest("array_of_strings", 0, x);*/
  }
}

TCResult TCBitset_Initialize(const void** outBitsetAPI) {
  auto services             = new TCBitsetServices;
  services->CreateBitset    = TCBitsetContext::CreateBitset;
  services->DestroyBitset   = TCBitsetContext::DestroyBitset;
  services->SetBit          = TCBitsetContext::SetBit;
  services->GetBitValue     = TCBitsetContext::GetBitValue;
  services->GetByteLength   = TCBitsetContext::GetByteLength;
  services->GetFirstBitIndx = TCBitsetContext::GetFirstBitIndx;
  services->ClearBitset     = TCBitsetContext::ClearBitset;
  services->Expand          = TCBitsetContext::ExpandBitset;

  TCBitset      = services;
  *outBitsetAPI = TCBitset;

  return TC_RESULT_SUCCESS;
}

void TCBitset_OnPreShutdown(const TCPluginInfo* pluginInfo, bool isLoaded) {}

TCResult TCBitset_OnPreShutdown() { return TC_RESULT_SUCCESS; }

TCResult TCBitset_Shutdown() { return TC_RESULT_SUCCESS; }