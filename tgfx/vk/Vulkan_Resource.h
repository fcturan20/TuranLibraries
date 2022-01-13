#pragma once
#include "Vulkan_Includes.h"
#include <glm/glm.hpp>
#include <string>


//This structure defines offset of the suballocation in the big memory allocation that happens at initialization
struct MemoryBlock {
	unsigned int MemAllocIndex = UINT32_MAX;
	VkDeviceSize Offset;
};

struct VK_Texture {
	unsigned int WIDTH, HEIGHT, DATA_SIZE;
	unsigned char MIPCOUNT;
	tgfx_texture_channels CHANNELs;
	VkImageUsageFlags USAGE;
	tgfx_texture_dimensions DIMENSION;

	VkImage Image = {};
	VkImageView ImageView = {};
	MemoryBlock Block;
};
VkImageUsageFlags Find_VKImageUsage_forVKTexture(VK_Texture& TEXTURE);
VkImageUsageFlags Find_VKImageUsage_forGFXTextureDesc(const VkImageUsageFlags& USAGEFLAG, tgfx_texture_channels channels);
void Find_AccessPattern_byIMAGEACCESS(const tgfx_imageaccess Access, VkAccessFlags& TargetAccessFlag, VkImageLayout& TargetImageLayout);
VkImageTiling Find_VkTiling(tgfx_texture_order order);
VkImageType Find_VkImageType(tgfx_texture_dimensions dimensions);

struct VK_COLORRTSLOT {
	VK_Texture* RT;
	tgfx_drawpassload LOADSTATE;
	bool IS_USED_LATER;
	tgfx_operationtype RT_OPERATIONTYPE;
	glm::vec4 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct VK_DEPTHSTENCILSLOT {
	VK_Texture* RT;
	tgfx_drawpassload DEPTH_LOAD, STENCIL_LOAD;
	bool IS_USED_LATER;
	tgfx_operationtype DEPTH_OPTYPE, STENCIL_OPTYPE;
	glm::vec2 CLEAR_COLOR;
	std::atomic_bool IsChanged = false;
};
struct VK_RTSLOTs {
	VK_COLORRTSLOT* COLOR_SLOTs = nullptr;
	unsigned char COLORSLOTs_COUNT = 0;
	VK_DEPTHSTENCILSLOT* DEPTHSTENCIL_SLOT = nullptr;	//There is one, but there may not be a Depth Slot. So if there is no, then this is nullptr.
	//Unused Depth and NoDepth are different. Unused Depth means RenderPass does have one but current Subpass doesn't use, but NoDepth means RenderPass doesn't have one!
	std::atomic_bool IsChanged = false;
};
struct VK_RTSLOTSET {
	VK_RTSLOTs PERFRAME_SLOTSETs[2];
	//You should change this struct's vkRenderPass object pointer as your vkRenderPass object
	VkFramebufferCreateInfo FB_ci[2];
	std::vector<VkImageView> ImageViews[2];
};
struct VK_IRTSLOTSET {
	VK_RTSLOTSET* BASESLOTSET;
	tgfx_operationtype* COLOR_OPTYPEs;
	tgfx_operationtype DEPTH_OPTYPE;
	tgfx_operationtype STENCIL_OPTYPE;
};


/* Vertex Attribute Layout Specification:
		All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
		Vertex Attributes are created as seperate objects because this helps debug visualization of the data
		Vertex Attributes are gonna be ordered by VertexAttributeLayout's std::vector elements order and this also defines attribute's in-shader location
		That means if position attribute is second element in "Attributes" std::vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.
*/
struct VK_VertexAttribLayout {
	tgfx_datatype* Attributes;
	unsigned int Attribute_Number, size_perVertex;


	VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
	VkVertexInputAttributeDescription* AttribDescs;
	VkPrimitiveTopology PrimitiveTopology;
	unsigned char AttribDesc_Count;
};
struct VK_VertexBuffer {
	unsigned int VERTEX_COUNT;
	VK_VertexAttribLayout* Layout;
	MemoryBlock Block;
};
struct VK_IndexBuffer {
	VkIndexType DATATYPE;
	VkDeviceSize IndexCount = 0;
	MemoryBlock Block;
};


struct VK_GlobalBuffer {
	VkDeviceSize DATA_SIZE;
	MemoryBlock Block;
	bool isUniform;
};

struct VK_DescBufferElement {
	VkDescriptorBufferInfo Info = {};
	std::atomic<unsigned char> IsUpdated = 255;
	VK_DescBufferElement();
	VK_DescBufferElement(const VK_DescBufferElement& copyDesc);
};
struct VK_DescImageElement {
	VkDescriptorImageInfo info = {};
	std::atomic<unsigned char> IsUpdated = 255;
	VK_DescImageElement();
	VK_DescImageElement(const VK_DescImageElement& copyDesc);
};
struct VK_Descriptor {
	DescType Type;
	void* Elements = nullptr;
	unsigned int ElementCount = 0;
};
struct VK_DescSet {
	VkDescriptorSet Set = VK_NULL_HANDLE;
	VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	VK_Descriptor* Descs = nullptr;
	//DescCount is size of the "Descs" pointer above, others are including element counts of each descriptor
	unsigned int DescCount = 0, DescUBuffersCount = 0, DescSBuffersCount = 0, DescSamplersCount = 0, DescImagesCount = 0;
	std::atomic_bool ShouldRecreate = false;
};
struct VK_DescSetUpdateCall {
	VK_DescSet* Set;
	unsigned int BindingIndex, ElementIndex;
};
struct VK_DescPool {
	VkDescriptorPool pool;
	tapi_atomic_uint REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
};





struct VK_ShaderSource {
	VkShaderModule Module;
	tgfx_shaderstage stage;
	void* SOURCE_CODE = nullptr;
	unsigned int DATA_SIZE = 0;
};
struct VK_SubDrawPass;
struct VK_GraphicsPipeline {
	VK_SubDrawPass* GFX_Subpass;

	VkPipelineLayout PipelineLayout;
	VkPipeline PipelineObject;
	VK_DescSet General_DescSet, Instance_DescSet;
};
struct VK_PipelineInstance {
	VK_GraphicsPipeline* PROGRAM;
	VK_DescSet DescSet;
};
struct VK_Sampler {
	VkSampler Sampler;
};
struct VK_ComputePipeline {
	VkPipeline PipelineObject;
	VkPipelineLayout PipelineLayout;
	VK_DescSet General_DescSet, Instance_DescSet;
};
struct VK_ComputeInstance {
	VK_ComputePipeline* PROGRAM;
	VK_DescSet DescSet;
};




//RENDER NODEs
struct VK_Pass;
struct VK_PassWaitDescription {
	VK_Pass** WaitedPass = nullptr;
	bool WaitLastFramesPass = false;
	bool isValid() const;
};
enum class PassType : unsigned char {
	ERROR = 0,
	DP = 1,
	TP = 2,
	CP = 3,
	WP = 4,
};
struct VK_Pass {
	//Name is to debug the rendergraph algorithms, production ready code won't use it!
	std::string NAME;
	VK_PassWaitDescription* const WAITs;
	const unsigned char WAITsCOUNT;
	PassType TYPE;
	//This is to store which branch this pass is used in last frames
	//With that way, we can identify which semaphore we should wait on if rendergraph is reconstructed
	//This is UINT32_MAX if pass isn't used last frame
	unsigned int LastUsedBranchID = UINT32_MAX;


	VK_Pass(const std::string& name, PassType type, unsigned int WAITSCOUNT);
	virtual bool IsWorkloaded() = 0;
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
	tapi_threadlocal_vector<VK_BUFtoIMinfo> BUFIMCopies;
	tapi_threadlocal_vector<VK_BUFtoBUFinfo> BUFBUFCopies;
	tapi_threadlocal_vector<VK_IMtoIMinfo> IMIMCopies;
	VK_TPCopyDatas();
};

struct VK_ImBarrierInfo {
	VkImageMemoryBarrier Barrier;
};
struct VK_BufBarrierInfo {
	VkBufferMemoryBarrier Barrier;
};
struct VK_TPBarrierDatas {
	tapi_threadlocal_vector<VK_ImBarrierInfo> TextureBarriers;
	tapi_threadlocal_vector<VK_BufBarrierInfo> BufferBarriers;
	VK_TPBarrierDatas();
	VK_TPBarrierDatas(const VK_TPBarrierDatas& copyfrom);
};

struct transferpass_vk : public VK_Pass {
	void* TransferDatas;
	tgfx_transferpass_type TYPE;

	transferpass_vk(const char* name, unsigned int WAITSCOUNT);
	virtual bool IsWorkloaded() override;
};

//COMPUTE PASS

struct VK_DispatchCall {
	VkPipelineLayout Layout = VK_NULL_HANDLE;
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkDescriptorSet* GeneralSet = nullptr, * InstanceSet = nullptr;
	glm::uvec3 DispatchSize;
};

struct VK_SubComputePass {
	VK_TPBarrierDatas Barriers_AfterSubpassExecutions;
	tapi_threadlocal_vector<VK_DispatchCall> Dispatches;
	VK_SubComputePass();
	bool isThereWorkload();
};

struct computepass_vk : public VK_Pass {
	std::vector<VK_SubComputePass> Subpasses;
	std::atomic_bool SubPassList_Updated = false;

	computepass_vk(const std::string& name, unsigned int WAITSCOUNT);
	virtual bool IsWorkloaded() override;
};

//DRAW PASS

struct VK_NonIndexedDrawCall {
	VkBuffer VBuffer;
	VkDeviceSize VOffset = 0;
	uint32_t FirstVertex = 0, VertexCount = 0, FirstInstance = 0, InstanceCount = 0;

	VkPipeline MatTypeObj;
	VkPipelineLayout MatTypeLayout;
	VkDescriptorSet* GeneralSet, * PerInstanceSet;
};
struct VK_IndexedDrawCall {
	VkBuffer VBuffer, IBuffer;
	VkDeviceSize VBOffset = 0, IBOffset = 0;
	uint32_t VOffset = 0, FirstIndex = 0, IndexCount = 0, FirstInstance = 0, InstanceCount = 0;
	VkIndexType IType;

	VkPipeline MatTypeObj;
	VkPipelineLayout MatTypeLayout;
	VkDescriptorSet* GeneralSet, * PerInstanceSet;
};
struct drawpass_vk;
struct VK_SubDrawPass {
	unsigned char Binding_Index;
	bool render_dearIMGUI = false;
	VK_IRTSLOTSET* SLOTSET;
	drawpass_vk* DrawPass;
	tapi_threadlocal_vector<VK_NonIndexedDrawCall> NonIndexedDrawCalls;
	tapi_threadlocal_vector<VK_IndexedDrawCall> IndexedDrawCalls;

	VK_SubDrawPass();
	bool isThereWorkload();
};
struct drawpass_vk : public VK_Pass {
	VkRenderPass RenderPassObject;
	VK_RTSLOTSET* SLOTSET;
	std::atomic<unsigned char> SlotSetChanged = false;
	unsigned char Subpass_Count;
	VK_SubDrawPass* Subpasses;
	VkFramebuffer FBs[2]{ VK_NULL_HANDLE };
	tgfx_BoxRegion RenderRegion;

	drawpass_vk(const std::string& name, unsigned int WAITSCOUNT);
	virtual bool IsWorkloaded() override;
};

//WINDOW PASS

struct VK_WindowCall {
	WINDOW* Window;
};
struct windowpass_vk : public VK_Pass {
	//Element 0 is the Penultimate, Element 1 is the Last, Element 2 is the Current buffers.
	std::vector<VK_WindowCall> WindowCalls[3];

	windowpass_vk(const std::string& name, unsigned int WAITSCOUNT);
	virtual bool IsWorkloaded() override;
};



struct VK_FrameGraph {
	//Branch and submit info
	//These are typeless because I may implement multiple branch and submit data structure
	//And also, type of these datas aren't used anywhere but FGAlgorithm.cpp
	void* FrameGraphTree = nullptr, * SubmitList = nullptr;
	unsigned char BranchCount = 0;
};