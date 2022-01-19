#include "gpucontentmanager.h"
#include <tgfx_core.h>
#include <tgfx_gpucontentmanager.h>

void  Destroy_AllResources (){}


result_tgfx Create_SamplingType (unsigned int MinimumMipLevel, unsigned int MaximumMipLevel,
	texture_mipmapfilter_tgfx MINFILTER, texture_mipmapfilter_tgfx MAGFILTER, texture_wrapping_tgfx WRAPPING_WIDTH,
	texture_wrapping_tgfx WRAPPING_HEIGHT, texture_wrapping_tgfx WRAPPING_DEPTH, samplingtype_tgfx_handle* SamplingTypeHandle){
	return result_tgfx_FAIL;
}

/*Attributes are ordered as the same order of input vector
* For example: Same attribute ID may have different location/order in another attribute layout
* So you should gather your vertex buffer data according to that
*/

result_tgfx Create_VertexAttributeLayout (const datatype_tgfx* Attributes, vertexlisttypes_tgfx listtype,
	vertexattributelayout_tgfx_handle* VertexAttributeLayoutHandle){
	return result_tgfx_FAIL;
}
void  Delete_VertexAttributeLayout (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle){}

result_tgfx Upload_toBuffer (buffer_tgfx_handle Handle, buffertype_tgfx Type, const void* DATA, unsigned int DATA_SIZE, unsigned int OFFSET){ return result_tgfx_FAIL; }

result_tgfx Create_StagingBuffer (unsigned int DATASIZE, unsigned int MemoryTypeIndex, buffer_tgfx_handle* Handle){ return result_tgfx_FAIL; }
void  Delete_StagingBuffer (buffer_tgfx_handle StagingBufferHandle){}
/*
* You should sort your vertex data according to attribute layout, don't forget that
* VertexCount shouldn't be 0
*/
result_tgfx Create_VertexBuffer (vertexattributelayout_tgfx_handle VertexAttributeLayoutHandle, unsigned int VertexCount,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* VertexBufferHandle){
	return result_tgfx_FAIL;
}
void  Unload_VertexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_IndexBuffer (datatype_tgfx DataType, unsigned int IndexCount, unsigned int MemoryTypeIndex, buffer_tgfx_handle* IndexBufferHandle){ return result_tgfx_FAIL; }
void  Unload_IndexBuffer (buffer_tgfx_handle BufferHandle){}

result_tgfx Create_Texture (texture_dimensions_tgfx DIMENSION, unsigned int WIDTH, unsigned int HEIGHT, texture_channels_tgfx CHANNEL_TYPE,
	unsigned char MIPCOUNT, textureusageflag_tgfx_handle USAGE, texture_order_tgfx DATAORDER, unsigned int MemoryTypeIndex, texture_tgfx_handle* TextureHandle){
	return result_tgfx_FAIL;
}
//TARGET OFFSET is the offset in the texture's buffer to copy to
result_tgfx Upload_Texture (texture_tgfx_handle TextureHandle, const void* InputData, unsigned int DataSize, unsigned int TargetOffset){ return result_tgfx_FAIL; }
void  Delete_Texture (texture_tgfx_handle TEXTUREHANDLE, unsigned char isUsedLastFrame){}

result_tgfx Create_GlobalBuffer (const char* BUFFER_NAME, unsigned int DATA_SIZE, unsigned char isUniform,
	unsigned int MemoryTypeIndex, buffer_tgfx_handle* GlobalBufferHandle) {
	return result_tgfx_FAIL;
}
void  Unload_GlobalBuffer (buffer_tgfx_handle BUFFER_ID){}
result_tgfx SetGlobalShaderInput_Buffer (unsigned char isUniformBuffer, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	buffer_tgfx_handle BufferHandle, unsigned int BufferOffset, unsigned int BoundDataSize) {
	return result_tgfx_FAIL;
}
result_tgfx SetGlobalShaderInput_Texture (unsigned char isSampledTexture, unsigned int ElementIndex, unsigned char isUsedLastFrame,
	texture_tgfx_handle TextureHandle, samplingtype_tgfx_handle sampler, image_access_tgfx access) { return result_tgfx_FAIL;}


result_tgfx Compile_ShaderSource (shaderlanguages_tgfx language, shaderstage_tgfx shaderstage, void* DATA, unsigned int DATA_SIZE,
	shadersource_tgfx_handle* ShaderSourceHandle) { return result_tgfx_FAIL;}
void  Delete_ShaderSource (shadersource_tgfx_handle ShaderSourceHandle){}
result_tgfx Link_MaterialType (shadersource_tgfx_listhandle ShaderSourcesList, shaderinputdescription_tgfx_listhandle ShaderInputDescs,
	vertexattributelayout_tgfx_handle AttributeLayout, subdrawpass_tgfx_handle Subdrawpass, cullmode_tgfx culling,
	polygonmode_tgfx polygonmode, depthsettings_tgfx_handle depthtest, stencilsettings_tgfx_handle StencilFrontFaced,
	stencilsettings_tgfx_handle StencilBackFaced, blendinginfo_tgfx_listhandle BLENDINGs, rasterpipelinetype_tgfx_handle* MaterialHandle){ return result_tgfx_FAIL;}
void  Delete_MaterialType (rasterpipelinetype_tgfx_handle ID){}
result_tgfx Create_MaterialInst (rasterpipelinetype_tgfx_handle MaterialType, rasterpipelineinstance_tgfx_handle* MaterialInstHandle){ return result_tgfx_FAIL; }
void  Delete_MaterialInst (rasterpipelineinstance_tgfx_handle ID){}

result_tgfx Create_ComputeType (shadersource_tgfx_handle Source, shaderinputdescription_tgfx_listhandle ShaderInputDescs, computeshadertype_tgfx_handle* ComputeTypeHandle){ return result_tgfx_FAIL; }
result_tgfx Create_ComputeInstance(computeshadertype_tgfx_handle ComputeType, computeshaderinstance_tgfx_handle* ComputeShaderInstanceHandle) { return result_tgfx_FAIL; }
void  Delete_ComputeShaderType (computeshadertype_tgfx_handle ID){}
void  Delete_ComputeShaderInstance (computeshaderinstance_tgfx_handle ID){}

//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetMaterialType_Buffer (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetMaterialType_Texture (rasterpipelinetype_tgfx_handle MaterialType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}

result_tgfx SetMaterialInst_Buffer (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle BufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetMaterialInst_Texture (rasterpipelineinstance_tgfx_handle MaterialInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}
//IsUsedRecently means is the material type/instance used in last frame. This is necessary for Vulkan synchronization process.
result_tgfx SetComputeType_Buffer (computeshadertype_tgfx_handle ComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetComputeType_Texture (computeshadertype_tgfx_handle ComputeType, unsigned char isComputeType, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}

result_tgfx SetComputeInst_Buffer (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	buffer_tgfx_handle TargetBufferHandle, buffertype_tgfx BUFTYPE, unsigned char isUniformBufferShaderInput, unsigned int ELEMENTINDEX, unsigned int TargetOffset, unsigned int BoundDataSize){
	return result_tgfx_FAIL;
}
result_tgfx SetComputeInst_Texture (computeshaderinstance_tgfx_handle ComputeInstance, unsigned char isUsedRecently, unsigned int BINDINDEX,
	texture_tgfx_handle TextureHandle, unsigned char isSampledTexture, unsigned int ELEMENTINDEX, samplingtype_tgfx_handle Sampler, image_access_tgfx usage){
	return result_tgfx_FAIL;
}


result_tgfx Create_RTSlotset (rtslotdescription_tgfx_listhandle Descriptions, rtslotset_tgfx_handle* RTSlotSetHandle){ return result_tgfx_FAIL; }
void  Delete_RTSlotSet (rtslotset_tgfx_handle RTSlotSetHandle){}
//Changes on RTSlots only happens at the frame slot is gonna be used
//For example; if you change next frame's slot, necessary API calls are gonna be called next frame
//For example; if you change slot but related slotset isn't used by drawpass, it doesn't happen until it is used
result_tgfx Change_RTSlotTexture (rtslotset_tgfx_handle RTSlotHandle, unsigned char isColorRT, unsigned char SlotIndex, unsigned char FrameIndex, texture_tgfx_handle TextureHandle){ return result_tgfx_FAIL; }
result_tgfx Inherite_RTSlotSet (rtslotusage_tgfx_listhandle Descriptions, rtslotset_tgfx_handle RTSlotSetHandle, inheritedrtslotset_tgfx_handle* InheritedSlotSetHandle){ return result_tgfx_FAIL; }
void  Delete_InheritedRTSlotSet (inheritedrtslotset_tgfx_handle InheritedRTSlotSetHandle){}
struct gpudatamanager_private {

};
inline void set_functionpointers() {
	core_tgfx_main->contentmanager->Change_RTSlotTexture = Change_RTSlotTexture;
	core_tgfx_main->contentmanager->Compile_ShaderSource = Compile_ShaderSource;
	core_tgfx_main->contentmanager->Create_ComputeInstance = Create_ComputeInstance;
	core_tgfx_main->contentmanager->Create_ComputeType = Create_ComputeType;
	core_tgfx_main->contentmanager->Create_GlobalBuffer = Create_GlobalBuffer;
	core_tgfx_main->contentmanager->Create_IndexBuffer = Create_IndexBuffer;
	core_tgfx_main->contentmanager->Create_MaterialInst = Create_MaterialInst;
	core_tgfx_main->contentmanager->Create_RTSlotset = Create_RTSlotset;
	core_tgfx_main->contentmanager->Create_SamplingType = Create_SamplingType;
	core_tgfx_main->contentmanager->Create_StagingBuffer = Create_StagingBuffer;
	core_tgfx_main->contentmanager->Create_Texture = Create_Texture;
	core_tgfx_main->contentmanager->Create_VertexAttributeLayout = Create_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Create_VertexBuffer = Create_VertexBuffer;
	core_tgfx_main->contentmanager->Delete_ComputeShaderInstance = Delete_ComputeShaderInstance;
	core_tgfx_main->contentmanager->Delete_ComputeShaderType = Delete_ComputeShaderType;
	core_tgfx_main->contentmanager->Delete_InheritedRTSlotSet = Delete_InheritedRTSlotSet;
	core_tgfx_main->contentmanager->Delete_MaterialInst = Delete_MaterialInst;
	core_tgfx_main->contentmanager->Delete_MaterialType = Delete_MaterialType;
	core_tgfx_main->contentmanager->Delete_RTSlotSet = Delete_RTSlotSet;
	core_tgfx_main->contentmanager->Delete_ShaderSource = Delete_ShaderSource;
	core_tgfx_main->contentmanager->Delete_StagingBuffer = Delete_StagingBuffer;
	core_tgfx_main->contentmanager->Delete_Texture = Delete_Texture;
	core_tgfx_main->contentmanager->Delete_VertexAttributeLayout = Delete_VertexAttributeLayout;
	core_tgfx_main->contentmanager->Destroy_AllResources = Destroy_AllResources;
	core_tgfx_main->contentmanager->Inherite_RTSlotSet = Inherite_RTSlotSet;
	core_tgfx_main->contentmanager->Link_MaterialType = Link_MaterialType;
	core_tgfx_main->contentmanager->SetComputeInst_Buffer = SetComputeInst_Buffer;
	core_tgfx_main->contentmanager->SetComputeInst_Texture = SetComputeInst_Texture;
	core_tgfx_main->contentmanager->SetComputeType_Buffer = SetComputeType_Buffer;
	core_tgfx_main->contentmanager->SetComputeType_Texture = SetComputeType_Texture;
	core_tgfx_main->contentmanager->SetGlobalShaderInput_Buffer = SetGlobalShaderInput_Buffer;
	core_tgfx_main->contentmanager->SetGlobalShaderInput_Texture = SetGlobalShaderInput_Texture;
	core_tgfx_main->contentmanager->SetMaterialInst_Buffer = SetMaterialInst_Buffer;
	core_tgfx_main->contentmanager->SetMaterialInst_Texture = SetMaterialInst_Texture;
	core_tgfx_main->contentmanager->SetMaterialType_Buffer = SetMaterialType_Buffer;
	core_tgfx_main->contentmanager->SetMaterialType_Texture = SetMaterialType_Texture;
	core_tgfx_main->contentmanager->Unload_GlobalBuffer = Unload_GlobalBuffer;
	core_tgfx_main->contentmanager->Unload_IndexBuffer = Unload_IndexBuffer;
	core_tgfx_main->contentmanager->Unload_VertexBuffer = Unload_VertexBuffer;
	core_tgfx_main->contentmanager->Upload_Texture = Upload_Texture;
	core_tgfx_main->contentmanager->Upload_toBuffer = Upload_toBuffer;
}
extern void Create_GPUContentManager() {
	contentmanager = new gpudatamanager_public;

	set_functionpointers();

}