#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

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
	1) All of the shader inputs is divided into 3 categories: Global, Mat erial Type (General), Material Instance (Per Instance)
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

	// If sampler is used as constant;
	//	DX12 limits bordercolor to be vec4(0), vec4(0,0,0,1) and vec4(1)
	result_tgfx (*Create_Sampler)
	(
		unsigned int MinMipLevel, unsigned int MaxMipLevel,
		texture_mipmapfilter_tgfx minFilter, texture_mipmapfilter_tgfx magFilter,
		texture_wrapping_tgfx wrapWidth, texture_wrapping_tgfx wrapHeight,
		texture_wrapping_tgfx wrapDepth, uvec4_tgfx bordercolor,
		samplingType_tgfxhnd* hnd
	);

	//Attributes list should end with UNDEFINED (one extra element at the end)
	result_tgfx (*Create_VertexAttributeLayout)
	(
		const datatype_tgfx* attributes, vertexlisttypes_tgfx listType, 
		vertexAttributeLayout_tgfxhnd* hnd
	);
	void (*Delete_VertexAttributeLayout)(vertexAttributeLayout_tgfxhnd hnd);

	result_tgfx (*Upload_toBuffer)
	(buffer_tgfxhnd hnd, const void* data, unsigned int size, unsigned int offst);
	//	If your GPU supports buffer_GPUaddress_pointers, you can use this.
	//		Otherwise this function pointer is NULL
	//	You can pass pointers to buffers (call buffer data or classic GPU buffers)
	//		and use complex data structures and access strategies in shaders
	result_tgfx(*GetBufferGPUPointer)(buffer_tgfxhnd hd, unsigned long long* ptr);


	/*
	* You should sort your vertex data according to attribute layout
	* VertexCount shouldn't be 0
	*/
	result_tgfx (*Create_VertexBuffer)
	(
		vertexAttributeLayout_tgfxhnd VertexAttributeLayoutHandle,
		unsigned int VertexCount, memoryAllocator_tgfxlsthnd allocatorList,
		buffer_tgfxhnd* VertexBufferHandle
	);
	void (*Unload_VertexBuffer)(buffer_tgfxhnd BufferHandle);

	result_tgfx (*Create_IndexBuffer)
	(
		datatype_tgfx DataType, unsigned int IndexCount,
		memoryAllocator_tgfxlsthnd allocatorList, buffer_tgfxhnd* IndexBufferHandle
	);
	void (*Unload_IndexBuffer)(buffer_tgfxhnd BufferHandle);

	result_tgfx (*Create_Texture)
	(
		texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT,
		textureChannels_tgfx CHANNEL_TYPE, unsigned char MIPCOUNT,
		textureUsageFlag_tgfxhnd USAGE, textureOrder_tgfx DATAORDER,
		memoryAllocator_tgfxlsthnd allocatorList, texture_tgfxhnd* TextureHandle
	);
	//TARGET OFFSET is the offset in the texture's buffer to copy to
	result_tgfx (*Upload_Texture)
	(
		texture_tgfxhnd TextureHandle, const void* InputData, unsigned int DataSize,
		unsigned int TargetOffset
	);
	void (*Delete_Texture)(texture_tgfxhnd TEXTUREHANDLE);

	//Extension can be UNIFORMBUFFER
	result_tgfx (*Create_GlobalBuffer)
	(
		const char* BUFFER_NAME, unsigned int DATA_SIZE,
		memoryAllocator_tgfxlsthnd allocatorList, extension_tgfxlsthnd exts,
		buffer_tgfxhnd* GlobalBufferHandle
	);
	void (*Unload_GlobalBuffer)(buffer_tgfxhnd BUFFER_ID);

	//If descType is sampler, SttcSmplrs can be used at binding index 0
	//If your GPU supports VariableDescCount, you set ElementCount to UINT32_MAX
	result_tgfx (*Create_BindingTable)
	(
		shaderdescriptortype_tgfx DescriptorType, unsigned int ElementCount, 
		shaderStageFlag_tgfxhnd VisibleStages, samplingType_tgfxlsthnd SttcSmplrs,
		bindingTable_tgfxhnd* bindingTableHandle
	);
	//Set a descriptor created with shaderdescriptortype_tgfx_SAMPLER
	void (*SetDescriptor_Sampler)
	(
		bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
		samplingType_tgfxhnd samplerHandle
	);
	//Set a descriptor created with shaderdescriptortype_tgfx_BUFFER
	void (*SetDescriptor_Buffer)
	(
		bindingTable_tgfxhnd bindingtable, unsigned int elementIndex, 
		buffer_tgfxhnd bufferHandle, unsigned int bufferOffset,
		unsigned int BoundDataSize, extension_tgfxlsthnd exts
	);
	//Set a descriptor created with shaderdescriptortype_tgfx_SAMPLEDTEXTURE
	void (*SetDescriptor_SampledTexture)
	(
		bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
		texture_tgfxhnd textureHandle
	);
	//Set a descriptor created with shaderdescriptortype_tgfx_STORAGEIMAGE
	void (*SetDescriptor_StorageImage)
	(
		bindingTable_tgfxhnd bindingtable, unsigned int elementIndex,
		texture_tgfxhnd textureHandle
	);

	result_tgfx (*Compile_ShaderSource)
	(
		shaderlanguages_tgfx language, shaderstage_tgfx shaderstage, void* DATA,
		unsigned int DATA_SIZE, shaderSource_tgfxhnd* ShaderSourceHandle
	);
	void (*Delete_ShaderSource)(shaderSource_tgfxhnd ShaderSourceHandle);
	//DX12;		You should use first slots for InstTables, then for TypeTables
	//Vulkan;	You should use first slots for TypeTables, then for InstTables
	result_tgfx (*Link_MaterialType)
	(
		shadersource_tgfx_listhandle ShaderSourcsLst,
		bindingTable_tgfxlsthnd typeTables, bindingTable_tgfxlsthnd instanceTables,
		vertexAttributeLayout_tgfxhnd AttribLayout, renderSubPass_tgfxhnd subpass,
		cullmode_tgfx clling, polygonmode_tgfx plgnmode, depthsettings_tgfxhnd test,
		stencilcnfg_tgfxnd StencilFrontFaced, stencilcnfg_tgfxnd StencilBackFaced,
		blendingInfo_tgfxlsthnd BLENDINGs, unsigned char CallBufferSize,
		rasterPipelineType_tgfxhnd* MaterialHandle
	);
	void (*Delete_MaterialType)(rasterPipelineType_tgfxhnd ID);
	result_tgfx (*Create_MaterialInst)
	(
		rasterPipelineType_tgfxhnd rstrpipetype, bindingTable_tgfxlsthnd instcList,
		rasterPipelineInstance_tgfxhnd *MaterialInstHandle
	);
	void (*Delete_MaterialInst)(rasterPipelineInstance_tgfxhnd ID);
		
	result_tgfx (*Create_ComputeType)
	(
		shaderSource_tgfxhnd Source, bindingTable_tgfxlsthnd TypeBindingTables,
		bindingTable_tgfxlsthnd instancelst, unsigned char isCallBufferSupported,
		computeShaderType_tgfxhnd* ComputeTypeHandle
	);
	result_tgfx (*Create_ComputeInstance)
	(
		computeShaderType_tgfxhnd computeType, bindingTable_tgfxlsthnd instanceList,
		computeShaderInstance_tgfxhnd* ComputeShaderInstanceHandle
	);
	void (*Delete_ComputeShaderType)(computeShaderType_tgfxhnd ID);
	void (*Delete_ComputeShaderInstance)(computeShaderInstance_tgfxhnd ID);

	result_tgfx (*SetBindingTable_Texture)
	(
		bindingTable_tgfxhnd table, unsigned int BINDINDEX, texture_tgfxhnd texture,
		unsigned char isSampledTexture
	);
	result_tgfx (*SetBindingTable_Buffer)
	(
		bindingTable_tgfxhnd table, unsigned int BINDINDEX, buffer_tgfxhnd buffer,
		unsigned int offset, unsigned int size, extension_tgfxlsthnd exts
	);



	result_tgfx (*Create_RTSlotset)
		(RTSlotDescription_tgfxlsthnd descriptions, RTSlotset_tgfxhnd* slotsetHnd);
	void (*Delete_RTSlotSet)(RTSlotset_tgfxhnd RTSlotSetHandle);
	// Changes on RTSlots only happens at the frame slot is gonna be used
	result_tgfx (*Change_RTSlotTexture)
	(
		RTSlotset_tgfxhnd SlotSetHnd, unsigned char isColor, unsigned char SlotIndx,
		unsigned char FrameIndex, texture_tgfxhnd TextureHandle
	);
	result_tgfx (*Inherite_RTSlotSet)
	(
		RTSlotUsage_tgfxlsthnd descs, RTSlotset_tgfxhnd RTSlotSetHandle,
		inheritedRTSlotset_tgfxhnd* InheritedSlotSetHandle
	);
	void (*Delete_InheritedRTSlotSet)(inheritedRTSlotset_tgfxhnd hnd);
} gpudatamanager_tgfx;