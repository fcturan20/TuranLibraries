#include "GFX_Includes.h"


namespace GFX_API {
	GFXHandle::operator void*() const{
		return Handle;
	}
	GFXHandle::GFXHandle(void* PTR) {
		Handle = PTR;
	}
	GFXHandle::GFXHandle() {
		Handle = nullptr;
	}

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
			LOG_CRASHING_TAPI("Uniform's size in bytes isn't set in GFX_ENUMs.cpp!");
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
			LOG_ERROR_TAPI("GetNameOf_TextureWRAPPING doesn't support this wrapping type!");
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
			LOG_ERROR_TAPI("GetTextureWRAPPING_byIndex doesn't support this index!\n");
		}
	}


	GFXAPI unsigned char GetByteSizeOf_TextureChannels(const TEXTURE_CHANNELs& channeltype) {
		switch (channeltype)
		{
		case TEXTURE_CHANNELs::API_TEXTURE_R8B:
		case TEXTURE_CHANNELs::API_TEXTURE_R8UB:
			return 1;
		case TEXTURE_CHANNELs::API_TEXTURE_RGB8B:
		case TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
			return 3;
		case TEXTURE_CHANNELs::API_TEXTURE_D24S8:
		case TEXTURE_CHANNELs::API_TEXTURE_D32:
		case TEXTURE_CHANNELs::API_TEXTURE_RGBA8B:
		case TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB:
		case TEXTURE_CHANNELs::API_TEXTURE_BGRA8UB:
		case TEXTURE_CHANNELs::API_TEXTURE_BGRA8UNORM:
		case TEXTURE_CHANNELs::API_TEXTURE_R32F:
		case TEXTURE_CHANNELs::API_TEXTURE_R32I:
		case TEXTURE_CHANNELs::API_TEXTURE_R32UI:
			return 4;
		case TEXTURE_CHANNELs::API_TEXTURE_RA32F:
		case TEXTURE_CHANNELs::API_TEXTURE_RA32I:
		case TEXTURE_CHANNELs::API_TEXTURE_RA32UI:
			return 8;
		case TEXTURE_CHANNELs::API_TEXTURE_RGB32F:
		case TEXTURE_CHANNELs::API_TEXTURE_RGB32I:
		case TEXTURE_CHANNELs::API_TEXTURE_RGB32UI:
			return 12;
		case TEXTURE_CHANNELs::API_TEXTURE_RGBA32F:
		case TEXTURE_CHANNELs::API_TEXTURE_RGBA32I:
		case TEXTURE_CHANNELs::API_TEXTURE_RGBA32UI:
			return 16;
		default:
			LOG_CRASHING_TAPI("GetSizeOf_TextureChannels() doesn't support this type!");
			break;
		}
	}
	GFXAPI const char* GetNameOf_TextureCHANNELs(TEXTURE_CHANNELs CHANNEL) {
		LOG_NOTCODED_TAPI("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (CHANNEL) {
		case TEXTURE_CHANNELs::API_TEXTURE_RGB8UB:
			return "RGB8UB";
		default:
			LOG_ERROR_TAPI("GetNameOf_TextureCHANNELs doesn't support this channel type!");
		}
	}
	GFXAPI vector<const char*> GetNames_TextureCHANNELs() {
		return vector<const char*>{

		};
	}
	GFXAPI TEXTURE_CHANNELs GetTextureCHANNEL_byIndex(unsigned int Index) {
		LOG_NOTCODED_TAPI("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (Index) {
		default:
			LOG_ERROR_TAPI("GetTextureCHANNEL_byIndex doesn't support this index!");
		}
		return TEXTURE_CHANNELs::API_TEXTURE_D24S8;
	}
	GFXAPI unsigned int GetIndexOf_TextureCHANNEL(TEXTURE_CHANNELs CHANNEL) {
		LOG_NOTCODED_TAPI("Texture Channels enum has changed but function related to it hasn't. Fix it!", true);
		switch (CHANNEL) {
		default:
			LOG_ERROR_TAPI("GetIndexOf_TextureCHANNEL doesn't support this channel type!");
		}
		return 0;
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
			LOG_ERROR_TAPI("GetNameof_SHADERLANGUAGE() doesn't support this language!");
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
			LOG_ERROR_TAPI("GetSHADERLANGUAGE_byIndex() doesn't support this index!");
		}
	}



	SHADERSTAGEs_FLAG::SHADERSTAGEs_FLAG() {
		VERTEXSHADER = false;
		FRAGMENTSHADER = false;
		COLORRTOUTPUT = false;
		TRANSFERCMD = false;
		SWAPCHAINDISPLAY = false;
	};
	SHADERSTAGEs_FLAG Create_ShaderStageFlag(bool vs, bool fs, bool rt_output, bool transfercmd, bool swpchn_diplay){
		SHADERSTAGEs_FLAG x;
		x.COLORRTOUTPUT = rt_output;
		x.FRAGMENTSHADER = fs;
		x.SWAPCHAINDISPLAY = swpchn_diplay;
		x.TRANSFERCMD = transfercmd;
		x.VERTEXSHADER = vs;
		return x;
	}



	TEXTUREUSAGEFLAG::TEXTUREUSAGEFLAG() {
		isCopiableFrom = false;
		isCopiableTo = false;
		isRandomlyWrittenTo = false;
		isRenderableTo = false;
		isSampledReadOnly = false;
	}


	Texture_Properties::Texture_Properties() {}
	Texture_Properties::Texture_Properties(TEXTURE_DIMENSIONs dimension, TEXTURE_MIPMAPFILTER mipmap_filtering, TEXTURE_WRAPPING wrapping, TEXTURE_CHANNELs channel_type, TEXTURE_ORDER dataorder)
		: DIMENSION(dimension), MIPMAP_FILTERING(mipmap_filtering), WRAPPING(wrapping), CHANNEL_TYPE(channel_type), DATAORDER(dataorder){}

	MemoryType::MemoryType(SUBALLOCATEBUFFERTYPEs HEAP, unsigned int MTIndex) : HEAPTYPE(HEAP), MemoryTypeIndex(MTIndex){}
}