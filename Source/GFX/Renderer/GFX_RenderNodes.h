#pragma once
#include "GFX/GFX_Includes.h"

/*
	SubDrawPass, DrawPass and ComputePass are all RenderNodes
	So we can't say any general rules about RenderNodes, each one of them is unique


*/

namespace GFX_API {
	struct GFXAPI PassWait_Description {
		//This is a handle pointer because you may refer to a pass that's not created but will be created before finishing the rendergraph creation
		//You should create a handle with "new GFXHandle" and when you create the referenced pass, you should set this handle as its returned handle as "*HandlePTR = Create_***Pass()"
		GFXHandle* WaitedPass;
		SHADERSTAGEs_FLAG WaitedStage;
		bool WaitLastFramesPass : 1;	//True means this barrier waits the last frame's pass execution instead of this frame's one
		//But it will change if it waits for SWAPCHAINDISPLAY stage, because it means pass is waiting for a Window Pass
		//And a Window Pass is always the end point of a FrameGraph
		//So "WaitLastFramesPass == false" states that you are waiting for 2 frame ago's presentation to finish
	};


	/*
	1) Handles are seperated because some users may want to use different textures for different frames (also swapchain textures is seperated each frame)
	If you want to use same texture across frames, you should specify both handle as the same
	2) OPTYPE defines how you want to use the RT, you can use all of the OPERATION_TYPE except "UNUSED".
	3) LOAPOP defines what do to do with RT before starting the Draw Pass.
	4) CLEAR_VALUE is ignored if LOADOP isn't CLEAR. 
	5) Some hardware implements additional cache for RTs. If you won't use the written data later, it's better to set isUSEDLATER false.
	*/
	struct GFXAPI RTSLOT_Description {
		GFX_API::GFXHandle TextureHandles[2];
		GFX_API::OPERATION_TYPE OPTYPE, OPTYPESTENCIL;
		GFX_API::DRAWPASS_LOAD LOADOP, LOADOPSTENCIL;
		vec4 CLEAR_VALUE; 
		bool isUSEDLATER;
		unsigned char SLOTINDEX;
	};
	//If IS_DEPTH is true, SLOTINDEX is ignored because there can be only one depth buffer at a time
	//If OPTYPE of the base slot is Read Only, you can only set OPTYPE in this description as Read Only or UNUSED
	struct GFXAPI RTSLOTUSAGE_Description {
		unsigned char SLOTINDEX;
		bool IS_DEPTH;
		OPERATION_TYPE OPTYPE, OPTYPESTENCIL;
	};
	//Don't forget to inherite a SlotSet with GFXRenderer->Inherite_RTSlotSet() and pass the handle to BASESLOTSET
	//If you set BASESLOTSET as a Base Slot Set, it is Undefined Behaviour!
	//WaitOp specifies which shader stage are you waiting to finish from the previous Subpass of the same DrawPass
	//ContinueOp specifies which shader stage is gonna continue executing after WaitedStage has finished
	struct GFXAPI SubDrawPass_Description {
	public:
		GFXHandle INHERITEDSLOTSET;
		unsigned char SubDrawPass_Index;
		SUBDRAWPASS_ACCESS WaitOp, ContinueOp;
	};
}