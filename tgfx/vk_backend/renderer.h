/* There are 5 different branches for Renderer:
1) RG Description: Describes render passes then checks if RG is valid. 
    If valid, creates Vulkan objects for the passes (RenderPass, Framebuffer etc.)
    Implemented in RG_Description.cpp
2) RG Static Linking: Creates optimized data structures for dynamic framegraph optimizations.
    Implemented in RG_StaticLinkage.cpp
3) RG Commander: Implements render commands API (Draws, Dispatches, Transfer calls etc.)
    Implemented in RG_Commander.cpp
4) RG Dynamic Linking: Optimizes RG for minimal queue and sync operations and submits recorded CBs.
    Implemented in RG_DynamicLinkage.cpp
5) RG CommandBuffer Recorder: Records command buffers according to the commands from RG_DynamicLinkage.cpp
    Implemented in RG_CBRecording.cpp
//For the sake of simplicity; RG_CBRecording, RG_StaticLinkage and RG_DynamicLinkage is implemented in RG_primitive.cpp for now
//But all of these branches are communicating through RG.h header, so don't include the header in other systems
*/


#pragma once
#include "includes.h"
#include <vector>
#include <tgfx_forwarddeclarations.h>
#include <tgfx_structs.h>
#include <atomic>

enum class RGReconstructionStatus : unsigned char {
	Invalid = 0,
	StartedConstruction = 1,
	FinishConstructionCalled = 2,
	HalfConstructed = 3,
	Valid = 4
};

//Renderer data that other parts of the backend can access
struct renderer_funcs;
struct renderer_public {
public:
	inline unsigned char Get_FrameIndex(bool is_LastFrame){
		return (is_LastFrame) ? ((VKGLOBAL_FRAMEINDEX - 1) % VKCONST_BUFFERING_IN_FLIGHT) : (VKGLOBAL_FRAMEINDEX);
	}
	void RendererResource_Finalizations();
	RGReconstructionStatus RGSTATUS();
};


//RENDER NODEs
enum class VK_PASSTYPE : unsigned char {
	INVALID = 0,
	DP = 1,
	TP,
	CP,
	WP,
	//for subpasses
	SUBTP_BARRIER,
	SUBTP_COPY
};
typedef uint16_t VK_PASS_ID_TYPE;
static constexpr VK_PASS_ID_TYPE VK_PASS_INVALID_ID = UINT16_MAX;