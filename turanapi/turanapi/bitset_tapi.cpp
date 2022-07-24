#include "bitset_tapi.h"
#include "ecs_tapi.h"
#include <iostream>


#define GetBitset(bitset) ((BITSET*)bitset)

struct BITSET {
	bool* Array;
	unsigned int ByteLength;
};

bitset_tapi* CreateBitset(unsigned int byte_length){
	BITSET* bitset = new BITSET;
	bitset->Array = new bool[byte_length];
    memset(bitset->Array, 0, byte_length);
	bitset->ByteLength = byte_length;
	return (bitset_tapi*)bitset;
}
void SetBit_True(bitset_tapi* set, unsigned int index) {
	if (index / 8 > GetBitset(set)->ByteLength - 1) {
		std::cout << "There is no such bit, maximum bit index: " << (GetBitset(set)->ByteLength * 8) - 1 << std::endl;
		return;
	}
	char& byte = ((char*)GetBitset(set)->Array)[index / 8];
	unsigned int bitindex = (index % 8);
	byte = (byte | char(1 << bitindex));
}
void SetBit_False(bitset_tapi* set, unsigned int index) {
	char& byte = ((char*)GetBitset(set)->Array)[index / 8];
	char bitindex = (index % 8);
	switch (bitindex) {
	case 0:
		byte = (byte & 254);
		break;
	case 1:
		byte = (byte & 253);
		break;
	case 2:
		byte = (byte & 251);
		break;
	case 3:
		byte = (byte & 247);
		break;
	case 4:
		byte = (byte & 239);
		break;
	case 5:
		byte = (byte & 223);
		break;
	case 6:
		byte = (byte & 191);
		break;
	case 7:
		byte = (byte & 127);
		break;
	}
}
unsigned char tapi_GetBit_Value(const bitset_tapi* set, unsigned int index) {
	char byte = ((char*)GetBitset(set)->Array)[index / 8];
	char bitindex = (index % 8);
	return byte & (1 << bitindex);
}
unsigned int tapi_GetByte_Length(const bitset_tapi* set) {
	return GetBitset(set)->ByteLength;
}
unsigned int tapi_GetIndex_FirstTrue(const bitset_tapi* set) {
	unsigned int byteindex = 0;
	for (unsigned int byte_value = 0; byte_value == 0; byteindex++) {
		byte_value = ((char*)GetBitset(set)->Array)[byteindex];
	}
	byteindex--;
	bool found = false;
	unsigned int bitindex = 0;
	for (bitindex = 0; !found; bitindex++) {
		found = tapi_GetBit_Value(set, (byteindex * 8) + bitindex);
	}
	bitindex--;
	return (byteindex * 8) + bitindex;
}
unsigned int tapi_GetIndex_FirstFalse(const bitset_tapi* set) {
	unsigned int byteindex = 0;
	for (unsigned int byte_value = 255; byte_value == 255; byteindex++) {
		byte_value = ((char*)GetBitset(set)->Array)[byteindex];
	}
	byteindex--;
	bool found = false;
	unsigned int bitindex = 0;
	for (bitindex = 0; !found; bitindex++) {
		found = !tapi_GetBit_Value(set, (byteindex * 8) + bitindex);
	}
	bitindex--;
	return (byteindex * 8) + bitindex;
}
void ClearBitset(bitset_tapi* set, unsigned char zero_or_one) {
	memset(GetBitset(set)->Array, zero_or_one, GetBitset(set)->ByteLength);
}
void ExpandBitset(bitset_tapi* set, unsigned int expand_size) {
	bool* new_block = new bool[expand_size + GetBitset(set)->ByteLength];
	if (new_block) {
		//This is a little bit redundant because all memory initialized with 0 at start
		//And when a memory block is deleted, all bytes are 0
		//But that doesn't hurt too much I think
		memset(new_block, 0, expand_size + GetBitset(set)->ByteLength);

		memcpy(new_block, GetBitset(set)->Array, GetBitset(set)->ByteLength);
		GetBitset(set)->ByteLength += expand_size;
		delete GetBitset(set)->Array;
		GetBitset(set)->Array = new_block;
	}
	else {
		std::cout << "Bitset expand has failed because Allocator has failed!\n";
		int x;
		std::cin >> x;
	}
}




typedef struct bitsetsys_tapi_d{
    bitsetsys_tapi_type* type;
}bitsetsys_tapi_d;
ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
    bitsetsys_tapi_type* type = (bitsetsys_tapi_type*)malloc(sizeof(bitsetsys_tapi_type));
    type->data = (bitsetsys_tapi_d*)malloc(sizeof(bitsetsys_tapi_d));
    type->funcs = (bitsetsys_tapi*)malloc(sizeof(bitsetsys_tapi));
    type->data->type = type;

    ecssys->addSystem(BITSET_TAPI_PLUGIN_NAME, BITSET_TAPI_PLUGIN_VERSION, type);

    type->funcs->clear_bitset = &ClearBitset;
    type->funcs->create_bitset = &CreateBitset;
    type->funcs->expand = &ExpandBitset;
    type->funcs->getbit_value = &tapi_GetBit_Value;
    type->funcs->getbyte_length = &tapi_GetByte_Length;
    type->funcs->getindex_firstfalse = &tapi_GetIndex_FirstFalse;
    type->funcs->getindex_firsttrue = &tapi_GetIndex_FirstTrue;
    type->funcs->setbit_false = &SetBit_False;
    type->funcs->setbit_true = &SetBit_True;
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) {
    
}