#pragma once
#include "predefinitions_tapi.h"
#define BITSET_TAPI_PLUGIN_NAME "tapi_bitsetsys"
#define BITSET_TAPI_PLUGIN_LOAD_TYPE bitsetsys_tapi_type*
#define BITSET_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)

typedef struct bitset_tapi bitset_tapi;

typedef struct bitsetsys_tapi {
  bitset_tapi* (*create_bitset)(unsigned int byte_length);
  void (*setbit_true)(bitset_tapi* set, unsigned int index);
  void (*setbit_false)(bitset_tapi* set, unsigned int index);
  unsigned char (*getbit_value)(const bitset_tapi* set, unsigned int index);
  unsigned int (*getbyte_length)(const bitset_tapi* set);
  unsigned int (*getindex_firstfalse)(const bitset_tapi* set);
  unsigned int (*getindex_firsttrue)(const bitset_tapi* set);
  void (*clear_bitset)(bitset_tapi* set, unsigned char zero_or_one);
  void (*expand)(bitset_tapi* set, unsigned int new_size);
} bitsetsys_tapi;

typedef struct bitsetsys_tapi_d bitsetsys_tapi_d;
typedef struct bitsetsys_tapi_type {
  bitsetsys_tapi_d* data;
  bitsetsys_tapi*   funcs;
} bitsetsys_tapi_type;