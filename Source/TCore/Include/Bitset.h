#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCBitset, "tcBitsetSystem", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCBitset);

typedef struct ITCBitset
{
	TCBitsetHandle (*CreateBitset)(unsigned int byte_length);
	void (*DestroyBitset)(TCBitsetHandle bitset);
	// If setTrue isn't 0, sets bit as true
	// Otherwise, sets false
	// @return 1 = value was opposite. 0 = value was same
	// TODO: When multi-threading is supported; index = UINT32_MAX'll mean first "!setTrue" bit will
	//  be "setTrue" and index of the bit will be returned
	void (*SetBit)(TCBitsetHandle set, unsigned int index, unsigned char set_true);
	unsigned char (*GetBitValue)(const TCBitsetHandle set, unsigned int index);
	unsigned int (*GetByteLength)(const TCBitsetHandle set);
	// If findTrue isn't 0, looks for first true bit
	// Otherwise, looks for first false bit.
	unsigned int (*GetFirstBitIndx)(const TCBitsetHandle set, unsigned char find_true);
	void (*ClearBitset)(TCBitsetHandle set, unsigned char set_true);
	void (*Expand)(TCBitsetHandle set, unsigned int new_size);
} ITCBitset;

TCORE_END_C_LINKAGE