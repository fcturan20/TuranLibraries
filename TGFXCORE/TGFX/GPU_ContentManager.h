#pragma once
#include "GFX_Includes.h"



#if defined(__cplusplus)
extern "C" {
#endif

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
	2) General and Per Instance shader inputs should be set with SetMaterial_XXX() functions
	3) Related SetMaterial_XXX() function of a shader input can only be called once per frame, second one (or concurrent one) will fail
	4) That means you can change 2 different shader inputs in the same frame, but you can't change the same shader inputs twice in a frame
	5) Depending on your GPU's "Descriptor Indexing" support, global shader input handling differs.
	6) Material Type (General) ones are only accessible by material instances that are instantiated by the same Material Type
	7) Material Instance ones are material instance specific
	8) There are 4 types of shader inputs: SampledTexture, ImageTexture, UniformBuffer, StorageBuffer (same as Khronos)
	9) Please read "Resources_README.md" for details
	*/
	typedef struct tgfx_gpudatamanager {
		void (*Destroy_AllResources)();


		tgfx_result (*Create_SamplingType)(unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
			tgfx_texture_mipmapfilter MINFILTER, tgfx_texture_mipmapfilter MAGFILTER, tgfx_texture_wrapping WRAPPING_WIDTH,
			tgfx_texture_wrapping WRAPPING_HEIGHT, tgfx_texture_wrapping WRAPPING_DEPTH, tgfx_samplingtype* SamplingTypeHandle);

		/*Attributes are ordered as the same order of input vector
		* For example: Same attribute ID may have different location/order in another attribute layout
		* So you should gather your vertex buffer data according to that
		*/
		tgfx_result (*Create_VertexAttributeLayout)(const tgfx_datatype* Attributes, tgfx_vertexlisttypes listtype, 
			tgfx_vertexattributelayout* VertexAttributeLayoutHandle);
		void (*Delete_VertexAttributeLayout)(tgfx_vertexattributelayout VertexAttributeLayoutHandle);

		tgfx_result (*Upload_toBuffer)(tgfx_buffer Handle, tgfx_buffertype Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET);

		tgfx_result (*Create_StagingBuffer)(unsigned int DATASIZE, unsigned int MemoryTypeIndex, tgfx_buffer* Handle);
		void (*Delete_StagingBuffer)(tgfx_buffer StagingBufferHandle);
		/*
		* You should sort your vertex data according to attribute layout, don't forget that
		* VertexCount shouldn't be 0
		*/
		tgfx_result (*Create_VertexBuffer)(tgfx_vertexattributelayout VertexAttributeLayoutHandle, unsigned int VertexCount,
			unsigned int MemoryTypeIndex, tgfx_buffer* VertexBufferHandle);
		void (*Unload_VertexBuffer)(tgfx_buffer BufferHandle);

		tgfx_result (*Create_IndexBuffer)(tgfx_datatype DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, tgfx_buffer* IndexBufferHandle);
		void (*Unload_IndexBuffer)(tgfx_buffer BufferHandle);

		tgfx_result (*Create_Texture)(tgfx_texture_dimensions DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, tgfx_texture_channels CHANNEL_TYPE,
			unsigned char MIPCOUNT, tgfx_textureusageflag USAGE, tgfx_texture_order DATAORDER, unsigned int MemoryTypeIndex, tgfx_texture* TextureHandle);
		//TARGET OFFSET is the offset in the texture's buffer to copy to
		tgfx_result (*Upload_Texture)(tgfx_texture TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset);
		void (*Delete_Texture)(tgfx_texture TEXTUREHANDLE, unsigned char isUsedLastFrame);

		tgfx_result (*Create_GlobalBuffer)(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform,
			unsigned int MemoryTypeIndex, tgfx_buffer* GlobalBufferHandle);
		void (*Unload_GlobalBuffer)(tgfx_buffer BUFFER_ID);
		tgfx_result (*SetGlobalShaderInput_Buffer)(unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame, 
			tgfx_buffer BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize);
		tgfx_result (*SetGlobalShaderInput_Texture)(unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame, 
			tgfx_texture TextureHandle, tgfx_samplingtype sampler, tgfx_imageaccess access);


		tgfx_result (*Compile_ShaderSource)(tgfx_shaderlanguages language, tgfx_shaderstage shaderstage, void* DATA, unsigned int DATA_SIZE, 
			tgfx_shadersource* ShaderSourceHandle);
		void (*Delete_ShaderSource)(tgfx_shadersource ShaderSourceHandle);
		tgfx_result (*Link_MaterialType)(tgfx_shadersource_list ShaderSourcesList, tgfx_shaderinputdescription_list ShaderInputDescs, 
			tgfx_vertexattributelayout AttributeLayout, tgfx_subdrawpass Subdrawpass, tgfx_cullmode culling, 
			tgfx_polygonmode polygonmode, tgfx_depthsettings depthtest, tgfx_stencilsettings StencilFrontFaced, 
			tgfx_stencilsettings StencilBackFaced, tgfx_blendinginfo_list BLENDINGs, tgfx_rasterpipelinetype* MaterialHandle);
		void (*Delete_MaterialType)(tgfx_rasterpipelinetype ID);
		tgfx_result (*Create_MaterialInst)(tgfx_rasterpipelinetype MaterialType, tgfx_rasterpipelineinstance * MaterialInstHandle);
		void (*Delete_MaterialInst)(tgfx_rasterpipelineinstance ID);
		
		tgfx_result (*Create_ComputeType)(tgfx_shadersource Source, tgfx_shaderinputdescription_list ShaderInputDescs, tgfx_computeshadertype* ComputeTypeHandle);
		tgfx_result (*Create_ComputeInstance)(tgfx_computeshadertype ComputeType, tgfx_computeshaderinstance* ComputeShaderInstanceHandle);
		void (*Delete_ComputeShaderType)(tgfx_computeshadertype ID);
		void (*Delete_ComputeShaderInstance)(tgfx_computeshaderinstance ID);

		//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
		tgfx_result (*SetMaterialType_Buffer)(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		tgfx_result (*SetMaterialType_Texture)(tgfx_rasterpipelinetype MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);
		
		tgfx_result (*SetMaterialInst_Buffer)(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_buffer BufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		tgfx_result (*SetMaterialInst_Texture)(tgfx_rasterpipelineinstance MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);
		//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
		tgfx_result (*SetComputeType_Buffer)(tgfx_computeshadertype ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		tgfx_result (*SetComputeType_Texture)(tgfx_computeshadertype ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);
		
		tgfx_result (*SetComputeInst_Buffer)(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_buffer TargetBufferHandle, tgfx_buffertype BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		tgfx_result (*SetComputeInst_Texture)(tgfx_computeshaderinstance ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			tgfx_texture TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, tgfx_samplingtype Sampler, tgfx_imageaccess usage);


		tgfx_result (*Create_RTSlotset)(tgfx_rtslotdescription_list Descriptions, tgfx_rtslotset* RTSlotSetHandle);
		void (*Delete_RTSlotSet)(tgfx_rtslotset RTSlotSetHandle);
		//Changes on RTSlots only happens at the frame slot is gonna be used
		//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
		//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
		tgfx_result (*Change_RTSlotTexture)(tgfx_rtslotset RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, tgfx_texture TextureHandle);
		tgfx_result (*Inherite_RTSlotSet)(tgfx_rtslotusage_list Descriptions, tgfx_rtslotset RTSlotSetHandle, tgfx_inheritedrtslotset* InheritedSlotSetHandle);
		void (*Delete_InheritedRTSlotSet)(tgfx_inheritedrtslotset InheritedRTSlotSetHandle);
	} tgfx_gpudatamanager;

#if defined(__cplusplus)
}
#endif