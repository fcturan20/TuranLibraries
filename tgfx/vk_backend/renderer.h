#pragma once
#include "includes.h"
#include <vector>
#include <tgfx_forwarddeclarations.h>
#include <tgfx_structs.h>

enum class RenderGraphStatus : unsigned char {
	Invalid = 0,
	StartedConstruction = 1,
	FinishConstructionCalled = 2,
	HalfConstructed = 3,
	Valid = 4
};

//Renderer data that other parts of the backend can access
struct renderer_public {
private:
	friend class framegraphsys_vk;
	friend result_tgfx Execute_RenderGraph();
	friend void Start_RenderGraphConstruction();
	friend unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass);
	friend void Destroy_RenderGraph();
	std::vector<drawpass_vk*> DrawPasses;
	std::vector<transferpass_vk*> TransferPasses;
	std::vector<windowpass_vk*> WindowPasses;
	std::vector<computepass_vk*> ComputePasses;
	RenderGraphStatus RG_Status = RenderGraphStatus::Invalid;
	unsigned char FrameIndex;
public:
	inline unsigned char Get_FrameIndex(bool is_LastFrame){
		return (is_LastFrame) ? ((FrameIndex + 1) % 2) : (FrameIndex);
	}
	void RendererResource_Finalizations();
	inline RenderGraphStatus RGSTATUS() { return RG_Status; }
};


struct drawpass_vk {
	VkRenderPass RenderPassObject;
	rtslotset_vk* SLOTSET;
	std::atomic<unsigned char> SlotSetChanged = false;
	unsigned char Subpass_Count;
	subdrawpass_vk* Subpasses;
	VkFramebuffer FBs[2]{ VK_NULL_HANDLE };
	boxregion_tgfx RenderRegion;
};
struct transferpass_vk {

};
struct windowpass_vk {

};
struct computepass_vk {

};
struct subdrawpass_vk {
	unsigned char Binding_Index;
	bool render_dearIMGUI = false;
	irtslotset_vk* SLOTSET;
	drawpass_vk* DrawPass; 
	threadlocal_vector<nonindexeddrawcall_vk> NonIndexedDrawCalls;
	threadlocal_vector<indexeddrawcall_vk> IndexedDrawCalls;
};