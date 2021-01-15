#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"


namespace Vulkan {
	struct VK_API VK_CommandBuffer {
		//All of these have sizes that divisible by FrameCount in Renderer
		vector<VkSemaphore*> WaitSemaphores, SignalSemaphores;
		vector<VkPipelineStageFlags*> WaitSemaphoreStages;
		vector<VkCommandBuffer*> CBs;
		VkQueue* Queue;
	};


	struct VK_API VK_MemoryBlock {
		VkDeviceSize Size = 0, Offset = 0;
		bool isEmpty = true;
	};

	struct VK_API VK_MemoryAllocation {
		unsigned int FullSize, UnusedSize;
		VkBufferUsageFlags Usage;
		VkBuffer Base_Buffer;
		VkMemoryRequirements Requirements;
		VkDeviceMemory Allocated_Memory;
		vector<VK_MemoryBlock> Allocated_Blocks;
	};

	struct VK_API VK_Texture {
		unsigned int WIDTH, HEIGHT, DATA_SIZE;
		GFX_API::TEXTURE_CHANNELs CHANNELs;
		GFX_API::TEXTUREUSAGEFLAG USAGE;
		SUBALLOCATEBUFFERTYPEs MEMORYREGION;

		VkImage Image = {};
		VkImageView ImageView = {};
		VkDeviceSize GPUMemoryOffset;
	};
	VK_API VkImageUsageFlags Find_VKImageUsage_forVKTexture(VK_Texture& TEXTURE);

	struct VK_API VK_COLORRTSLOT {
		VK_Texture* RT;
		GFX_API::DRAWPASS_LOAD LOADSTATE;
		bool IS_USED_LATER;
		GFX_API::OPERATION_TYPE RT_OPERATIONTYPE;
		vec4 CLEAR_COLOR;
	};
	struct VK_API VK_DEPTHSTENCILSLOT {
		VK_Texture* RT;
		GFX_API::OPERATION_TYPE DEPTH_OPTYPE;
		GFX_API::OPERATION_TYPE STENCIL_OPTYPE;
		vec2 CLEAR_COLOR;
	};
	struct VK_RTSLOTs {
		VK_COLORRTSLOT* COLOR_SLOTs = nullptr;
		unsigned char COLORSLOTs_COUNT = 0;
		VK_DEPTHSTENCILSLOT* DEPTHSTENCIL_SLOT = nullptr;	//There is one, but there may not be a Depth Slot. So if there is no, then this is nullptr.
		//Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
	};
	struct VK_API VK_RTSLOTSET {
		VK_RTSLOTs PERFRAME_SLOTSETs[2];
	};
	struct VK_API VK_IRTSLOTSET {
		VK_RTSLOTSET* BASESLOTSET;
		GFX_API::OPERATION_TYPE* COLOR_OPTYPEs;
		GFX_API::OPERATION_TYPE DEPTH_OPTYPE;
		GFX_API::OPERATION_TYPE STENCIL_OPTYPE;
	};



	//I want to support InstanceAttribute too but not now because I don't know Vulkan yet
	struct GFXAPI VK_VertexAttribute {
		GFX_API::DATA_TYPE DATATYPE;
	};
	/* Vertex Attribute Layout Specification:
			All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
			Vertex Attributes are created as seperate objects because this helps debug visualization of the data
			Vertex Attributes are goona be ordered by VertexAttributeLayout's vector elements order and this also defines attribute's in-shader location
			That means if position attribute is second element in "Attributes" vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.

	*/
	struct VK_API VK_VertexAttribLayout {
		VK_VertexAttribute** Attributes;
		unsigned int Attribute_Number, size_perVertex;


		VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
		VkVertexInputAttributeDescription* AttribDescs;
		unsigned char AttribDesc_Count;
	};
	struct VK_API VK_VertexBuffer {
		unsigned int VERTEX_COUNT;
		VK_VertexAttribLayout* Layout;
		VkBuffer Buffer;
	};





	struct VK_API VK_GlobalBuffer {
		unsigned int DATA_SIZE, BINDINGPOINT;
		GFX_API::BUFFER_VISIBILITY VISIBILITY;
		GFX_API::SHADERSTAGEs_FLAG ACCESSED_STAGEs;
	};

	struct VK_API VK_DescSetLayout {
		VkDescriptorSetLayout Layout;
		unsigned int IMAGEDESCCOUNT = 0, SAMPLERDESCCOUNT = 0, UBUFFERDESCCOUNT = 0, SBUFFERDESCCOUNT = 0;
	};
	struct VK_API VK_DescPool {
		VkDescriptorPool pool;
		unsigned int REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
	};





	struct VK_API VK_ShaderSource {
		VkShaderModule Module;
	};
	struct VK_SubDrawPass;
	struct VK_API VK_GraphicsPipeline {
		VK_SubDrawPass* GFX_Subpass;
		GFX_API::MaterialDataDescriptor* DATADESCs;
		unsigned int DESCCOUNT, ASSET_ID;

		VkPipelineLayout PipelineLayout;
		VkPipeline PipelineObject;
		VK_DescSetLayout General_DescSetLayout, Instance_DescSetLayout;
		VkDescriptorSet General_DescSet;
	};
	struct VK_API VK_PipelineInstance {
		VK_GraphicsPipeline* PROGRAM;

		VkDescriptorSet DescSet;
	};
	struct VK_API VK_Sampler {
	};




	//RenderNodes

	enum class TPType : unsigned char {
		ERROR = 0,
		DP = 1,
		TP = 2,
		CP = 3,
		WP = 4,
	};

	enum class VK_BUFFERTYPEs : unsigned char {
		MESH = 0,
		STORAGE = 1,
		UNIFORM = 2
	};
	struct VK_ImCopyInfo {
		VK_Texture* SOURCE_IM, DESTINATION_IM;
		GFX_API::IMAGEUSAGE SOURCE_PREVIOUSUSAGE, SOURCE_LATERUSAGE;
		GFX_API::IMAGEUSAGE DESTINATION_PREVIOUSUSAGE, DESTINATION_LATERUSAGE;
		//Add copy region, mipmap etc info here later!
	};
	struct VK_ImDownloadInfo {
		VK_Texture* IMAGE;
	};

	struct VK_ImUploadInfo {
		VK_Texture* IMAGE;
		VkDeviceSize StagingBufferOffset;
	};
	struct VK_BufUploadInfo {
		VK_BUFFERTYPEs BufferType;
		GFX_API::GFXHandle BUFFER;
		VkDeviceSize StagingBufferOffset;
	};
	struct VK_TPUploadDatas {
		TuranAPI::Threading::TLVector<VK_ImUploadInfo> TextureUploads;
		TuranAPI::Threading::TLVector<VK_BufUploadInfo> BufferUploads;
		VK_TPUploadDatas();
	};

	struct VK_ImBarrierInfo {
		VK_Texture* IMAGE;
		GFX_API::IMAGEUSAGE PREVIOUSUSAGE, LATERUSAGE;
		GFX_API::SHADERSTAGEs_FLAG WAITSTAGE, SIGNALSTAGE;
	};
	struct VK_BufBarrierInfo {
		VK_BUFFERTYPEs BufferType;
		GFX_API::GFXHandle BUFFER;
		GFX_API::SHADERSTAGEs_FLAG WAITSTAGE, SIGNALSTAGE;
	};
	struct VK_TPBarrierDatas {
		TuranAPI::Threading::TLVector<VK_ImBarrierInfo> TextureBarriers;
		TuranAPI::Threading::TLVector<VK_BufBarrierInfo> BufferBarriers;
		VK_TPBarrierDatas();
	};

	struct VK_TransferPass {
		VK_CommandBuffer* CommandBuffer;
		GFX_API::GFXHandle TransferDatas;
		vector<GFX_API::PassWait_Description> WAITs;
		GFX_API::TRANFERPASS_TYPE TYPE;
		string NAME;
	};

	struct VK_API VK_SubDrawPass {
		unsigned char Binding_Index;
		VK_IRTSLOTSET* SLOTSET;
		GFX_API::GFXHandle DrawPass;
		//vector<GFX_API::DrawCall_Description> DrawCalls;
	};
	struct VK_API VK_DrawPass {
		//Name is to debug the rendergraph algorithms, production ready code won't use it!
		string NAME;
		VkRenderPass RenderPassObject;

		VK_RTSLOTSET* SLOTSET;
		unsigned char Subpass_Count;
		VK_SubDrawPass* Subpasses;
		vector<GFX_API::PassWait_Description> WAITs;
	};

	struct VK_API VK_WindowCall {
		VK_ImBarrierInfo BarrierInfo;
		std::function<void()> JobToDo;
		VkSwapchainKHR Swapchain;
	};
	struct VK_API VK_WindowPass {
		string NAME;
		vector<VK_WindowCall> WindowCalls;
		vector<GFX_API::PassWait_Description> WAITs;
	};






	//RenderGraph Algorithm Related Resources

	struct VK_DrawPassInstance {
		VK_DrawPass* BasePass;
		VkFramebuffer Framebuffer;
	};


	struct VK_BranchPass {
		GFX_API::GFXHandle Handle;	//Use VK_DrawPassInstance if is_TransferPass is true, otherwise use VK_TransferPass
		TPType TYPE;
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
		VK_CommandBuffer* CB = nullptr;
		//Index + 1
		vector<unsigned char> BranchIndexes;
		vector<unsigned char> WaitSemaphoreIndexes;
		unsigned char SignalSemaphoreIndex = 0;
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