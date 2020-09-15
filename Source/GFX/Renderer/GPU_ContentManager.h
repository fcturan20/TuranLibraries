#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_ENUMs.h"
#include "GFX/GFX_FileSystem/Resource_Type/Texture_Resource.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"
#include "GFX_Resource.h"


namespace GFX_API {

	/*RenderingComponents aren't supported here because every RenderGraph will define how it handles RenderComponents
	Nonetheless, the RenderingComponent's data should be in GPU so we should provide functions to send a buffer to GPU here

	All resources (except Meshes) are coupled with their GPU representations in responsible GFX_API's classes.
	That means, you can use your Asset_ID in GFX operations! If you didn't understand, you can go to OpenGL4.
	But because there is no concept of Mesh in GFX, you have to use Upload_MeshBuffer's returned ID in GFX operations.
	
	When Material Types are compiled, they're written to disk! So you should use Delete_MaterialType when you don't need a MaterialType anymore!
	I'd like to keep this process transparent and give the control to the user but the scope of the project is high enough to not allow this.
	I didn't want to support recompile, because this causes an unnecessary complex coupling between MaterialType-StoredBinaryData-FileSystem-GPUContentManager
	If user wants to recompile all the Material Types, he can delete all of them and compile again.
	*/
	class GFXAPI GPU_ContentManager {
	protected:
		vector<GFX_Texture> TEXTUREs;
		vector<GFX_GlobalBuffer> GLOBALBUFFERs;
		vector<GFX_ShaderSource> SHADERSOURCEs;
		vector<GFX_ShaderProgram> SHADERPROGRAMs;
		vector<GFX_ComputeShader> COMPUTESHADERs;
		vector<VertexAttribute> VERTEXATTRIBUTEs;
		vector<VertexAttributeLayout> VERTEXATTRIBLAYOUTs;
		vector<RT_SLOTSET> RT_SLOTSETs;
		vector<TransferPacket> TRANSFER_PACKETs;


		Bitset GLOBALBUFFERID_BITSET;
		unsigned int Create_GlobalBufferID();
		void Delete_GlobalBufferID(unsigned int ID);

		Bitset VertexAttributeID_BITSET;
		unsigned int Create_VertexAttributeID();
		void Delete_VertexAttributeID(unsigned int ID);

		Bitset VertexAttribLayoutID_BITSET;
		unsigned int Create_VertexAttribLayoutID();
		void Delete_VertexAttribLayoutID(unsigned int ID);

		Bitset RTSLOTSETID_BITSET;
		unsigned int Create_RTSLOTSETID();
		void Delete_RTSLOTSETID(unsigned int ID);

		Bitset TRANSFERPACKETID_BITSET;
		unsigned int Create_TransferPacketID();
		void Delete_TransferPacketID(unsigned int ID);
		GFX_API::TransferPacket* Find_TransferPacket_byID(unsigned int PacketID, unsigned int* vector_index = nullptr);

		void Add_to_TransferPacket(unsigned int TransferPacketID, void* TransferHandle);
	public:
		GPU_ContentManager();
		virtual ~GPU_ContentManager();

		virtual void Unload_AllResources() = 0;

		//Registers Vertex Attribute to the system and gives it an ID
		unsigned int Create_VertexAttribute(DATA_TYPE TYPE, bool is_perVertex);
		//Returns true if operation is successful
		virtual bool Delete_VertexAttribute(unsigned int Attribute_ID) = 0;
		virtual unsigned int Create_VertexAttributeLayout(vector<unsigned int> Attributes) = 0;
		virtual void Delete_VertexAttributeLayout(unsigned int Layout_ID) = 0;

		//Return MeshBufferID to use in Draw Calls
		virtual unsigned int Upload_MeshBuffer(unsigned int attributelayout, const void* vertex_data, unsigned int data_size, unsigned int vertex_count, 
			const void* index_data, unsigned int index_count) = 0;
		//When you call this function, Draw Calls that uses this ID may draw another Mesh or crash
		virtual void Unload_MeshBuffer(unsigned int MeshBuffer_ID) = 0;

		virtual unsigned int Upload_PointBuffer(const void* point_data, unsigned int data_size, unsigned int point_count) = 0;
		virtual unsigned int CreatePointBuffer_fromMeshBuffer(unsigned int MeshBuffer_ID, unsigned int AttributeIndex_toUseAsPointBuffer) = 0;
		virtual void Unload_PointBuffer(unsigned int PointBuffer_ID) = 0;

		//Create a Texture Buffer and upload if texture's GPUREADONLY_CPUEXISTENCE
		virtual void Create_Texture(Texture_Resource* TEXTURE_ASSET, unsigned int Asset_ID, unsigned int TransferPacketID) = 0;
		virtual void Upload_Texture(unsigned int Asset_ID, void* DATA, unsigned int DATA_SIZE, unsigned int TransferPacketID) = 0;
		virtual void Unload_Texture(unsigned int ASSET_ID) = 0;


		virtual unsigned int Create_GlobalBuffer(const char* BUFFER_NAME, void* DATA, unsigned int DATA_SIZE, BUFFER_VISIBILITY USAGE) = 0;
		//If you want to upload the data, but data's pointer didn't change since the last time (Creation or Re-Upload) you can use nullptr!
		//Also if the data's size isn't changed since the last time, you can pass as 0.
		//If DATA isn't nullptr, old data that buffer holds will be deleted!
		virtual void Upload_GlobalBuffer(unsigned int BUFFER_ID, void* DATA = nullptr, unsigned int DATA_SIZE = 0) = 0;
		virtual void Unload_GlobalBuffer(unsigned int BUFFER_ID) = 0;


		virtual void Compile_ShaderSource(ShaderSource_Resource* SHADER, unsigned int Asset_ID, string* compilation_status) = 0;
		virtual void Delete_ShaderSource(unsigned int Shader_ID) = 0;
		virtual void Compile_ComputeShader(GFX_API::ComputeShader_Resource* SHADER, unsigned int Asset_ID, string* compilation_status) = 0;
		virtual void Delete_ComputeShader(unsigned int ASSET_ID) = 0;
		virtual void Link_MaterialType(Material_Type* MATTYPE_ASSET, unsigned int Asset_ID) = 0;
		virtual void Delete_MaterialType(unsigned int Asset_ID) = 0;
		virtual void Create_MaterialInst(Material_Instance* MATINST_ASSET, unsigned int Asset_ID) = 0;
		virtual void Delete_MaterialInst(unsigned int Asset_ID) = 0;


		virtual unsigned int Create_RTSlotSet(unsigned int* RTIDs, unsigned int* SlotIndexes, ATTACHMENT_READSTATE* ReadTypes, 
			RT_ATTACHMENTs* AttachmentTypes, OPERATION_TYPE* OperationsTypes, unsigned int SlotCount) = 0;
	};
}