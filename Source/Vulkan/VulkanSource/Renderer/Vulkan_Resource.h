#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"


namespace Vulkan {
	//This structure defines offset of the suballocation in the big memory allocation that happens at initialization
	struct MemoryBlock {
		unsigned int MemAllocIndex;
		VkDeviceSize Offset;
	};
	struct VK_API VK_Semaphore {
		VkSemaphore SPHandle;
		bool isUsed = false;
	};
	

	struct VK_API VK_Texture {
		unsigned int WIDTH, HEIGHT, DATA_SIZE;
		GFX_API::TEXTURE_CHANNELs CHANNELs;
		GFX_API::TEXTUREUSAGEFLAG USAGE;

		VkImage Image = {};
		VkImageView ImageView = {};
		MemoryBlock Block;
	};
	VK_API VkImageUsageFlags Find_VKImageUsage_forVKTexture(VK_Texture& TEXTURE);
	VK_API VkImageUsageFlags Find_VKImageUsage_forGFXTextureDesc(GFX_API::TEXTUREUSAGEFLAG USAGEFLAG, GFX_API::TEXTURE_CHANNELs channels);
	VK_API void Find_AccessPattern_byIMAGEACCESS(const GFX_API::IMAGE_ACCESS& Access, VkAccessFlags& TargetAccessFlag, VkImageLayout& TargetImageLayout);
	VK_API VkImageTiling Find_VkTiling(GFX_API::TEXTURE_ORDER order);
	VK_API VkImageType Find_VkImageType(GFX_API::TEXTURE_DIMENSIONs dimensions);

	struct VK_API VK_COLORRTSLOT {
		VK_Texture* RT;
		GFX_API::DRAWPASS_LOAD LOADSTATE;
		bool IS_USED_LATER;
		GFX_API::OPERATION_TYPE RT_OPERATIONTYPE;
		vec4 CLEAR_COLOR;
		std::atomic_bool IsChanged = false;
	};
	struct VK_API VK_DEPTHSTENCILSLOT {
		VK_Texture* RT;
		GFX_API::DRAWPASS_LOAD DEPTH_LOAD, STENCIL_LOAD;
		bool IS_USED_LATER;
		GFX_API::OPERATION_TYPE DEPTH_OPTYPE, STENCIL_OPTYPE;
		vec2 CLEAR_COLOR;
		std::atomic_bool IsChanged = false;
	};
	struct VK_RTSLOTs {
		VK_COLORRTSLOT* COLOR_SLOTs = nullptr;
		unsigned char COLORSLOTs_COUNT = 0;
		VK_DEPTHSTENCILSLOT* DEPTHSTENCIL_SLOT = nullptr;	//There is one, but there may not be a Depth Slot. So if there is no, then this is nullptr.
		//Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
		atomic_bool IsChanged = false;
	};
	struct VK_API VK_RTSLOTSET {
		VK_RTSLOTs PERFRAME_SLOTSETs[2];
		//You should change this struct's vkRenderPass object pointer as your vkRenderPass object
		VkFramebufferCreateInfo FB_ci[2];
		vector<VkImageView> ImageViews[2];
	};
	struct VK_API VK_IRTSLOTSET {
		VK_RTSLOTSET* BASESLOTSET;
		GFX_API::OPERATION_TYPE* COLOR_OPTYPEs;
		GFX_API::OPERATION_TYPE DEPTH_OPTYPE;
		GFX_API::OPERATION_TYPE STENCIL_OPTYPE;
	};


	/* Vertex Attribute Layout Specification:
			All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
			Vertex Attributes are created as seperate objects because this helps debug visualization of the data
			Vertex Attributes are gonna be ordered by VertexAttributeLayout's vector elements order and this also defines attribute's in-shader location
			That means if position attribute is second element in "Attributes" vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.
	*/
	struct VK_API VK_VertexAttribLayout {
		GFX_API::DATA_TYPE* Attributes;
		unsigned int Attribute_Number, size_perVertex;


		VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
		VkVertexInputAttributeDescription* AttribDescs;
		VkPrimitiveTopology PrimitiveTopology;
		unsigned char AttribDesc_Count;
	};
	struct VK_API VK_VertexBuffer {
		unsigned int VERTEX_COUNT;
		VK_VertexAttribLayout* Layout;
		MemoryBlock Block;
	};
	struct VK_API VK_IndexBuffer {
		VkIndexType DATATYPE;
		VkDeviceSize IndexCount = 0;
		MemoryBlock Block;
	};


	struct VK_API VK_GlobalBuffer {
		VkDeviceSize DATA_SIZE, BINDINGPOINT;
		MemoryBlock Block;
		bool isUniform;
		GFX_API::SHADERSTAGEs_FLAG ACCESSED_STAGEs;
	};

	enum class DescType : unsigned char {
		IMAGE,
		SAMPLER,
		UBUFFER,
		SBUFFER
	};
	struct VK_API VK_DescBufferElement {
		VkDescriptorBufferInfo Info;
		std::atomic<bool> IsUpdated;
		VK_DescBufferElement();
		VK_DescBufferElement(const VK_DescBufferElement& copyDesc);
	};
	struct VK_API VK_DescImageElement {
		VkDescriptorImageInfo info;
		std::atomic_bool IsUpdated;
		VK_DescImageElement();
		VK_DescImageElement(const VK_DescImageElement& copyDesc);
	};
	struct VK_Descriptor {
		DescType Type;
		GFX_API::GFXHandle Elements = nullptr;
		unsigned int ElementCount = 0;
	};
	struct VK_API VK_DescSet {
		VkDescriptorSet Set = VK_NULL_HANDLE;
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
		VK_Descriptor* Descs = nullptr;
		//DescCount is size of the "Descs" pointer above, others are including element counts of each descriptor
		unsigned int DescCount = 0, DescUBuffersCount = 0, DescSBuffersCount = 0, DescSamplersCount = 0, DescImagesCount = 0;
		std::atomic_bool ShouldRecreate = false;
	};
	struct VK_API VK_DescSetUpdateCall {
		VK_DescSet* Set;
		unsigned int BindingIndex, ElementIndex;
	};
	struct VK_API VK_DescPool {
		VkDescriptorPool pool;
		TuranAPI::Threading::AtomicUINT REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
	};





	struct VK_API VK_ShaderSource {
		VkShaderModule Module;
	};
	struct VK_SubDrawPass;
	struct VK_API VK_GraphicsPipeline {
		VK_SubDrawPass* GFX_Subpass;

		VkPipelineLayout PipelineLayout;
		VkPipeline PipelineObject;
		VK_DescSet General_DescSet, Instance_DescSet;
	};
	struct VK_API VK_PipelineInstance {
		VK_GraphicsPipeline* PROGRAM;

		VK_DescSet DescSet;
	};
	struct VK_API VK_Sampler {
		VkSampler Sampler;
	};




	//RenderNodes

	enum class PassType : unsigned char {
		ERROR = 0,
		DP = 1,
		TP = 2,
		CP = 3,
		WP = 4,
	};
	struct VK_ImDownloadInfo {
		VK_Texture* IMAGE;
	};

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
		TuranAPI::Threading::TLVector<VK_BUFtoIMinfo> BUFIMCopies;
		TuranAPI::Threading::TLVector<VK_BUFtoBUFinfo> BUFBUFCopies;
		TuranAPI::Threading::TLVector<VK_IMtoIMinfo> IMIMCopies;
		VK_TPCopyDatas();
	};

	struct VK_ImBarrierInfo {
		VkImageMemoryBarrier Barrier;
	};
	struct VK_BufBarrierInfo {
		VkBufferMemoryBarrier Barrier;
	};
	struct VK_TPBarrierDatas {
		TuranAPI::Threading::TLVector<VK_ImBarrierInfo> TextureBarriers;
		TuranAPI::Threading::TLVector<VK_BufBarrierInfo> BufferBarriers;
		VK_TPBarrierDatas();
	};

	struct VK_TransferPass {
		GFX_API::GFXHandle TransferDatas;
		vector<GFX_API::PassWait_Description> WAITs;
		GFX_API::TRANFERPASS_TYPE TYPE;
		string NAME;
	};

	struct VK_NonIndexedDrawCall {
		VkBuffer VBuffer;
		VkDeviceSize VOffset = 0;
		uint32_t FirstVertex = 0, VertexCount = 0, FirstInstance = 0, InstanceCount = 0;

		VkPipeline MatTypeObj;
		VkPipelineLayout MatTypeLayout;
		VkDescriptorSet *GeneralSet, *PerInstanceSet;
	};
	struct VK_IndexedDrawCall {
		VkBuffer VBuffer, IBuffer;
		VkDeviceSize VBOffset = 0, IBOffset = 0;
		uint32_t VOffset = 0, FirstIndex = 0, IndexCount = 0, FirstInstance = 0, InstanceCount = 0;
		VkIndexType IType;

		VkPipeline MatTypeObj;
		VkPipelineLayout MatTypeLayout;
		VkDescriptorSet *GeneralSet, *PerInstanceSet;
	};
	struct VK_API VK_SubDrawPass {
		unsigned char Binding_Index;
		bool render_dearIMGUI = false;
		VK_IRTSLOTSET* SLOTSET;
		GFX_API::GFXHandle DrawPass;
		TuranAPI::Threading::TLVector<VK_NonIndexedDrawCall> NonIndexedDrawCalls;
		TuranAPI::Threading::TLVector<VK_IndexedDrawCall> IndexedDrawCalls;
		VK_SubDrawPass();
		bool isThereWorkload();
	};
	struct VK_API VK_DrawPass {
		//Name is to debug the rendergraph algorithms, production ready code won't use it!
		string NAME;
		VkRenderPass RenderPassObject;

		VK_RTSLOTSET* SLOTSET;
		std::atomic<unsigned char> SlotSetChanged = false;
		unsigned char Subpass_Count;
		VK_SubDrawPass* Subpasses;
		VkFramebuffer FBs[2];
		GFX_API::BoxRegion RenderRegion;
		vector<GFX_API::PassWait_Description> WAITs;
	};

	struct VK_API VK_WindowCall {
		WINDOW* Window;
	};
	struct VK_API VK_WindowPass {
		string NAME;
		//Element 0 is the Penultimate, Element 1 is the Last, Element 2 is the Current buffers.
		vector<VK_WindowCall> WindowCalls[3];
		vector<GFX_API::PassWait_Description> WAITs;
	};






	//RenderGraph Algorithm Related Resources
	struct VK_BranchPass {
		GFX_API::GFXHandle Handle;
		PassType TYPE;
	};
	/*
	1) VK_RGBranch consists of one or more passes that should be executed serially without executing any other passes
	Note: You can think a VK_RGBranch as a VkSubmitInfo with one command buffer if its all passes are active
	2) VK_RGBranch will be called RB for this spec
	3) a RB start after one or more RBs and end before one or more RBs
	4) Render Graph starts with Root Passes that doesn't wait for any of the current passes
	Note: Don't forget that a Pass may be a Rendering Path too (It doesn't matter if it is a root pass or not)
	5) Passes that waits for the same passes will branch and create their own RBs
	6) If a pass waits for passes that are in different RBs, then this pass will connect them and starts a new RB
	Note: Of course if all of the wait passes are in same RB, algorithm finds the order of execution.
	7) For maximum workload, each RGBranch stores a signal semaphore and list of wait semaphores that point to the signal semaphores of other RGBranches
	But GFX API doesn't have to use the listed semaphores, because there may not be any workload of this branch or any of the dependent branches
	8) ID is only used for now and same branch in different frames have the same ID (Because this ID is generated in current frame construction proccess)
	9) CorePasses is a linked list of all passes that created at RenderGraph_Construction process
	10) CFPasses is a linked list of all passes of the CorePasses that has some workload and CFNeed_QueueSpecs is the queue support flag of these passes
	11) GFX API will analyze the workload of the all passes in the Branch each frame and create CFPasses and CFNeed_QueueSpecs
	*/
	struct VK_RGBranch;
	struct VK_Submit {
		vector<VkPipelineStageFlags> WaitSemaphoreStages;
		vector<unsigned char> WaitSemaphoreIndexes;
		unsigned char SignalSemaphoreIndex, CBIndex = 255;
		//Index + 1
		vector<unsigned char> BranchIndexes;
		VK_QUEUE* Run_Queue = nullptr;
		bool is_EndSubmit = false, is_Ended = false;
	};
	struct VK_RGBranch {
		//Static Datas

		unsigned char ID;	
		VK_BranchPass* CorePasses = nullptr;
		unsigned char PassCount = 0;
		VK_RGBranch **PenultimateSwapchainBranches = nullptr, **LFDependentBranches = nullptr, **CFDependentBranches = nullptr, **LaterExecutedBranches = nullptr;
		unsigned char PenultimateSwapchainBranchCount = 0, LFDependentBranchCount = 0, CFDependentBranchCount = 0, LaterExecutedBranchCount = 0;

		//Dynamic Datas

		VK_QUEUEFLAG CFNeeded_QueueSpecs;
		VK_Submit* AttachedSubmit = nullptr;
		//All of these are real indexes, so there is no sentinel value
		vector<unsigned char> LFDynamicDependents, CFDynamicDependents, DynamicLaterExecutes;
		//All these indexes are indexes of the CorePasses' active passes but +1 because 0 means null
		//That means, if you use these indexes to access a pass from CorePasses, you should access with "index - 1"
		unsigned char* CurrentFramePassesIndexes = nullptr;
	};

	struct VK_FrameGraph {
		VK_RGBranch* FrameGraphTree;
		unsigned char BranchCount = 0;
		vector<VK_Submit*> CurrentFrameSubmits;
	};

}