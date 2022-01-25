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
struct renderer_funcs;
struct renderer_public {
private:
	friend struct framegraphsys_vk;
	friend struct renderer_funcs;
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


//RENDER NODEs
struct VK_Pass {
	struct WaitDescription {
		VK_Pass** WaitedPass = nullptr;
		bool WaitLastFramesPass = false;
		bool isValid() const { printer(result_tgfx_NOTCODED, "PassDescription::isValid isn't coded"); }
	};
	enum class PassType : unsigned char {
		INVALID = 0,
		DP = 1,
		TP = 2,
		CP = 3,
		WP = 4,
	};
	//Name is to debug the rendergraph algorithms, production ready code won't use it!
	std::string NAME;
	WaitDescription* const WAITs;
	const unsigned char WAITsCOUNT;
	PassType TYPE;
	//This is to store which branch this pass is used in last frames
	//With that way, we can identify which semaphore we should wait on if rendergraph is reconstructed
	//This is UINT32_MAX if pass isn't used last frame
	unsigned int LastUsedBranchID = UINT32_MAX;


	VK_Pass(const std::string& name, PassType type, unsigned int waitsCount) : WAITs(new WaitDescription[waitsCount]), WAITsCOUNT(waitsCount), TYPE(type), NAME(name) {}
};

template<class T>
bool isWorkloaded(T* pass) {
	return pass->isWorkloaded();
}

//DRAW PASS

struct nonindexeddrawcall_vk {
	VkBuffer VBuffer;
	VkDeviceSize VOffset = 0;
	uint32_t FirstVertex = 0, VertexCount = 0, FirstInstance = 0, InstanceCount = 0;

	VkPipeline MatTypeObj;
	VkPipelineLayout MatTypeLayout;
	VkDescriptorSet* GeneralSet, * PerInstanceSet;
};
struct indexeddrawcall_vk {
	VkBuffer VBuffer, IBuffer;
	VkDeviceSize VBOffset = 0, IBOffset = 0;
	uint32_t VOffset = 0, FirstIndex = 0, IndexCount = 0, FirstInstance = 0, InstanceCount = 0;
	VkIndexType IType;

	VkPipeline MatTypeObj;
	VkPipelineLayout MatTypeLayout;
	VkDescriptorSet* GeneralSet, * PerInstanceSet;
};
struct subdrawpass_vk {
	unsigned char Binding_Index;
	bool render_dearIMGUI = false;
	irtslotset_vk* SLOTSET;
	drawpass_vk* DrawPass;
	threadlocal_vector<nonindexeddrawcall_vk> NonIndexedDrawCalls;
	threadlocal_vector<indexeddrawcall_vk> IndexedDrawCalls;
	subdrawpass_vk() : IndexedDrawCalls(1024), NonIndexedDrawCalls(1024){}
	bool isThereWorkload();
};
struct drawpass_vk {
	VK_Pass base_data;
	VkRenderPass RenderPassObject;
	rtslotset_vk* SLOTSET;
	std::atomic<unsigned char> SlotSetChanged = false;
	unsigned char Subpass_Count;
	subdrawpass_vk* Subpasses;
	VkFramebuffer FBs[2]{ VK_NULL_HANDLE };
	boxregion_tgfx RenderRegion;

	drawpass_vk(const std::string& name, unsigned int waitsCount) : base_data(name, VK_Pass::PassType::DP, waitsCount){}
	bool isWorkloaded();
};

//TRANSFER PASS

struct VK_BUFtoIMinfo {
	VkImage TargetImage;
	VkBuffer SourceBuffer;
	VkBufferImageCopy BufferImageCopy;
};
struct VK_BUFtoBUFinfo {
	VkBuffer SourceBuffer, DistanceBuffer;
	VkBufferCopy info;
};
struct VK_IMtoIMinfo {
	VkImage SourceTexture, TargetTexture;
	VkImageCopy info;
};
struct VK_TPCopyDatas {
	threadlocal_vector<VK_BUFtoIMinfo> BUFIMCopies;
	threadlocal_vector<VK_BUFtoBUFinfo> BUFBUFCopies;
	threadlocal_vector<VK_IMtoIMinfo> IMIMCopies;
	VK_TPCopyDatas();
};

struct VK_ImBarrierInfo {
	VkImageMemoryBarrier Barrier;
};
struct VK_BufBarrierInfo {
	VkBufferMemoryBarrier Barrier;
};
struct VK_TPBarrierDatas {
	threadlocal_vector<VK_ImBarrierInfo> TextureBarriers;
	threadlocal_vector<VK_BufBarrierInfo> BufferBarriers;
	VK_TPBarrierDatas() :TextureBarriers(1024) , BufferBarriers(1024) {}
	VK_TPBarrierDatas(const VK_TPBarrierDatas& copyfrom) :TextureBarriers(copyfrom.TextureBarriers), BufferBarriers(copyfrom.BufferBarriers) {}
};

struct transferpass_vk {
	VK_Pass base_data;
	void* TransferDatas;
	transferpasstype_tgfx TYPE;

	transferpass_vk(const char* name, unsigned int WAITSCOUNT) : base_data(name, VK_Pass::PassType::TP, WAITSCOUNT) {}
	bool isWorkloaded();
};

struct windowcall_vk {
	window_vk* Window;
};
struct windowpass_vk {
	VK_Pass base_data;
	//Element 0 is the Penultimate, Element 1 is the Last, Element 2 is the Current buffers.
	std::vector<windowcall_vk> WindowCalls[3];

	windowpass_vk(const char* name, unsigned int waitsCount) : base_data(name, VK_Pass::PassType::WP, waitsCount){}
	bool isWorkloaded();
};
struct dispatchcall_vk {
	VkPipelineLayout Layout = VK_NULL_HANDLE;
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkDescriptorSet* GeneralSet = nullptr, * InstanceSet = nullptr;
	glm::uvec3 DispatchSize;
};

struct subcomputepass_vk {
	VK_TPBarrierDatas Barriers_AfterSubpassExecutions;
	threadlocal_vector<dispatchcall_vk> Dispatches;
	subcomputepass_vk();
	bool isThereWorkload();
};

struct computepass_vk {
	VK_Pass base_data;
	std::vector<subcomputepass_vk> Subpasses;
	std::atomic_bool SubPassList_Updated = false;

	computepass_vk(const std::string& name, unsigned int WAITSCOUNT);
	bool isWorkloaded();
};