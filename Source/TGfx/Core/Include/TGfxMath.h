#pragma once
#include <TCoreMacros.h>

TCORE_BEGIN_C_LINKAGE
struct TGfxUI4
{
	unsigned int x, y, z, w;
};

struct TGfxUI3
{
	unsigned int x, y, z;
};

struct TGfxUI2
{
	unsigned int x, y;
};

struct TGfxF2
{
	float x, y;
};

struct TGfxF3
{
	float x, y, z;
};

struct TGfxF4
{
	float x, y, z, w;
};

struct TGfxI2
{
	int x, y;
};

struct TGfxI3
{
	int x, y, z;
};

struct TGfxI4
{
	int x, y, z, w;
};

struct TGfxBoxRegion
{
	unsigned int XOffset, YOffset, Width, Height;
};

struct TGfxCubeRegion
{
	unsigned int XOffset, YOffset, ZOffset, Width, Height, Depth;
};

TCORE_END_C_LINKAGE