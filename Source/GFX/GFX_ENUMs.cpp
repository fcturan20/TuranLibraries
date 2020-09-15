#include "GFX_ENUMs.h"

namespace GFX_API {
	GFXAPI unsigned int Get_UNIFORMTYPEs_SIZEinbytes(DATA_TYPE uniform) {
		switch (uniform)
		{
		case DATA_TYPE::VAR_FLOAT32:
		case DATA_TYPE::VAR_INT32:
		case DATA_TYPE::VAR_UINT32:
			return 4;
		case DATA_TYPE::VAR_BYTE8:
		case DATA_TYPE::VAR_UBYTE8:
			return 1;
		case DATA_TYPE::VAR_VEC2:
			return 8;
		case DATA_TYPE::VAR_VEC3:
			return 12;
		case DATA_TYPE::VAR_VEC4:
			return 16;
		case DATA_TYPE::VAR_MAT4x4:
			return 64;
		default:
			TuranAPI::LOG_CRASHING("Uniform's size in bytes isn't set in GFX_ENUMs.cpp!");
			return 0;
		}
	}

	GFXAPI const char* Find_UNIFORM_VARTYPE_Name(DATA_TYPE uniform_var_type) {
		switch (uniform_var_type) {
		case DATA_TYPE::VAR_UINT32:
			return "Unsigned Integer 32-bit";
		case DATA_TYPE::VAR_INT32:
			return "Signed Integer 32-bit";
		case DATA_TYPE::VAR_FLOAT32:
			return "Float 32-bit";
		case DATA_TYPE::VAR_VEC2:
			return "Vec2 (2 float)";
		case DATA_TYPE::VAR_VEC3:
			return "Vec3 (3 float)";
		case DATA_TYPE::VAR_VEC4:
			return "Vec4 (4 float)";
		case DATA_TYPE::VAR_MAT4x4:
			return "Matrix 4x4";
		case DATA_TYPE::VAR_UBYTE8:
			return "Unsigned Byte 8-bit";
		case DATA_TYPE::VAR_BYTE8:
			return "Signed Byte 8-bit";
		default:
			return "Error, Uniform_Var_Type isn't supported by Find_UNIFORM_VARTYPE_Name!\n";
		}
	}


	
	GFXAPI const char* GetNameOf_TextureWRAPPING(TEXTURE_WRAPPING WRAPPING) {
		switch (WRAPPING) {
		case TEXTURE_WRAPPING::API_TEXTURE_REPEAT:
			return "Repeat";
		case TEXTURE_WRAPPING::API_TEXTURE_MIRRORED_REPEAT:
			return "Mirrored Repeat";
		case TEXTURE_WRAPPING::API_TEXTURE_CLAMP_TO_EDGE:
			return "Clamp to Edge";
		default:
			TuranAPI::LOG_ERROR("GetNameOf_TextureWRAPPING doesn't support this wrapping type!");
			return "ERROR";
		}
	}
	GFXAPI vector<const char*> GetNames_TextureWRAPPING() {
		return vector<const char*>{
			GetNameOf_TextureWRAPPING(TEXTURE_WRAPPING::API_TEXTURE_REPEAT), GetNameOf_TextureWRAPPING(TEXTURE_WRAPPING::API_TEXTURE_MIRRORED_REPEAT),
				GetNameOf_TextureWRAPPING(TEXTURE_WRAPPING::API_TEXTURE_CLAMP_TO_EDGE)
		};
	}
	GFXAPI TEXTURE_WRAPPING GetTextureWRAPPING_byIndex(unsigned int Index) {
		switch (Index) {
		case 0:
			return TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
		case 1:
			return TEXTURE_WRAPPING::API_TEXTURE_MIRRORED_REPEAT;
		case 2:
			return TEXTURE_WRAPPING::API_TEXTURE_CLAMP_TO_EDGE;
		default:
			TuranAPI::LOG_ERROR("GetTextureWRAPPING_byIndex doesn't support this index!\n");
		}
	}


	GFXAPI const char* GetNameOf_TextureCHANNELs(TEXTURE_CHANNELs CHANNEL) {
		TuranAPI::LOG_NOTCODED("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (CHANNEL) {
		case TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
			return "RGB8UB";
		default:
			TuranAPI::LOG_ERROR("GetNameOf_TextureCHANNELs doesn't support this channel type!");
		}
	}
	GFXAPI vector<const char*> GetNames_TextureCHANNELs() {
		return vector<const char*>{

		};
	}
	GFXAPI TEXTURE_CHANNELs GetTextureCHANNEL_byIndex(unsigned int Index) {
		TuranAPI::LOG_NOTCODED("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (Index) {
		default:
			TuranAPI::LOG_ERROR("GetTextureCHANNEL_byIndex doesn't support this index!");
		}
	}
	GFXAPI unsigned int GetIndexOf_TextureCHANNEL(TEXTURE_CHANNELs CHANNEL) {
		TuranAPI::LOG_NOTCODED("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (CHANNEL) {
		default:
			TuranAPI::LOG_ERROR("GetIndexOf_TextureCHANNEL doesn't support this channel type!");
		}
	}



	GFXAPI const char* GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs LANGUAGE) {
		switch (LANGUAGE) {
		case SHADER_LANGUAGEs::GLSL:
			return "GLSL";
		case SHADER_LANGUAGEs::HLSL:
			return "HLSL";
		case SHADER_LANGUAGEs::SPIRV:
			return "SPIR-V";
		case SHADER_LANGUAGEs::TSL:
			return "TSL";
		default:
			TuranAPI::LOG_ERROR("GetNameof_SHADERLANGUAGE() doesn't support this language!");
		}
	}
	GFXAPI vector<const char*> GetNames_SHADERLANGUAGEs() {
		return vector<const char*> {
				GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs::GLSL), GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs::HLSL), 
				GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs::SPIRV), GetNameof_SHADERLANGUAGE(SHADER_LANGUAGEs::TSL)
		};
	}
	GFXAPI SHADER_LANGUAGEs GetSHADERLANGUAGE_byIndex(unsigned int Index) {
		switch (Index) {
		case 0:
			return SHADER_LANGUAGEs::GLSL;
		case 1:
			return SHADER_LANGUAGEs::HLSL;
		case 2:
			return SHADER_LANGUAGEs::SPIRV;
		case 3:
			return SHADER_LANGUAGEs::TSL;
		default:
			TuranAPI::LOG_ERROR("GetSHADERLANGUAGE_byIndex() doesn't support this index!");
		}
	}



	SHADERSTAGEs_FLAG::SHADERSTAGEs_FLAG() {
		VERTEXSHADER = false;
		FRAGMENTSHADER = false;
	};
}