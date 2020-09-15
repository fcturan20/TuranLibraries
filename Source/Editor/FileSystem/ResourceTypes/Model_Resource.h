#pragma once
#include "Editor/Editor_Includes.h"
#include "GFX/Renderer/GPU_ContentManager.h"

enum class RESOURCETYPEs : char;

namespace TuranEditor {
	struct Static_Mesh_Data {
	public:
		Static_Mesh_Data();
		~Static_Mesh_Data();

		void* VERTEX_DATA = nullptr;
		unsigned int VERTEXDATA_SIZE, VERTEX_NUMBER = 0;
		unsigned int* INDEX_DATA = nullptr;
		unsigned int INDICES_NUMBER = 0;
		unsigned short Material_Index = 0;

		GFX_API::VertexAttributeLayout DataLayout;

		bool Verify_Mesh_Data();
	};


	class Static_Model {
	public:
		Static_Model();
		~Static_Model();

		//Meshes will be stored as pointers in an array, so point to that "Pointer Array"
		vector<Static_Mesh_Data*> MESHes;
		vector<unsigned int> MATINST_IDs;
		unsigned int* WORLD_ID = nullptr;

		//Return vector contains MeshBufferIDs of the Model!
		vector<unsigned int> Upload_toGPU();
	};
}