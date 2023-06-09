#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define BITSET_TAPI_PLUGIN_NAME "tapi_bitset"
#define BITSET_TAPI_PLUGIN_LOAD_TYPE struct tapi_bitsetSys_type*
#define BITSET_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)

struct tapi_bitsetSys {
  struct tapi_bitset* (*createBitset)(unsigned int byte_length);
  void (*destroyBitset)(struct tapi_bitset* bitset);
  // If setTrue isn't 0, sets bit as true
  // Otherwise, sets false
  // @return 1 = value was opposite. 0 = value was same
  // TODO: When multi-threading is supported; index = UINT32_MAX'll mean first "!setTrue" bit will
  //  be "setTrue" and index of the bit will be returned
  void (*setBit)(struct tapi_bitset* set, unsigned int index, unsigned char setTrue);
  unsigned char (*getBitValue)(const struct tapi_bitset* set, unsigned int index);
  unsigned int (*getByteLength)(const struct tapi_bitset* set);
  // If findTrue isn't 0, looks for first true bit
  // Otherwise, looks for first false bit.
  unsigned int (*getFirstBitIndx)(const struct tapi_bitset* set, unsigned char findTrue);
  void (*clearBitset)(struct tapi_bitset* set, unsigned char setTrue);
  void (*expand)(struct tapi_bitset* set, unsigned int newSize);
};

struct tapi_bitsetSys_type {
  struct tapi_bitsetSys_d* data;
  struct tapi_bitsetSys*   funcs;
};

#ifdef __cplusplus
}
#endif