#pragma once
#include <TCoreMacros.h>

TCORE_BEGIN_C_LINKAGE

typedef struct TGfxUVec4
{
	unsigned int x, y, z, w;
} TGfxUVec4;

typedef struct TGfxUVec3
{
	unsigned int x, y, z;
} TGfxUVec3;

typedef struct TGfxUVec2
{
	unsigned int x, y;
} TGfxUVec2;

typedef struct TGfxFVec2
{
	float x, y;
} TGfxFVec2;

typedef struct TGfxFVec3
{
	float x, y, z;
} TGfxFVec3;

typedef struct TGfxFVec4
{
	float x, y, z, w;
} TGfxFVec4;

typedef struct TGfxIVec2
{
	int x, y;
} TGfxIVec2;

typedef struct TGfxIVec3
{
	int x, y, z;
} TGfxIVec3;

typedef struct TGfxIVec4
{
	int x, y, z, w;
} TGfxIVec4;

typedef struct TGfxBoxRegion
{
	unsigned int XOffset, YOffset, Width, Height;
} TGfxBoxRegion;

typedef struct TGfxCubeRegion
{
	unsigned int XOffset, YOffset, ZOffset, Width, Height, Depth;
} TGfxCubeRegion;

TCORE_END_C_LINKAGE