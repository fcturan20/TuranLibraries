#pragma once

typedef struct uvec4_tgfx {
	unsigned int x, y, z, w;
};

typedef struct uvec3_tgfx {
    unsigned int x, y, z;
} uvec3_tgfx;

typedef struct uvec2_tgfx {
	unsigned int x, y;
} uvec2_tgfx;

typedef struct vec2_tgfx {
    float x, y;
} vec2_tgfx;

typedef struct vec3_tgfx {
    float x, y, z;
} vec3_tgfx;

typedef struct vec4_tgfx {
	float x, y, z, w;
};


typedef struct boxregion_tgfx {
    unsigned int XOffset, YOffset, WIDTH, HEIGHT;
} boxregion_tgfx;

typedef struct cuberegion_tgfx {
    unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
} cuberegion_tgfx;

typedef struct memory_description_tgfx {
	unsigned int memorytype_id;
	memoryallocationtype_tgfx allocationtype;
	unsigned long max_allocationsize;
};
typedef struct gpudescription_tgfx {
	const char* NAME;
	unsigned int API_VERSION, DRIVER_VERSION;
	gpu_type_tgfx GPU_TYPE;
	unsigned char is_GraphicOperations_Supported, is_ComputeOperations_Supported, is_TransferOperations_Supported;
	const memory_description_tgfx* MEMTYPEs;
	unsigned char MEMTYPEsCOUNT;
	unsigned char isSupported_SeperateDepthStencilLayouts, isSupported_SeperateRTSlotBlending,
		isSupported_NonUniformShaderInputIndexing;
	//These limits are maximum count of usable resources in a shader stage (VS, FS etc.)
	//Don't forget that sum of the accesses in a shader stage shouldn't exceed MaxUsableResources_perStage!
	unsigned int MaxSampledTexture_perStage, MaxImageTexture_perStage, MaxUniformBuffer_perStage, MaxStorageBuffer_perStage, MaxUsableResources_perStage;
	unsigned int MaxShaderInput_SampledTexture, MaxShaderInput_ImageTexture, MaxShaderInput_UniformBuffer, MaxShaderInput_StorageBuffer;
} gpudescription_tgfx;
