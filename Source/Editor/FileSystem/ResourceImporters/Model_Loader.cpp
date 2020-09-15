#include "Model_Loader.h"
#include "Editor/FileSystem/EditorFileSystem_Core.h"

#include "Editor/Editors/Status_Window.h"

//Assimp libraries to load Model
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Editor/FileSystem/ResourceTypes/Model_Resource.h"
#include "Editor/FileSystem/ResourceTypes/ResourceTYPEs.h"
#include "GFX/GFX_Core.h"
#include "Editor/FileSystem/ResourceImporters/Texture_Loader.h"
#include "Editor/RenderContext/Editor_DataManager.h"

using namespace TuranAPI;

namespace TuranEditor {

	struct Attribute_BitMask {
		bool TextCoords : 2;
		bool VertexColors : 2;
		bool Normal : 1;
		bool Tangent : 1;
		bool Bitangent : 1;
		bool Bones : 1;
		Attribute_BitMask() {
			TextCoords = 0;
			VertexColors = 0;
			Normal = false;
			Tangent = false;
			Bitangent = false;
			Bones = false;
		}
	};
	void Find_AvailableAttributes(aiMesh* data, Attribute_BitMask& Available_Attributes);
	void Load_MeshData(const aiMesh* data, Attribute_BitMask& Attribute_Info, Static_Model* Model);
	unsigned int Load_MaterialData(const aiMaterial* data, const char* directory, unsigned int* object_worldid);

	unsigned int Model_Importer::Import_Model(const char* PATH, bool Use_SurfaceMaterial, unsigned int MaterialType_toInstance) {
		//Check if model is available
		Assimp::Importer import;
		const aiScene* Scene = nullptr;
		{
			Scene = import.ReadFile(PATH, aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_Triangulate
			);

			//Check if scene reading errors!
			if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode) {
				std::cout << (("Failed on Loading Mesh with Assimp; " + string(import.GetErrorString())).c_str()) << std::endl;
				return 0;
			}

			if (Scene->mNumMeshes == 0) {
				std::cout << "Failed because there is no mesh in loaded scene!"  << std::endl;
				return 0;
			}
		}

		//Importing process
		Static_Model* Loaded_Model = new Static_Model;
		{
			string compilation_status;


			vector<Attribute_BitMask> aiMesh_Attributes;
			aiMesh_Attributes.resize(Scene->mNumMeshes);
			for (unsigned int i = 0; i < Scene->mNumMeshes; i++) {
				Find_AvailableAttributes(Scene->mMeshes[i], aiMesh_Attributes[i]);
			}


			//Show Attribute Infos
			Attribute_BitMask* bitmask = nullptr;
			{
				for (unsigned int i = 0; i < Scene->mNumMeshes; i++) {
					bitmask = &aiMesh_Attributes[i];
					compilation_status.append("Mesh Index: ");
					compilation_status.append(std::to_string(i).c_str());
					compilation_status.append(" Texture Coordinate Sets Number: ");
					compilation_status.append(std::to_string(bitmask->TextCoords).c_str());
					compilation_status.append(" Vertex Color Sets Number: ");
					compilation_status.append(std::to_string(bitmask->VertexColors).c_str());
					if (bitmask->Normal) {
						compilation_status.append(" Vertex Normals're found\n");
					}
					else {
						compilation_status.append(" Vertex Normals're not found\n");
					}
					if (bitmask->Tangent) {
						compilation_status.append(" Tangents're found\n");
					}
					else {
						compilation_status.append(" Tangents're not found\n");
					}
					if (bitmask->Bitangent) {
						compilation_status.append(" Bitangents're found\n");
					}
					else {
						compilation_status.append(" Bitangents're not found\n");
					}
					if (bitmask->Bones) {
						compilation_status.append(" Bones're found\n");
					}
					else {
						compilation_status.append(" Bones're not found\n");
					}
				}
			}


			for (unsigned int i = 0; i < Scene->mNumMeshes; i++) {
				Load_MeshData(Scene->mMeshes[i], aiMesh_Attributes[i], Loaded_Model);
			}

			//Finalization
			compilation_status.append("Compiled the model successfully!");
			new Status_Window(compilation_status.c_str());
		}

		//If model has materials, create a Folder and load materials etc.
		if (Scene->mNumMaterials) {
			if (Use_SurfaceMaterial) {
				string DIR = PATH;
				DIR = DIR.substr(0, DIR.find_last_of('/'));
				Loaded_Model->MATINST_IDs.resize(Scene->mNumMaterials);
				for (unsigned int i = 0; i < Loaded_Model->MATINST_IDs.size(); i++) {
					aiMaterial* MAT = Scene->mMaterials[i];
					Loaded_Model->MATINST_IDs[i] = Load_MaterialData(MAT, DIR.c_str(), Loaded_Model->WORLD_ID);
				}
			}
			else {
				TuranAPI::LOG_NOTCODED("While loading a model, you have to use Surface Material Type for now!", true);
				return 0;
			}
		}

		if (Loaded_Model) {
			Resource_Identifier* RESOURCE = new Resource_Identifier;
			RESOURCE->PATH = PATH;
			RESOURCE->DATA = Loaded_Model;
			RESOURCE->TYPE = RESOURCETYPEs::EDITOR_STATICMODEL;
			TuranEditor::EDITOR_FILESYSTEM->Add_anAsset_toAssetList(RESOURCE);
			return RESOURCE->ID;
		}
		else {
			return 0;
		}
	}

	void Find_AvailableAttributes(aiMesh* data, Attribute_BitMask& Available_Attributes) {
		if (data->HasNormals()) {
			Available_Attributes.Normal = true;
		}
		else {
			Available_Attributes.Normal = false;
		}
		if (data->HasTangentsAndBitangents()) {
			Available_Attributes.Bitangent = true;
			Available_Attributes.Tangent = true;
		}
		else {
			Available_Attributes.Bitangent = false;
			Available_Attributes.Tangent = false;
		}
		//Maximum Available UV Set Number is 4
		Available_Attributes.TextCoords = 0;
		for (unsigned int i = 0; i < 4; i++) {
			if (data->HasTextureCoords(i)) {
				Available_Attributes.TextCoords++;
			}
		}
		//Maximum Available VertexColor Set Number is 4
		Available_Attributes.VertexColors = 0;
		for (unsigned int i = 0; i < 4; i++) {
			if (data->HasVertexColors(i)) {
				Available_Attributes.VertexColors += 1;
			}
		}
		if (data->HasBones()) {
			Available_Attributes.Bones = true;
		}
		else {
			Available_Attributes.Bones = false;
		}
	}

	void Load_MeshData(const aiMesh* data, Attribute_BitMask& Attribute_Info, Static_Model* Model) {
		Static_Mesh_Data* MESH = new Static_Mesh_Data;
		Model->MESHes.push_back(MESH);

		//You should choose the Vertex Attribute Layout, then set the data according to it. So I should create a Attribute Layout Settings window here.
		//And give the user which attributes the imported asset has.
		MESH->Material_Index = data->mMaterialIndex;
		if (data->mNumVertices == 4563) {
			TuranAPI::LOG_CRASHING("This is it!");
		}
		MESH->VERTEX_NUMBER = data->mNumVertices;

		//Mesh Attribute Loading
		{
			GFX_API::VertexAttribute PositionAttribute;
			//Create Position Attribute!
			PositionAttribute.AttributeName = "Positions";
			PositionAttribute.Index = 0;
			PositionAttribute.Stride = 0;
			PositionAttribute.Start_Offset = 0;
			PositionAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC3;
			MESH->DataLayout.Attributes.push_back(PositionAttribute);

			//Use this index to set Attribute index for each attribute
			unsigned int NextAttribute_Index = 1;

			//Load UV Sets!
			for (unsigned int i = 0; i < Attribute_Info.TextCoords; i++) {
				GFX_API::VertexAttribute TextCoordAttribute;
				TextCoordAttribute.AttributeName.append("UV Set ");
				TextCoordAttribute.AttributeName.append(std::to_string(i).c_str());
				TextCoordAttribute.Index = NextAttribute_Index + i;
				TextCoordAttribute.Stride = 0;
				TextCoordAttribute.Start_Offset = 0;
				switch (data->mNumUVComponents[i]) {
				case 2:
					TextCoordAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC2;
					break;
				default:
					TextCoordAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC2;
					TuranAPI::LOG_WARNING("One of the meshes has unsupported number of channels in its Texture Coordinates!");
				}
				MESH->DataLayout.Attributes.push_back(TextCoordAttribute);
				NextAttribute_Index++;
			}

			//Load Vertex Color Sets!
			for (unsigned int i = 0; i < Attribute_Info.VertexColors; i++) {
				GFX_API::VertexAttribute VertColorAttribute;
				VertColorAttribute.AttributeName.append("VertColor Set ");
				VertColorAttribute.AttributeName.append(std::to_string(i).c_str());
				VertColorAttribute.Index = NextAttribute_Index + i;
				VertColorAttribute.Stride = 0;
				VertColorAttribute.Start_Offset = 0;
				VertColorAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC4;
				MESH->DataLayout.Attributes.push_back(VertColorAttribute);
				NextAttribute_Index++;
			}

			if (Attribute_Info.Normal) {
				GFX_API::VertexAttribute NormalAttribute;
				NormalAttribute.AttributeName = "Normals";
				NormalAttribute.Index = NextAttribute_Index;
				NextAttribute_Index++;
				NormalAttribute.Stride = 0;
				NormalAttribute.Start_Offset = 0;
				NormalAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC3;
				MESH->DataLayout.Attributes.push_back(NormalAttribute);
			}
			if (Attribute_Info.Tangent) {
				GFX_API::VertexAttribute TangentAttribute;
				TangentAttribute.AttributeName = "Tangents";
				TangentAttribute.Index = NextAttribute_Index;
				NextAttribute_Index++;
				TangentAttribute.Stride = 0;
				TangentAttribute.Start_Offset = 0;
				TangentAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC3;
				MESH->DataLayout.Attributes.push_back(TangentAttribute);
			}
			if (Attribute_Info.Bitangent) {
				GFX_API::VertexAttribute BitangentAttribute;
				BitangentAttribute.AttributeName = "Bitangents";
				BitangentAttribute.Index = NextAttribute_Index;
				NextAttribute_Index++;
				BitangentAttribute.Stride = 0;
				BitangentAttribute.Start_Offset = 0;
				BitangentAttribute.DATATYPE = GFX_API::DATA_TYPE::VAR_VEC3;
				MESH->DataLayout.Attributes.push_back(BitangentAttribute);
			}
			if (Attribute_Info.Bones) {
				TuranAPI::LOG_ERROR("Loader doesn't support to import Bone attribute of the meshes!");
			}


			//Final Checks on Attribute Layout
			if (!MESH->DataLayout.VerifyAttributeLayout()) {
				TuranAPI::LOG_ERROR("Attribute Layout isn't verified, there is a problem somewhere!");
				return;
			}
			MESH->DataLayout.Calculate_SizeperVertex();
		}

		MESH->VERTEXDATA_SIZE = MESH->DataLayout.size_pervertex * MESH->VERTEX_NUMBER;
		MESH->VERTEX_DATA = new unsigned char[MESH->VERTEXDATA_SIZE];
		if (!MESH->VERTEX_DATA) {
			TuranAPI::LOG_CRASHING("Allocator failed to create a buffer for mesh's data!");
			return;
		}


		//Filling MESH->DATA!
		{
			//First, fill the position buffer
			memcpy(MESH->VERTEX_DATA, data->mVertices, MESH->VERTEX_NUMBER *  sizeof(vec3));
			
			unsigned char* NEXTDATA_PTR = (unsigned char*)MESH->VERTEX_DATA + (MESH->VERTEX_NUMBER * sizeof(vec3));
			unsigned char NEXTATTRIB_INDEX = 1;
			GFX_API::VertexAttributeLayout& LAYOUT = MESH->DataLayout;

			
			if (Attribute_Info.TextCoords) {
				for (unsigned int i = 0; i < Attribute_Info.TextCoords; i++) {
					vec2* TEXTCOORD_PTR = (vec2*)NEXTDATA_PTR;
					GFX_API::VertexAttribute& TEXTCOORD_ATTRIB = LAYOUT.Attributes[NEXTATTRIB_INDEX];
					unsigned int data_size = MESH->VERTEX_NUMBER * 8;
					for (unsigned int j = 0; j < data_size / 8; j++) {
						TEXTCOORD_PTR[j] = vec2(data->mTextureCoords[i][j].x, data->mTextureCoords[i][j].y);
					}
					NEXTDATA_PTR += data_size;
					NEXTATTRIB_INDEX++;
				}
			}
			if (Attribute_Info.VertexColors) {
				for (unsigned int i = 0; i < Attribute_Info.VertexColors; i++) {
					GFX_API::VertexAttribute& VERTCOLOR_ATTRIB = LAYOUT.Attributes[NEXTATTRIB_INDEX];
					unsigned int data_size = MESH->VERTEX_NUMBER * GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(VERTCOLOR_ATTRIB.DATATYPE);
					memcpy(NEXTDATA_PTR, data->mColors[i], data_size);
					NEXTDATA_PTR += data_size;
					NEXTATTRIB_INDEX++;
				}
			}
			if (Attribute_Info.Normal) {
				GFX_API::VertexAttribute& NORMAL_ATTRIB = LAYOUT.Attributes[NEXTATTRIB_INDEX];
				unsigned int data_size = MESH->VERTEX_NUMBER * GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(NORMAL_ATTRIB.DATATYPE);
				memcpy(NEXTDATA_PTR, data->mNormals, data_size);
				NEXTDATA_PTR += data_size;
				NEXTATTRIB_INDEX++;
			}
			if (Attribute_Info.Tangent) {
				GFX_API::VertexAttribute& TANGENT_ATTRIB = LAYOUT.Attributes[NEXTATTRIB_INDEX];
				unsigned int data_size = MESH->VERTEX_NUMBER * GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(TANGENT_ATTRIB.DATATYPE);
				memcpy(NEXTDATA_PTR, data->mTangents, data_size);
				NEXTDATA_PTR += data_size;
				NEXTATTRIB_INDEX++;
			}
			if (Attribute_Info.Bitangent) {
				GFX_API::VertexAttribute& BITANGENT_ATTRIB = LAYOUT.Attributes[NEXTATTRIB_INDEX];
				unsigned int data_size = MESH->VERTEX_NUMBER * GFX_API::Get_UNIFORMTYPEs_SIZEinbytes(BITANGENT_ATTRIB.DATATYPE);
				memcpy(NEXTDATA_PTR, data->mBitangents, data_size);
				NEXTDATA_PTR += data_size;
				NEXTATTRIB_INDEX++;
			}
		}

		//Filling MESH->INDEXDATA!
		if (data->HasFaces()) {
			MESH->INDEX_DATA = new unsigned int[data->mNumFaces * 3];
			for (unsigned int i = 0; i < data->mNumFaces; i++) {
				MESH->INDEX_DATA[i * 3] = data->mFaces[i].mIndices[0];
				MESH->INDEX_DATA[i * 3 + 1] = data->mFaces[i].mIndices[1];
				MESH->INDEX_DATA[i * 3 + 2] = data->mFaces[i].mIndices[2];
			}
			MESH->INDICES_NUMBER = data->mNumFaces * 3;
		}
	}


	unsigned int Load_MaterialData(const aiMaterial* data, const char* directory, unsigned int* object_worldid) {
		SURFACEMAT_PROPERTIES properties;
		if (data->GetTextureCount(aiTextureType_DIFFUSE)) {
			aiString PATH;
			data->GetTexture(aiTextureType_DIFFUSE, 0, &PATH);
			string dir = (string)directory + '/';
			string texture_path = dir + (string)PATH.C_Str();
			properties.DIFFUSETEXTURE_ID = Texture_Loader::Import_Texture(texture_path.c_str())->ID;
		}
		if (data->GetTextureCount(aiTextureType_HEIGHT)) {
			aiString PATH;
			data->GetTexture(aiTextureType_HEIGHT, 0, &PATH);
			string dir = (string)directory + '/';
			string texture_path = dir + (string)PATH.C_Str();
			properties.NORMALSTEXTURE_ID = Texture_Loader::Import_Texture(texture_path.c_str())->ID;
		}
		if (data->GetTextureCount(aiTextureType_SPECULAR)) {
			aiString PATH;
			data->GetTexture(aiTextureType_SPECULAR, 0, &PATH);
			string dir = (string)directory + '/';
			string texture_path = dir + (string)PATH.C_Str();
			properties.SPECULARTEXTURE_ID = Texture_Loader::Import_Texture(texture_path.c_str())->ID;
		}
		if (data->GetTextureCount(aiTextureType_OPACITY)) {
			aiString PATH;
			data->GetTexture(aiTextureType_OPACITY, 0, &PATH);
			string dir = (string)directory + '/';
			string texture_path = dir + (string)PATH.C_Str();
			properties.OPACITYTEXTURE_ID = Texture_Loader::Import_Texture(texture_path.c_str())->ID;
		}

		return Editor_RendererDataManager::Create_SurfaceMaterialInstance(properties, object_worldid);
	}
}