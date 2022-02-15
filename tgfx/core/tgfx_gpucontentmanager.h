#pragma once
#include "tgfx_forwarddeclarations.h"


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

	typedef struct gpudatamanager_tgfx {
		void (*Destroy_AllResources)();


		result_tgfx (*Create_SamplingType)(unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
			texture_mipmapfilter_tgfx MINFILTER, texture_mipmapfilter_tgfx MAGFILTER, texture_wrapping_tgfx WRAPPING_WIDTH,
			texture_wrapping_tgfx WRAPPING_HEIGHT, texture_wrapping_tgfx WRAPPING_DEPTH, samplingtype_tgfx_handle* SamplingTypeHandle);

		//Attributes list should end with datatype_tgfx_UNDEFINED (one extra element at the end)
    
		result_tgfx (*Create_VertexAttributeLayout)(const datatype_tgfx* Attributes, vertexlisttypes_tgfx listtype, 
			vertexattributelayout_tgfx_handle* VertexAttributeLayoutHandle);
		void (*Delete_VertexAttributeLayout)(vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle);

		result_tgfx (*Upload_toBuffer)(buffer_tgfx_handle Handle, buffertype_tgfx Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET);

		result_tgfx (*Create_StagingBuffer)(unsigned int DATASIZE, unsigned int MemoryTypeIndex, buffer_tgfx_handle* Handle);
		void (*Delete_StagingBuffer)(buffer_tgfx_handle StagingBufferHandle);
		/*
		* You should sort your vertex data according to attribute layout, don't forget that
		* VertexCount shouldn't be 0
		*/
		result_tgfx (*Create_VertexBuffer)(vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle, unsigned int VertexCount,
			unsigned int MemoryTypeIndex, buffer_tgfx_handle* VertexBufferHandle);
		void (*Unload_VertexBuffer)(buffer_tgfx_handle BufferHandle);

		result_tgfx (*Create_IndexBuffer)(datatype_tgfx DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, buffer_tgfx_handle* IndexBufferHandle);
		void (*Unload_IndexBuffer)(buffer_tgfx_handle BufferHandle);

		result_tgfx (*Create_Texture)(texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, texture_channels_tgfx CHANNEL_TYPE,
			unsigned char MIPCOUNT, textureusageflag_tgfx_handle USAGE, texture_order_tgfx DATAORDER, unsigned int MemoryTypeIndex, texture_tgfx_handle* TextureHandle);
		//TARGET OFFSET is the offset in the texture's buffer to copy to
		result_tgfx (*Upload_Texture)(texture_tgfx_handle TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset);
		void (*Delete_Texture)(texture_tgfx_handle TEXTUREHANDLE, unsigned char isUsedLastFrame);

		result_tgfx (*Create_GlobalBuffer)(const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform,
			unsigned int MemoryTypeIndex, buffer_tgfx_handle* GlobalBufferHandle);
		void (*Unload_GlobalBuffer)(buffer_tgfx_handle BUFFER_ID);
		result_tgfx (*SetGlobalShaderInput_Buffer)(unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame, 
			buffer_tgfx_handle BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize);
		result_tgfx (*SetGlobalShaderInput_Texture)(unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame, 
			texture_tgfx_handle TextureHandle, samplingtype_tgfx_handle sampler, image_access_tgfx access);


		result_tgfx (*Compile_ShaderSource)(shaderlanguages_tgfx language, shaderstage_tgfx shaderstage, void* DATA, unsigned int DATA_SIZE, 
			shadersource_tgfx_handle* ShaderSourceHandle);
		void (*Delete_ShaderSource)(shadersource_tgfx_handle ShaderSourceHandle);
		result_tgfx (*Link_MaterialType)(shadersource_tgfx_listhandle ShaderSourcesList, shaderinputdescription_tgfx_listhandle ShaderInputDescs, 
			vertexattributelayout_tgfx_handle AttributeLayout, subdrawpass_tgfx_handle Subdrawpass, cullmode_tgfx culling, 
			polygonmode_tgfx polygonmode, depthsettings_tgfx_handle depthtest, stencilsettings_tgfx_handle StencilFrontFaced, 
			stencilsettings_tgfx_handle StencilBackFaced, blendinginfo_tgfx_listhandle BLENDINGs, rasterpipelinetype_tgfx_handle* MaterialHandle);
		void (*Delete_MaterialType)(rasterpipelinetype_tgfx_handle ID);
		result_tgfx (*Create_MaterialInst)(rasterpipelinetype_tgfx_handle MaterialType, rasterpipelineinstance_tgfx_handle * MaterialInstHandle);
		void (*Delete_MaterialInst)(rasterpipelineinstance_tgfx_handle ID);
		
		result_tgfx (*Create_ComputeType)(shadersource_tgfx_handle Source, shaderinputdescription_tgfx_listhandle ShaderInputDescs, computeshadertype_tgfx_handle* ComputeTypeHandle);
		result_tgfx (*Create_ComputeInstance)(computeshadertype_tgfx_handle ComputeType, computeshaderinstance_tgfx_handle* ComputeShaderInstanceHandle);
		void (*Delete_ComputeShaderType)(computeshadertype_tgfx_handle ID);
		void (*Delete_ComputeShaderInstance)(computeshaderinstance_tgfx_handle ID);

		//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
		result_tgfx (*SetMaterialType_Buffer)(rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		result_tgfx (*SetMaterialType_Texture)(rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage);
		
		result_tgfx (*SetMaterialInst_Buffer)(rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		result_tgfx (*SetMaterialInst_Texture)(rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage);
		//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
		result_tgfx (*SetComputeType_Buffer)(computeshadertype_tgfx_handle ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		result_tgfx (*SetComputeType_Texture)(computeshadertype_tgfx_handle ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
			texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage);
		
		result_tgfx (*SetComputeInst_Buffer)(computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize);
		result_tgfx (*SetComputeInst_Texture)(computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
			texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage);


		result_tgfx (*Create_RTSlotset)(rtslotdescription_tgfx_listhandle Descriptions, rtslotset_tgfx_handle* RTSlotSetHandle);
		void (*Delete_RTSlotSet)(rtslotset_tgfx_handle RTSlotSetHandle);
		//Changes on RTSlots only happens at the frame slot is gonna be used
		//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
		//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
		result_tgfx (*Change_RTSlotTexture)(rtslotset_tgfx_handle RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, texture_tgfx_handle TextureHandle);
		result_tgfx (*Inherite_RTSlotSet)(rtslotusage_tgfx_listhandle Descriptions, rtslotset_tgfx_handle RTSlotSetHandle, inheritedrtslotset_tgfx_handle* InheritedSlotSetHandle);
		void (*Delete_InheritedRTSlotSet)(inheritedrtslotset_tgfx_handle InheritedRTSlotSetHandle);
	} gpudatamanager_tgfx;