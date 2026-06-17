#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCBitset, "tcBitsetSystem", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))
TCORE_DEFINE_HANDLE(TC)

typedef struct TCBitsetServices {
  struct tlBitset* (*CreateBitset)(unsigned int byte_length);
  void (*DestroyBitset)(struct tlBitset* bitset);
  // If setTrue isn't 0, sets bit as true
  // Otherwise, sets false
  // @return 1 = value was opposite. 0 = value was same
  // TODO: When multi-threading is supported; index = UINT32_MAX'll mean first "!setTrue" bit will
  //  be "setTrue" and index of the bit will be returned
  void (*SetBit)(struct tlBitset* set, unsigned int index, unsigned char setTrue);
  unsigned char (*GetBitValue)(const struct tlBitset* set, unsigned int index);
  unsigned int (*GetByteLength)(const struct tlBitset* set);
  // If findTrue isn't 0, looks for first true bit
  // Otherwise, looks for first false bit.
  unsigned int (*GetFirstBitIndx)(const struct tlBitset* set, unsigned char findTrue);
  void (*ClearBitset)(struct tlBitset* set, unsigned char setTrue);
  void (*Expand)(struct tlBitset* set, unsigned int newSize);
} TCBitsetServices;

TCORE_END_C_LINKAGE