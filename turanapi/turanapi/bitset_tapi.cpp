#include "bitset_tapi.h"
#include <atomic>
#include <iostream>

#include "ecs_tapi.h"

typedef struct tapi_bitset {
  bool*        m_array;
  unsigned int m_byteLength;
} bitset_tapi;

bitset_tapi* tapi_createBitset(unsigned int byteLength) {
  bitset_tapi* bitset = ( bitset_tapi* )malloc(sizeof(bitset_tapi) + (sizeof(bool) * byteLength));
  bitset->m_array     = ( bool* )(bitset + 1);
  memset(bitset->m_array, 0, byteLength);
  bitset->m_byteLength = byteLength;
  return ( bitset_tapi* )bitset;
}
void tapi_destroyBitset(bitset_tapi* hnd) {
  bitset_tapi* bitset = ( bitset_tapi* )hnd;
  free(bitset);
}
void tapi_setBit(bitset_tapi* set, unsigned int index, unsigned char setTrue) {
  if (index / 8 > set->m_byteLength - 1) {
    std::cout << "There is no such bit, maximum bit index: " << (set->m_byteLength * 8) - 1
              << std::endl;
    return;
  }
  char&        byte     = (( char* )set->m_array)[index / 8];
  unsigned int bitindex = (index % 8);
  if (setTrue) {
    byte                  = (byte | char(1 << bitindex));
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
unsigned char tapi_getBitValue(const bitset_tapi* set, unsigned int index) {
  unsigned char byte     = ((unsigned char* )set->m_array)[index / 8];
  unsigned char bitindex = (index % 8);
  if (byte & (1 << bitindex)) {
    return true;
  }
  return false;
}
unsigned int tapi_getByteLength(const bitset_tapi* set) { return set->m_byteLength; }
unsigned int tapi_getFirstBitIndx(const bitset_tapi* set, unsigned char findTrue) {
  if (findTrue) {
    unsigned int byteindex = 0;
    for (unsigned int byte_value = 0; byte_value == 0; byteindex++) {
      byte_value = (( char* )set->m_array)[byteindex];
    }
    byteindex--;
    bool         found    = false;
    unsigned int bitindex = 0;
    for (bitindex = 0; !found; bitindex++) {
      found = tapi_getBitValue(set, (byteindex * 8) + bitindex);
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
      found = !tapi_getBitValue(set, (byteindex * 8) + bitindex);
    }
    bitindex--;
    return (byteindex * 8) + bitindex;
  }
}
void tapi_clearBitset(bitset_tapi* set, unsigned char setTrue) {
  memset(set->m_array, setTrue, set->m_byteLength);
}
void tapi_expandBitset(bitset_tapi* set, unsigned int expandSize) {
  bool* new_block = new bool[expandSize + set->m_byteLength];
  if (new_block) {
    // This is a little bit redundant because all memory initialized with 0 at start
    // And when a memory block is deleted, all bytes are 0
    // But that doesn't hurt too much I think
    memset(new_block, 0, expandSize + set->m_byteLength);

    memcpy(new_block, set->m_array, set->m_byteLength);
    set->m_byteLength += expandSize;
    delete set->m_array;
    set->m_array = new_block;
  } else {
    std::cout << "Bitset expand has failed because Allocator has failed!\n";
    int x;
    std::cin >> x;
  }
}

typedef struct bitsetsys_tapi_d {
  bitsetsys_tapi_type* type;
} bitsetsys_tapi_d;
ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  bitsetsys_tapi_type* type = ( bitsetsys_tapi_type* )malloc(sizeof(bitsetsys_tapi_type));
  type->data                = ( bitsetsys_tapi_d* )malloc(sizeof(bitsetsys_tapi_d));
  type->funcs               = ( bitsetsys_tapi* )malloc(sizeof(bitsetsys_tapi));
  type->data->type          = type;

  ecssys->addSystem(BITSET_TAPI_PLUGIN_NAME, BITSET_TAPI_PLUGIN_VERSION, type);

  type->funcs->createBitset    = &tapi_createBitset;
  type->funcs->destroyBitset   = &tapi_destroyBitset;
  type->funcs->setBit          = &tapi_setBit;
  type->funcs->getBitValue     = &tapi_getBitValue;
  type->funcs->getByteLength   = &tapi_getByteLength;
  type->funcs->getFirstBitIndx = &tapi_getFirstBitIndx;
  type->funcs->clearBitset     = &tapi_clearBitset;
  type->funcs->expand          = &tapi_expandBitset;
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) {}