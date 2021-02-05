#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Texture_Resource.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"
#include "GFX_RenderNodes.h"


namespace GFX_API {

	/*			GFX Content Manager:
	* This system manages all of the GFX asset and GPU memory management proccesses
	* Create_xxx (Texture, Buffer etc) functions will suballocate memory from the specified MemoryType
	* If you want to upload data to the GPU local memory, you should create a buffer to use as a staging memory in either HOSTVISIBLE or FASTHOSTVISIBLE memory
	* Create_xxx and Delete_xxx functions are implicitly synchronized, means you don't have to worry about concurrency

	How to Upload Vertex Buffers:
	1) You should define a Vertex Attribute Layout (which is a collection of Vertex Attributes, so first Vertex Attributes).
	2) GFX API copies VertexAttributeLayout.perVertexSize() * vertex_count amount of data to the Staging Buffer
	3) Then while executing the RenderGraph, Staging->GPU Local copy will be executed 


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


	Why Global Buffer is different than all other resources?
	1) Because you can't create global buffers after RenderGraph construction!
	2) Because global buffers are accessed by all shaders. A Shader needs re-compiling if global buffers list changes.
		

	*/
	class GFXAPI GPU_ContentManager {
	protected:
	public:
		GPU_ContentManager();
		virtual ~GPU_ContentManager();

		virtual void Unload_AllResources() = 0;

		//Registers Vertex Attribute to the system and gives it an ID
		virtual TAPIResult Create_VertexAttribute(const GFX_API::DATA_TYPE& TYPE, const bool& is_perVertex, GFX_API::GFXHandle& Handle) = 0;
		//Returns true if operation is successful
		virtual bool Delete_VertexAttribute(GFXHandle Attribute_ID) = 0;
		/*Attributes are ordered as the same order of input vector
		* For example: Same attribute ID may have different location/order in another attribute layout
		* So you should gather your vertex buffer data according to that
		*/
		virtual TAPIResult Create_VertexAttributeLayout(const vector<GFX_API::GFXHandle>& Attributes, GFX_API::GFXHandle& Handle) = 0;
		virtual void Delete_VertexAttributeLayout(GFXHandle Layout_ID) = 0;

		virtual TAPIResult Create_StagingBuffer(unsigned int DATASIZE, const GFX_API::SUBALLOCATEBUFFERTYPEs& MemoryRegion, GFX_API::GFXHandle& Handle) = 0;
		virtual TAPIResult Uploadto_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET) = 0;
		virtual void Delete_StagingBuffer(GFX_API::GFXHandle StagingBufferHandle) = 0;
		/*
		* You should sort your vertex data according to attribute layout, don't forget that
		* VertexCount shouldn't be 0
		*/
		virtual TAPIResult Create_VertexBuffer(GFX_API::GFXHandle AttributeLayout, unsigned int VertexCount, 
			GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& VertexBufferHandle) = 0;
		virtual TAPIResult Upload_VertexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) = 0;
		virtual void Unload_VertexBuffer(GFX_API::GFXHandle BufferHandle) = 0;

		virtual TAPIResult Create_IndexBuffer(unsigned int DataSize, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& IndexBufferHandle) = 0;
		virtual TAPIResult Upload_IndexBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) = 0;
		virtual void Unload_IndexBuffer(GFX_API::GFXHandle BufferHandle) = 0;
		

		virtual TAPIResult Create_Texture(const GFX_API::Texture_Resource& TEXTURE_ASSET, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& TextureHandle) = 0;
		virtual TAPIResult Upload_Texture(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) = 0;
		//TARGET OFFSET is the offset in the texture's buffer to copy to
		virtual void Unload_Texture(GFXHandle TEXTUREHANDLE) = 0;

		virtual TAPIResult Create_GlobalBuffer(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned int BINDINDEX, bool isUniform,
			GFX_API::SHADERSTAGEs_FLAG AccessableStages, GFX_API::SUBALLOCATEBUFFERTYPEs MemoryType, GFX_API::GFXHandle& GlobalBufferHandle) = 0;
		virtual TAPIResult Upload_GlobalBuffer(GFX_API::GFXHandle BufferHandle, const void* InputData,
			unsigned int DataSize, unsigned int TargetOffset) = 0;
		virtual void Unload_GlobalBuffer(GFXHandle BUFFER_ID) = 0;


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
		virtual void SetMaterial_UniformBuffer(GFX_API::GFXHandle MaterialType_orInstance, bool isMaterialType, bool isUsedRecently, unsigned int BINDINDEX, GFX_API::GFXHandle TargetBufferHandle,
			GFX_API::BUFFER_TYPE BufferType, unsigned int TargetOffset) = 0;


		virtual TAPIResult Create_RTSlotset(const vector<GFX_API::RTSLOT_Description>& Descriptions, GFX_API::GFXHandle& RTSlotSetHandle) = 0;
		virtual TAPIResult Inherite_RTSlotSet(const vector<GFX_API::RTSLOTUSAGE_Description>& Descriptions, GFX_API::GFXHandle RTSlotSetHandle, GFX_API::GFXHandle& InheritedSlotSetHandle) = 0;
	};
}