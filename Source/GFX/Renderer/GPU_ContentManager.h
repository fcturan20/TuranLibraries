#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"
#include "GFX_RenderNodes.h"


namespace GFX_API {

	/*			GFX Content Manager:
	* This system manages all of the GFX asset and GPU memory management proccesses
	* Create_xxx (Texture, Buffer etc) functions will suballocate memory from the specified MemoryType
	* If you want to upload data to the GPU local memory, you should create Staging Buffer in either HOSTVISIBLE or FASTHOSTVISIBLE memory
	* Create_xxx and Delete_xxx functions are implicitly synchronized, means you don't have to worry about concurrency

	How to Upload Vertex Buffers:
	1) You should define a Vertex Attribute Layout (which is a collection of Vertex Attributes, so first Vertex Attributes).
	2) If you specify HOSTVISIBLE or FASTHOSTVISIBLE while creating the vertex buffer, you can use Upload_toBuffer() function to upload data
	3) If you specify DEVICELOCAL or READBACK, you can only upload data to some other buffer and copy from it to the vertex buffer while executing RenderGraph


	Why is Vertex Attribute Layout system that much complicated?
	1) Vulkan API is very limiting in terms of Vertex Attribute Layout capabilities (Because the layout is given in the Graphics Pipeline compiling process).
	2) So Turan Engine's Vertex Attribute Layout capabilities will be limited to interleaved rendering.
	3) I want some matching all over the vertex attribute layouts because vertex attributes always has meanings (Position, Vertex Normal, Texture Coordinates etc)
	and their order is less meaningful than their existence. That's why you should create a VertexAttribute object, it has a meaning.


	What does mean these RTSlot and RTSlotSet things?
	1) Draw Pass is to render something on a texture called Render Target (RT). But each SubDrawPass should define which Draw Pass' RTs it uses.
	2) Also it must be possible to change RTs. So this textureless RT is called RTSlot, which should point to a RT texture.
	3) Draw Pass should store all these RTSlots, so this collection of RTSlots is called RTSlotSet.
	4) But a SubDrawPass should only define which slots it uses (It shouldn't point to some other texture etc).
	5) So SubDrawPass should use a inherited the RTSlotSet of the DrawPass it's in, IRTSlotSet it's called.
	6) How to create them is described well in related function's arguments and argument's structure definitions in GFX_RenderNodes.h.


	Why Global Buffer and Global Textures are different than all other resources?
	1) Because you can't create globals after RenderGraph construction!
	2) Because globals are accessed by all shaders. A Shader needs re-compiling if global buffers list changes.
	3) Global Buffer is a buffer (like all other buffers) but Global Texture is a description of a Shader Input
	So you can change a global texture to point to some other texture
		

	How does Material System works?
	1) All of the shader inputs is divided into 3 categories: Global, Material Type (General), Material Instance (Per Instance)
	2) Array of the shader inputs aren't supported for now
	3) General and Per Instance shader inputs should be set with SetMaterial_XXX() functions
	4) Related SetMaterial_XXX() function of a shader input can only be called once per frame, second one (or concurrent one) will fail
	5) That means you can change 2 different shader inputs in the same frame, but you can't change the same shader inputs twice in a frame
	6) Globals are only uniform and storage buffers for now (Set = 0 in SPIR-V shaders) and can be accessible from all materials
	7) Material Type (General) ones are only accessible by material instances that are instantiated by the same Material Type (Set = 1 in SPIR-V shaders)
	8) Material Instance ones are material instance specific (Set = 2 in SPIR-V shaders)

	*/
	class GFXAPI GPU_ContentManager {
	protected:
	public:
		GPU_ContentManager();
		virtual ~GPU_ContentManager();
		virtual void Destroy_AllResources() = 0;


		virtual TAPIResult Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs dimension, unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
			GFX_API::TEXTURE_MIPMAPFILTER MINFILTER, GFX_API::TEXTURE_MIPMAPFILTER MAGFILTER, GFX_API::TEXTURE_WRAPPING WRAPPING_WIDTH,
			GFX_API::TEXTURE_WRAPPING WRAPPING_HEIGHT, GFX_API::TEXTURE_WRAPPING WRAPPING_DEPTH, GFX_API::GFXHandle& SamplingTypeHandle) = 0;

		/*Attributes are ordered as the same order of input vector
		* For example: Same attribute ID may have different location/order in another attribute layout
		* So you should gather your vertex buffer data according to that
		*/
		virtual TAPIResult Create_VertexAttributeLayout(const vector<GFX_API::DATA_TYPE>& Attributes, GFX_API::VERTEXLIST_TYPEs listtype, GFX_API::GFXHandle& Handle) = 0;
		virtual void Delete_VertexAttributeLayout(GFXHandle Layout_ID) = 0;

		virtual TAPIResult Upload_toBuffer(GFX_API::GFXHandle Handle, GFX_API::BUFFER_TYPE Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) = 0;

		virtual TAPIResult Create_StagingBuffer(unsigned int DATASIZE, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& Handle) = 0;
		virtual void Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) = 0;
		/*
		* You should sort your vertex data according to attribute layout, don't forget that
		* VertexCount shouldn't be 0
		*/
		virtual TAPIResult Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
			unsigned int MemoryTypeIndex, GFX_API::GFXHandle& VertexBufferHandle) = 0;
		virtual void Unload_VertexBuffer(GFX_API::GFXHandle BufferHandle) = 0;

		virtual TAPIResult Create_IndexBuffer(GFX_API::DATA_TYPE DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& IndexBufferHandle) = 0;
		virtual void Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) = 0;
		
		virtual TAPIResult Create_Texture(const GFX_API::Texture_Description& TEXTURE_ASSET, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& TextureHandle) = 0;
		//TARGET OFFSET is the offset in the texture's buffer to copy to
		virtual TAPIResult Upload_Texture(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) = 0;
		virtual void Delete_Texture(GFXHandle TEXTUREHANDLE, bool isUsedLastFrame) = 0;

		virtual TAPIResult Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform,
			GFX_API::SHADERSTAGEs_FLAG AccessableStages, unsigned int MemoryTypeIndex, GFX_API::GFXHandle& GlobalBufferHandle) = 0;
		virtual void Unload_GlobalBuffer(GFXHandle BUFFER_ID) = 0;
		virtual TAPIResult Create_GlobalTexture(const char* TEXTURE_NAME, bool isSampledTexture, unsigned int BINDINDEX, unsigned int TextureCount, GFX_API::SHADERSTAGEs_FLAG AccessableStages,
			GFX_API::GFXHandle& GlobalTextureHandle) = 0;
		virtual TAPIResult SetGlobal_SampledTexture(GFX_API::GFXHandle GlobalTextureHandle, unsigned int TextureIndex, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType,
			GFX_API::IMAGE_ACCESS access) = 0; 
		virtual TAPIResult SetGlobal_ImageTexture(GFX_API::GFXHandle GlobalTextureHandle, unsigned int TextureIndex, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType,
			GFX_API::IMAGE_ACCESS access) = 0;


		//Return handle to reference in GFX!
		virtual TAPIResult Compile_ShaderSource(const GFX_API::ShaderSource_Resource* SHADER, GFX_API::GFXHandle& ShaderSourceHandle) = 0;
		virtual void Delete_ShaderSource(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual TAPIResult Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, GFX_API::GFXHandle* Handles, unsigned int Count) = 0;
		virtual void Delete_ComputeShader(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual TAPIResult Link_MaterialType(const GFX_API::Material_Type& MATTYPE_ASSET, GFX_API::GFXHandle& MaterialHandle) = 0;
		virtual void Delete_MaterialType(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual TAPIResult Create_MaterialInst(GFX_API::GFXHandle MaterialType, GFX_API::GFXHandle& MaterialInstHandle) = 0;
		virtual void Delete_MaterialInst(GFXHandle ID) = 0;
		//If isMaterialType is false, MaterialType_orInstance input will be proccessed as MaterialInstance handle
		//IsUsedRecently means is the material type/instance used in last two frames. This is necessary for Vulkan synchronization process.
		virtual TAPIResult SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
			GFX_API::GFXHandle TargetBufferHandle, unsigned int ELEMENTINDEX, GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset, unsigned int BoundDataSize) = 0;
		virtual TAPIResult SetMaterial_SampledTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
			unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) = 0;
		virtual TAPIResult SetMaterial_ImageTexture(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX,
			unsigned int ELEMENTINDEX, GFX_API::GFXHandle TextureHandle, GFX_API::GFXHandle SamplingType, GFX_API::IMAGE_ACCESS usage) = 0;


		virtual TAPIResult Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) = 0;
		//Changes on RTSlots only happens at the frame slot is gonna be used
		//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
		//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
		virtual TAPIResult Change_RTSlotTexture(GFX_API::GFXHandle RTSlotHandle, bool isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, GFX_API::GFXHandle TextureHandle) = 0;
		virtual TAPIResult Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) = 0;
		virtual void Delete_RTSlotSet(GFX_API::GFXHandle RTSlotSetHandle) = 0;
	};
}