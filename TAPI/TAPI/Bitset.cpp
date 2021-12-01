#include "Bitset.h"
#include <stdio.h>
#include <iostream>
#include <string.h>


#define GetBitset(bitset) ((BITSET*)bitset)

struct BITSET {
	bool* Array;
	unsigned int ByteLength;
};

tapi_bitset tapi_CreateBitset(unsigned int byte_length){
	BITSET* bitset = new BITSET;
	bitset->Array = new bool[byte_length];
	bitset->ByteLength = byte_length;
	return (tapi_bitset)bitset;
}
void tapi_SetBit_True(tapi_bitset set, unsigned int index) {
	if (index / 8 > GetBitset(set)->ByteLength - 1) {
		std::cout << "There is no such bit, maximum bit index: " << (GetBitset(set)->ByteLength * 8) - 1 << std::endl;
		return;
	}
	char& byte = ((char*)GetBitset(set)->Array)[index / 8];
	unsigned int bitindex = (index % 8);
	byte = (byte | char(1 << bitindex));
}
void tapi_SetBit_False(tapi_bitset set, unsigned int index) {
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
unsigned char tapi_GetBit_Value(tapi_bitset set, unsigned int index) {
	char byte = ((char*)GetBitset(set)->Array)[index / 8];
	char bitindex = (index % 8);
	return byte & (1 << bitindex);
}
unsigned int tapi_GetByte_Length(tapi_bitset set) {
	return GetBitset(set)->ByteLength;
}
unsigned int tapi_GetIndex_FirstTrue(tapi_bitset set) {
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
unsigned int tapi_GetIndex_FirstFalse(tapi_bitset set) {
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
void tapi_Clear(tapi_bitset set, unsigned char zero_or_one) {
	memset(GetBitset(set)->Array, zero_or_one, GetBitset(set)->ByteLength);
}
void tapi_Expand(tapi_bitset set, unsigned int expand_size) {
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


