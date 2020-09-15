#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Texture_Resource.h"

/*
	All resources have GL_ID variable. This variable is a pointer to GL specific structure that's used by GL functions (For example; unsigned ints for Textures, Buffers etc. in OpenGL)
	
*/



namespace GFX_API {

	/* Vertex Attribute Layout Specification:
			All vertex attributes should be interleaved because there is no easy way in Vulkan for de-interleaved path.
			Vertex Attributes are created as seperate objects because this helps debug visualization of the data
			Vertex Attributes are goona be ordered by VertexAttributeLayout's vector elements order and this also defines attribute's in-shader location
			That means if position attribute is second element in "Attributes" vector, MaterialType that using this layout uses position attribute at location = 1 instead of 0.

	*/

	//There are datas in VertexAttribute but no one should access it except GPUContentManager!
	//I want to support InstanceAttribute too but not now because I don't know Vulkan yet
	struct GFXAPI VertexAttribute {
	public:
		unsigned char ID;
		DATA_TYPE DATATYPE;
	};

	struct GFXAPI VertexAttributeLayout {
	public:
		unsigned int ID;
		void* GL_ID;
		VertexAttribute** Attributes;
		unsigned int Attribute_Number, size_perVertex;
	};


	/*
	* 
	*/
	struct GFXAPI GFX_GlobalBuffer {
		unsigned int ID;
		void* GL_ID;
		unsigned int DATA_SIZE, BINDINGPOINT;
		BUFFER_VISIBILITY VISIBILITY;
		SHADERSTAGEs_FLAG ACCESSED_STAGEs;
	};


	struct GFXAPI GFX_Texture {
		unsigned int ASSET_ID, WIDTH, HEIGHT, DATA_SIZE;
		void* GL_ID;
		TEXTURE_CHANNELs CHANNELs;
		TEXTURE_TYPEs TYPE;
		BUFFER_VISIBILITY BUF_VIS;
	};

	//If this slot isn't used, then RT_READTYPE should be UNUSED
	//If you inherite this for a subpass or a material type, ATTACHMENT_TYPE is ignored
	struct GFXAPI RT_SLOT {
		unsigned int SLOT_Index;
		GFX_Texture* RT;
		ATTACHMENT_READSTATE RT_READTYPE;
		RT_ATTACHMENTs ATTACHMENT_TYPE;
		OPERATION_TYPE RT_OPERATIONTYPE;
	};

	struct GFXAPI RT_SLOTSET {
		unsigned int ID;
		RT_SLOT** SLOTs;
		unsigned char SLOT_COUNT;
		void* GL_ID;
	};


	struct GFXAPI TransferPacket {
		unsigned int ID;
		BUFFER_TYPE PACKETLIST_TYPE;
		vector<void*> Packet;
	};



	//This represents Rasterization Shader Stages, not Compute
	struct GFXAPI GFX_ShaderSource {
		unsigned int ASSET_ID;
		void* GL_ID;
	};


	struct GFXAPI GFX_ShaderProgram {
		unsigned int ASSET_ID;
		void* GL_ID;
	};

	struct GFXAPI GFX_ComputeShader {
		unsigned int ASSET_ID;
		void* GL_ID;
	};

	struct GFXAPI RenderState {
		DEPTH_MODEs DEPTH_MODE;
		DEPTH_TESTs DEPTH_TEST_FUNC;
		//There should be Stencil and Blending options, but for now leave it like this
	};
}