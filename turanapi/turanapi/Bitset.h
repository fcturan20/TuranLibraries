#pragma once
#include "API_includes.h"



#ifdef __cplusplus
extern "C" {
#endif
	TAPIHANDLE(bitset)
	tapi_bitset tapi_CreateBitset(unsigned int byte_length);
	void tapi_SetBit_True(tapi_bitset set, unsigned int index);
	void tapi_SetBit_False(tapi_bitset set, unsigned int index);
	unsigned char tapi_GetBit_Value(const tapi_bitset set, unsigned int index);
	unsigned int tapi_GetByte_Length(const tapi_bitset* set);
	unsigned int tapi_GetIndex_FirstFalse(const tapi_bitset* set);
	unsigned int tapi_GetIndex_FirstTrue(const tapi_bitset* set);
	void tapi_Clear(tapi_bitset* set, unsigned char zero_or_one);
	void tapi_Expand(tapi_bitset* set, unsigned int expand_size);


#ifdef __cplusplus
}
#endif

