#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Texture_Resource.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"
#include "GFX_RenderNodes.h"


namespace GFX_API {

	/* Specification:
	1) This system has a VRAM allocation system
	2) You can't store resources in Staging memory, only GPU Local memory is allowed!
	3) You can't know recent left GPU Local Memory, you can only query last frame's left GPU Local Memory
	4) If you upload a resource and it fits Staging Memory but not the GPU Local Memory, GFX API will fail in GFXRenderer->Run()
	*/
	class GFXAPI GPU_ContentManager {
	protected:
	public:
		GPU_ContentManager();
		virtual ~GPU_ContentManager();

		virtual void Unload_AllResources() = 0;

		//Registers Vertex Attribute to the system and gives it an ID
		virtual GFXHandle Create_VertexAttribute(GFX_API::DATA_TYPE TYPE, bool is_perVertex) = 0;
		//Returns true if operation is successful
		virtual bool Delete_VertexAttribute(GFXHandle Attribute_ID) = 0;
		/*Attributes are ordered as the same order of input vector
		* For example: Same attribute ID may have different location/order in another attribute layout
		* So you should gather your mesh buffer data according to that
		*/
		virtual GFXHandle Create_VertexAttributeLayout(const vector<GFXHandle>& Attributes) = 0;
		virtual void Delete_VertexAttributeLayout(GFXHandle Layout_ID) = 0;

		/*
		* Return MeshBufferID to use in Draw Calls
		* You should order your vertex data according to attribute layout, don't forget that
		* For now, Mesh Buffers only support index data as unsigned integer
		* If index_data doesn't point to a buffer that's (4 * index_count) size, then it's a Undefined Behaviour
		* Vertex_count shouldn't be 0
		* If you want to use indexed mesh buffer, index_count shouldn't be 0
		* Buffer Create Rule
		1) vertex_data and index_data: valid, index_count != 0 -> Both Vertex and Index Data sent to Staging Buffer if there is enough space, Indexed Rendering: Supported
		2) vertex_data and index_data: valid, index_count == 0 -> Only Vertex Data sent to Staging Buffer if there is enough space, Indexed Rendering: Not Supported
		3) vertex_data or index_data: invalid, index_count != 0 -> No data sent, just buffer object is created. Indexed Rendering: Supported
		4) vertex_data: valid, index_data: invalid, index_count == 0 -> Only Vertex Data sent to Staging Buffer if there is enough space, Indexed Rendering: Not Supported
		*/
		virtual GFXHandle Create_MeshBuffer(GFXHandle attributelayout, const void* vertex_data, unsigned int vertex_count,
			const unsigned int* index_data, unsigned int index_count, GFXHandle TransferPassHandle) = 0;
		virtual void Upload_MeshBuffer(GFXHandle MeshBufferHandle, const void* vertex_data, const void* index_data) = 0;
		//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
		virtual void Unload_MeshBuffer(GFXHandle MeshBuffer_ID) = 0;

		//This function only creates a texture object, you should upload its data with Upload_Texture() or 
		//change its layout with Renderer->Barrier_Texture() if its data is GPU side only and unknown by the application
		virtual GFXHandle Create_Texture(const Texture_Description& TEXTURE_ASSET, GFXHandle TransferPassID) = 0;
		virtual void Upload_Texture(GFXHandle TEXTUREHANDLE, void* DATA, unsigned int DATA_SIZE) = 0;
		virtual void Unload_Texture(GFXHandle TEXTUREHANDLE) = 0;


		//Return handle to reference in GFX!
		virtual GFXHandle Create_GlobalBuffer(const char* BUFFER_NAME, void* DATA, unsigned int DATA_SIZE, BUFFER_VISIBILITY USAGE) = 0;
		//If you want to upload the data, but data's pointer didn't change since the last time (Creation or Re-Upload) you can use nullptr!
		//Also if the data's size isn't changed since the last time, you can pass as 0.
		//If DATA isn't nullptr, old data that buffer holds will be deleted!
		virtual void Upload_GlobalBuffer(GFXHandle BUFFER_ID, void* DATA = nullptr, unsigned int DATA_SIZE = 0) = 0;
		virtual void Unload_GlobalBuffer(GFXHandle BUFFER_ID) = 0;

		//Return handle to reference in GFX!
		virtual GFXHandle Compile_ShaderSource(ShaderSource_Resource* SHADER) = 0;
		virtual void Delete_ShaderSource(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual GFXHandle Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER) = 0;
		virtual void Delete_ComputeShader(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual GFXHandle Link_MaterialType(Material_Type* MATTYPE_ASSET) = 0;
		virtual void Delete_MaterialType(GFXHandle ID) = 0;
		//Return handle to reference in GFX!
		virtual GFXHandle Create_MaterialInst(Material_Instance* MATINST_ASSET) = 0;
		virtual void Delete_MaterialInst(GFXHandle ID) = 0;


		virtual GFXHandle Create_RTSlotSet(const vector<GFX_API::RTSLOT_Description>& Descriptions) = 0;
		virtual GFXHandle Inherite_RTSlotSet(const GFX_API::GFXHandle SLOTSETHandle, const vector<GFX_API::IRTSLOT_Description>& Descriptions) = 0;
	};
}