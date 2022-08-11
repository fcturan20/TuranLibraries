#pragma once

typedef struct tgfx_uvec4 {
	unsigned int x, y, z, w;
} uvec4_tgfx;

typedef struct tgfx_uvec3 {
    unsigned int x, y, z;
} uvec3_tgfx;

typedef struct tgfx_uvec2 {
	unsigned int x, y;
} uvec2_tgfx;

typedef struct tgfx_vec2 {
    float x, y;
} vec2_tgfx;

typedef struct tgfx_vec3 {
    float x, y, z;
} vec3_tgfx;

typedef struct tgfx_vec4 {
	float x, y, z, w;
} vec4_tgfx;


typedef struct tgfx_boxRegion {
    unsigned int XOffset, YOffset, WIDTH, HEIGHT;
} boxRegion_tgfx;

typedef struct tgfx_cubeRegion {
    unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
} cubeRegion_tgfx;

typedef struct tgfx_memory_description {
	unsigned int memorytype_id;
	memoryallocationtype_tgfx allocationtype;
	unsigned long max_allocationsize;
} memoryDescription_tgfx;

typedef struct tgfx_gpu_description {
	const char* NAME;
	unsigned int API_VERSION, DRIVER_VERSION;
	gpu_type_tgfx GPU_TYPE;
	unsigned char is_GraphicOperations_Supported, is_ComputeOperations_Supported, is_TransferOperations_Supported;
	const memoryDescription_tgfx* MEMTYPEs;
	unsigned char MEMTYPEsCOUNT;
	unsigned char isSupported_SeperateDepthStencilLayouts, isSupported_SeperateRTSlotBlending,
		isSupported_NonUniformShaderInputIndexing;
	//These limits are maximum count of usable resources in a shader stage (VS, FS etc.)
	//Don't forget that sum of the accesses in a shader stage shouldn't exceed MaxUsableResources_perStage!
	unsigned int MaxSampledTexture_perStage, MaxImageTexture_perStage, MaxUniformBuffer_perStage, MaxStorageBuffer_perStage, MaxUsableResources_perStage;
	unsigned int MaxShaderInput_SampledTexture, MaxShaderInput_ImageTexture, MaxShaderInput_UniformBuffer, MaxShaderInput_StorageBuffer;
} gpuDescription_tgfx;


typedef struct tgfx_init_secondstage_info {
  gpu_tgfxhnd RendererGPU;
  /*
  If isMultiThreaded is 0 :
  * 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
  * 2) TGFX never uses multiple threads. So GPU command creation process may take longer on large workloads
  * If isMultiThreaded is 1 :
  * 1) You shouldn't call TGFX functions concurrently. So your calls to TGFX functions should be externally synchronized.
  * 2) TGFX uses multiple threads to create GPU commands faster.
  * If isMultiThreaded is 2 :
  * 1) You can call TGFX functions concurrently. You can call all TGFX functions asynchronously unless otherwise is mentioned.
  * 2) TGFX uses multiple threads to create GPU commands faster.
  */
  unsigned char ShouldActive_dearIMGUI, isMultiThreaded;
  unsigned int MAXDYNAMICSAMPLER_DESCCOUNT, MAXDYNAMICSAMPLEDIMAGE_DESCCOUNT,
    MAXDYNAMICSTORAGEIMAGE_DESCCOUNT, MAXDYNAMICBUFFER_DESCCOUNT,
    MAXBINDINGTABLECOUNT;
} initSecondStageInfo_tgfx;
