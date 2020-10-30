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
		bool WaitLastFramesPass;	//True means this barrier waits the last frame's pass execution instead of this frame's one
	};


	/*
	1) This class will be used to store necessary informations to draw triangles
	2) A Draw Call can only be assigned to one SubDrawPass
	*/
	struct GFXAPI DrawCall_Description {
		GFXHandle MeshBuffer_ID, ShaderInstance_ID, SubDrawPass_ID;
	};
	//If you're referencing the Swapchain, you should use IS_SWAPCHAIN
	//If IS_SWAPCHAIN is not NO_SWPCHN, then RT_Handle is ignored because GFX API doesn't give Swapchain handle to the user
	struct GFXAPI RTSLOT_Description {
		GFXHandle RT_Handle;
		OPERATION_TYPE OPTYPE;
		DRAWPASS_LOAD LOADOP;
		vec4 CLEAR_VALUE;
		SWAPCHAIN_IDENTIFIER IS_SWAPCHAIN;
		unsigned char SLOTINDEX;
	};
	//If IS_DEPTH is true, SLOTINDEX is ignored because there can be only one depth buffer at a time
	//If OPTYPE of the base slot is Read Only, you can only set OPTYPE in this description as Read Only or UNUSED
	struct GFXAPI IRTSLOT_Description {
		unsigned char SLOTINDEX;
		bool IS_DEPTH;
		OPERATION_TYPE OPTYPE;
	};
	//Don't forget to inherite a SlotSet with GFXRenderer->Inherite_RTSlotSet() and pass the handle to BASESLOTSET
	//If you set BASESLOTSET as a Base Slot Set, it is Undefined Behaviour!
	struct GFXAPI SubDrawPass_Description {
	public:
		GFXHandle INHERITEDSLOTSET;
		unsigned char SubDrawPass_Index;
		vector<PassWait_Description> WaitDescriptions;
	};

	/*
	* There are 4 types of TransferPasses and each type only support its type of GFXContentManager command: Barrier, Copy, Upload, Download
	* Barrier means passes waits for the given resources (also image usage changes may happen that may change layout of the image)
	And Barrier commands are also useful for creating resources that don't depend on CPU side data and GPU operations creates the data
	That means you should use Barrier TP to create RTs that don't have data before doing any GPU operations (G-Buffer RTs for instance)
	* Upload cmd uploads resource from CPU to GPU
	* Download cmd downloads resource from GPU to CPU
	* Copy cmd copies resources between each other on GPU

	Upload TP:
	1) If your texture data comes from the CPU, you should create the texture in this pass

	Download TP:
	1) Don't forget that you should carefully set IMUSAGEs of the Download cmds!
	2) If you're gonna delete the texture (Save the texture to CPU and delete), you can set LATER_IMUSAGE same with PREVIOUS_IMUSAGE so no barrier will be created

	Copy TP:
	1) Don't forget that you should carefully set IMUSAGEs of the Copy cmds!

	Note: You may ask why Self-Copy cmd isn't supported by all type of TPs, because that makes user be able to specify a image usage transition while a copy operation to it
	already specifed means that there may be a usage conflict and automatic solvation of this conflict needs resource state tracking which isn't the purpose of the GFX API!
	I know it can still happen if you create a Self-Copy TP and an Upload or Copy TP but this lets API user to easily debug this type of issues!
	*/
	struct GFXAPI TransferPass_Description {
		vector<PassWait_Description> WaitDescriptions;
		TRANFERPASS_TYPE TP_TYPE;
		string NAME;
	};
}