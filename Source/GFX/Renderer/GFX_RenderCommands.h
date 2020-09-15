#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_ENUMs.h"
#include "GFX_Resource.h"

namespace GFX_API {
	/*
	1) This class will be used to store necessary informations to draw triangles
	2) A Draw Call can only be assigned to one SubDrawPass
	*/
	struct GFXAPI DrawCall {
		unsigned int MeshBuffer_ID;
		unsigned int ShaderInstance_ID;
		unsigned int SubDrawPassID;
	};

	/*
	Definitions of Points and Lines Rendering should be in the related Draw Pass
	That means, you aren't allowed to pass shader instance and you can't specify colors or sizes of lines and points outside of shader
	I want to support Point and Line rendering for debugging purposes, nothing more
	*/
	struct GFXAPI PointLineDrawCall {
		bool Draw_asPoint;	//True -> Draw as Point, False -> Draw as Line
		unsigned int PointBuffer_ID;
	};
}