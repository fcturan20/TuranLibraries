#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(tsBitset, "tcBitsetSystem", tcBitsetSys, TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

struct tcBitset;
struct tcBitsetSys {
  struct tlBitsetSysPriv* d;
  struct tlBitset* (*createBitset)(unsigned int byte_length);
  void (*destroyBitset)(struct tlBitset* bitset);
  // If setTrue isn't 0, sets bit as true
  // Otherwise, sets false
  // @return 1 = value was opposite. 0 = value was same
  // TODO: When multi-threading is supported; index = UINT32_MAX'll mean first "!setTrue" bit will
  //  be "setTrue" and index of the bit will be returned
  void (*setBit)(struct tlBitset* set, unsigned int index, unsigned char setTrue);
  unsigned char (*getBitValue)(const struct tlBitset* set, unsigned int index);
  unsigned int (*getByteLength)(const struct tlBitset* set);
  // If findTrue isn't 0, looks for first true bit
  // Otherwise, looks for first false bit.
  unsigned int (*getFirstBitIndx)(const struct tlBitset* set, unsigned char findTrue);
  void (*clearBitset)(struct tlBitset* set, unsigned char setTrue);
  void (*expand)(struct tlBitset* set, unsigned int newSize);
};

TCORE_END_C_LINKAGE