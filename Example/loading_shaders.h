#pragma once
#include "loading_libraries.h"


extern rasterPipelineType_tgfxhnd base_3dshadertype = NULL;
extern rasterPipelineInstance_tgfxhnd base_3dshaderinst = NULL;
extern samplingType_tgfxhnd linear1mip_repeatsampler = NULL;
extern computeShaderType_tgfxhnd RT_computeshadertype = NULL;
extern computeShaderInstance_tgfxhnd RT_computeshaderinstance = NULL;

class SHADERS {
public:
	static void LoadShaders() {
		/*
		void* vert_text = filesys->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstShader.vert"); if (!vert_text) { printf("Vertex Shader read has failed!"); }
		void* frag_text = filesys->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstShader.frag"); if (!frag_text) { printf("Fragment Shader read has failed!"); }
		shadersource_tgfx_handle compiled_sources[3];
		TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_VERTEXSHADER, vert_text, 0, &compiled_sources[0]);
		TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_FRAGMENTSHADER, frag_text, 0, &compiled_sources[1]);
		compiled_sources[2] = (shadersource_tgfx_handle)TGFXINVALID;
		shaderinputdescription_tgfx_handle inputdescs[2] = { TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_SAMPLER_G, 0, 1,
			TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_FRAGMENTSHADER)), (shaderinputdescription_tgfx_handle)TGFXINVALID };
		extension_tgfx_handle depthbounds_ext[2] = { TGFXHELPER->EXT_DepthBoundsInfo(0.0, 1.0), (extension_tgfx_handle)TGFXINVALID };
		depthsettings_tgfx_handle depthconfig = TGFXHELPER->CreateDepthConfiguration(true, depthtest_tgfx_LEQUAL, depthbounds_ext);
		TGFXCONTENTMANAGER->Link_MaterialType(compiled_sources, inputdescs, vertexattriblayout, framerendering_subDP, cullmode_tgfx_OFF,
			polygonmode_tgfx_FILL, depthconfig, nullptr, nullptr, (blendinginfo_tgfx_listhandle)&TGFXINVALID, &base_3dshadertype);
		TGFXCONTENTMANAGER->Create_MaterialInst(base_3dshadertype, &base_3dshaderinst);
		TGFXCONTENTMANAGER->Create_SamplingType(0, 1, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, &linear1mip_repeatsampler);
		TGFXCONTENTMANAGER->SetGlobalShaderInput_Texture(true, 0, false, orange_texture, linear1mip_repeatsampler, image_access_tgfx_SHADER_SAMPLEONLY);

		const char* comp_text = filesys->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstComputeShader.comp");
		shadersource_tgfx_handle compiled_computeshadersource = nullptr;
		TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_COMPUTESHADER, (void*)comp_text, 0, &compiled_computeshadersource);
		shaderinputdescription_tgfx_handle first_compute_inputs[4] = { TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_UBUFFER_G, 0, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)),
			TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_UBUFFER_G, 1, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)),
			TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_IMAGE_G, 2, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)), (shaderinputdescription_tgfx_handle)TGFXINVALID };
		TGFXCONTENTMANAGER->Create_ComputeType(compiled_computeshadersource, first_compute_inputs, &RT_computeshadertype);
		TGFXCONTENTMANAGER->Create_ComputeInstance(RT_computeshadertype, &RT_computeshaderinstance);
		TGFXCONTENTMANAGER->SetComputeType_Buffer(RT_computeshadertype, false, 0, shaderuniformsbuffer, buffertype_tgfx_GLOBAL, true, 0, 0, 32);
		TGFXCONTENTMANAGER->SetComputeType_Buffer(RT_computeshadertype, false, 1, shaderuniformsbuffer, buffertype_tgfx_GLOBAL, true, 0, 64, 142);
		TGFXCONTENTMANAGER->SetComputeType_Texture(RT_computeshadertype, false, 2, orange_texture, false, 0, nullptr, image_access_tgfx_SHADER_WRITEONLY);
		*/
	}
};