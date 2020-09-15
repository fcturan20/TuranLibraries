#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX_Resource.h"
#include "GFX_RenderCommands.h"

/*
	SubDrawPass, DrawPass and ComputePass are all RenderNodes
	So we can't say any general rules about RenderNodes, each one of them is unique


*/

namespace GFX_API {

	struct GFXAPI SubDrawPass {
	public:
		unsigned int ID;
		unsigned char Binding_Index;
		RT_SLOTSET* SLOTSET;
		vector<DrawCall> DrawCalls;
	};

	struct GFXAPI SubDrawPass_Description {
	public:
		unsigned int SLOTSET_ID;
		unsigned char SubDrawPass_Index;
		vector<unsigned int> Wait_TransferPackets;
	};

	struct GFXAPI DrawPass {
	public:
		unsigned int ID;
		void* GL_ID;
		RT_SLOTSET* SLOTSET;
		unsigned char Subpass_Count;
		GFX_API::SubDrawPass* Subpasses;
	};

	/*
	struct GFXAPI ComputePass {
	protected:
		vector<ComputeShader_Instance*> ComputeShaders;
	public:
		virtual void Execute() = 0;
	};*/





}
