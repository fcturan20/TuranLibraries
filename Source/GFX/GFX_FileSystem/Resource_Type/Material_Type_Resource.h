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


	/*	This structure is a Material Type specific buffer:
	* In Vulkan, this structure represents a descriptor. BINDINGPOINT and in-shader buffer binding should match. 
	If there is no general shader input, per instance shader inputs should use Descriptor Set 1.
	If there is general shader input, general shader input should use Descriptor Set 1 and per instance shader inputs should use Descriptor Set 2
	* In OpenGL, this structure's NAME should match with in-shader buffer's name (You shouldn't set binding in shader!). At linking, ContentManager finds BINDINGPOINT and stores it
	*/
	struct GFXAPI ShaderInput_Description {
		unsigned int BINDINGPOINT = 0, ELEMENTCOUNT = 0;
		string NAME;
		SHADERSTAGEs_FLAG SHADERSTAGEs;
		SHADERINPUT_TYPE TYPE;
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
		vector<ShaderInput_Description> MATERIALTYPEDATA;
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
