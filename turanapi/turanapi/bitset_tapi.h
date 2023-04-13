#pragma once
#include "predefinitions_tapi.h"
#define BITSET_TAPI_PLUGIN_NAME "tapi_bitset"
#define BITSET_TAPI_PLUGIN_LOAD_TYPE bitsetsys_tapi_type*
#define BITSET_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)

typedef struct tapi_bitset* bitset_tapi;

typedef struct bitsetsys_tapi {
  bitset_tapi (*createBitset)(unsigned int byte_length);
  void (*destroyBitset)(bitset_tapi bitset);
  // If setTrue isn't 0, sets bit as true
  // Otherwise, sets false
  // @return 1 = value was opposite. 0 = value was same
  // TODO: When multi-threading is supported; index = UINT32_MAX'll mean first "!setTrue" bit will
  //  be "setTrue" and index of the bit will be returned
  void (*setBit)(bitset_tapi set, unsigned int index, unsigned char setTrue);
  unsigned char (*getBitValue)(const bitset_tapi set, unsigned int index);
  unsigned int (*getByteLength)(const bitset_tapi set);
  // If findTrue isn't 0, looks for first true bit
  // Otherwise, looks for first false bit.
  unsigned int (*getFirstBitIndx)(const bitset_tapi set, unsigned char findTrue);
  void (*clearBitset)(bitset_tapi set, unsigned char setTrue);
  void (*expand)(bitset_tapi set, unsigned int newSize);
} bitsetsys_tapi;

typedef struct bitsetsys_tapi_d bitsetsys_tapi_d;
typedef struct bitsetsys_tapi_type {
  bitsetsys_tapi_d* data;
  bitsetsys_tapi*   funcs;
} bitsetsys_tapi_type;