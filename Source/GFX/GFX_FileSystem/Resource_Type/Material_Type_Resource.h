#pragma once
#include "GFX/GFX_Includes.h"

enum class RESOURCETYPEs : char;

namespace GFX_API {

	//This class doesn't support Compute, only the Rasterization stages
	class GFXAPI ShaderSource_Resource {
	public:
		ShaderSource_Resource();
		SHADERSTAGEs_FLAG STAGE;
		void* SOURCE_DATA;
		unsigned int DATA_SIZE;
		SHADER_LANGUAGEs LANGUAGE;
	};
	
	/*
		1) ACCESS_TYPE defines the texture's binding way
		2) OP_TYPE defines why shader accesses the texture. 
	If a texture is READ_WRITE but you just read it in the shader, you can set as READ_ONLY
		3) TEXTURE_ID is texture's asset ID. But render targets (framebuffer attachments) aren't assets.
	
	struct GFXAPI Texture_Access {
		TEXTURE_DIMENSIONs DIMENSIONs;
		TEXTURE_CHANNELs CHANNELs;
		TEXTURE_ACCESS ACCESS_TYPE;
		unsigned int BINDING_POINT;		
		unsigned int TEXTURE_ID;
	};*/



	/*	This structure is a Material Type specific buffer:
	* In Vulkan, this structure represents Descriptor Set 1 and BINDINGPOINT and in-shader buffer binding should match (in-shader buffer should use Set 1)
	* In OpenGL, this structure's NAME should match with in-shader buffer's name (You shouldn't set binding in shader!). At linking, ContentManager finds BINDINGPOINT and stores it
	*/
	struct GFXAPI MaterialDataDescriptor {
		unsigned int DATA_SIZE = 0, BINDINGPOINT = 0, ELEMENTCOUNT = 0;
		string NAME;
		SHADERSTAGEs_FLAG SHADERSTAGEs;
		MATERIALDATA_TYPE TYPE;
	};

	/* Used to specify the material instance specific data! The data responsibility's in your hands!
	* This data is invalid if BINDINGPOINT isn't same with BINDINGPOINT of any MaterialDataDescriptor in the Material_Type
	*/
	struct GFXAPI MaterialInstanceData {
		void* BINDINGPOINT;
		void* DATA_GFXID;
	};


	class GFXAPI Material_Type {
	public:
		Material_Type();


		GFXHandle VERTEXSOURCE_ID, FRAGMENTSOURCE_ID, ATTRIBUTELAYOUT_ID, SubDrawPass_ID;
		vector<MaterialDataDescriptor> MATERIALTYPEDATA;
		CULL_MODE culling;
		POLYGON_MODE polygon;
		DEPTH_TESTs depthtest;
		DEPTH_MODEs depthmode;
		STENCIL_SETTINGS frontfacedstencil, backfacedstencil;
		//These infos should be set per slot of the used RTSlotSet
		//For attachments that aren't in the list below, blending is off
		//Different constant values for different RTSlots is not supported
		//Only Element 0's Constant Color in this vector is used
		vector<ATTACHMENT_BLENDING> BLENDINGINFOS;
		
		float LINEWIDTH = 1.0f;
	};

	
	class GFXAPI ComputeShader_Resource {
	public:
		SHADER_LANGUAGEs LANGUAGE;
		string SOURCE_CODE;
	};

	class GFXAPI ComputeShader_Instance {
	public:
		unsigned int ComputeShader;
	};
}
