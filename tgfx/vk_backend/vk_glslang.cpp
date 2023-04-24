// glslang was using overriden "new" operator when started in TGFXVulkan.dll
// So I moved glslang to its own dynamic dll
#include <glslang/SPIRV/GlslangToSpv.h>

#include <string_tapi.h>
#include <predefinitions_tapi.h>
#include <tgfx_forwarddeclarations.h>

#include "vk_predefinitions.h"
#include "vk_includes.h"

TBuiltInResource glslToSpirvLimitTable;

static EShLanguage Find_EShShaderStage_byTGFXShaderStage(shaderStage_tgfx stage) {
  switch (stage) {
    case shaderStage_tgfx_VERTEXSHADER: return EShLangVertex;
    case shaderStage_tgfx_FRAGMENTSHADER: return EShLangFragment;
    case shaderStage_tgfx_COMPUTESHADER: return EShLangCompute;
    default:
      assert(0 && "Find_EShShaderStage_byTGFXShaderStage() doesn't support this type of stage!");
      return EShLangVertex;
  }
}

static wchar_t* glslangGetErrorMessage(glslang::TShader& shader) {
  wchar_t* wLog = nullptr;
  stringSys->createString(string_type_tapi_UTF16, ( void** )&wLog,
                          L"Shader compilation failed! %s %s", shader.getInfoLog(),
                          shader.getInfoDebugLog());
  return wLog;
}
const void*     vk_compileWithGlslang(shaderStage_tgfx tgfxstage, const void* i_DATA,
                                             unsigned int  i_DATA_SIZE,
                                             unsigned int* compiledbinary_datasize) {
  EShLanguage       stage = Find_EShShaderStage_byTGFXShaderStage(tgfxstage);
  glslang::TShader  shader(stage);
  glslang::TProgram program;

  // Enable SPIR-V and Vulkan rules when parsing GLSL
  EShMessages messages = ( EShMessages )(EShMsgSpvRules | EShMsgVulkanRules);

  const char* strings[1] = {( const char* )i_DATA};
  shader.setStrings(strings, 1);

  if (!shader.parse(&glslToSpirvLimitTable, 100, false, messages)) {
    *compiledbinary_datasize = 0;
    return glslangGetErrorMessage(shader);
  }

  program.addShader(&shader);

  //
  // Program-level processing...
  //

  if (!program.link(messages)) {
    *compiledbinary_datasize = 0;
    return glslangGetErrorMessage(shader);
  }
  std::vector<unsigned int> binarydata;
  glslang::GlslangToSpv(*program.getIntermediate(stage), binarydata);
  if (binarydata.size()) {
    unsigned int* outbinary  = new unsigned int[binarydata.size()];
    *compiledbinary_datasize = binarydata.size() * 4;
    memcpy(outbinary, binarydata.data(), binarydata.size() * 4);
    return outbinary;
  }

  // Fail
  {
    *compiledbinary_datasize = 0;
    return L"glslang couldn't compile the shader!";
  }
}

void vk_initGlslang() {
  // Start glslang
  glslang::InitializeProcess();
  // Initialize limitation table
  // from Eric's Blog "Translate GLSL to SPIRV for Vulkan at Runtime" post:
  // https://lxjk.github.io/2020/03/10/Translate-GLSL-to-SPIRV-for-Vulkan-at-Runtime.html
  glslToSpirvLimitTable.maxLights                                   = 32;
  glslToSpirvLimitTable.maxClipPlanes                               = 6;
  glslToSpirvLimitTable.maxTextureUnits                             = 32;
  glslToSpirvLimitTable.maxTextureCoords                            = 32;
  glslToSpirvLimitTable.maxVertexAttribs                            = 64;
  glslToSpirvLimitTable.maxVertexUniformComponents                  = 4096;
  glslToSpirvLimitTable.maxVaryingFloats                            = 64;
  glslToSpirvLimitTable.maxVertexTextureImageUnits                  = 32;
  glslToSpirvLimitTable.maxCombinedTextureImageUnits                = 80;
  glslToSpirvLimitTable.maxTextureImageUnits                        = 32;
  glslToSpirvLimitTable.maxFragmentUniformComponents                = 4096;
  glslToSpirvLimitTable.maxDrawBuffers                              = 32;
  glslToSpirvLimitTable.maxVertexUniformVectors                     = 128;
  glslToSpirvLimitTable.maxVaryingVectors                           = 8;
  glslToSpirvLimitTable.maxFragmentUniformVectors                   = 16;
  glslToSpirvLimitTable.maxVertexOutputVectors                      = 16;
  glslToSpirvLimitTable.maxFragmentInputVectors                     = 15;
  glslToSpirvLimitTable.minProgramTexelOffset                       = -8;
  glslToSpirvLimitTable.maxProgramTexelOffset                       = 7;
  glslToSpirvLimitTable.maxClipDistances                            = 8;
  glslToSpirvLimitTable.maxComputeWorkGroupCountX                   = 65535;
  glslToSpirvLimitTable.maxComputeWorkGroupCountY                   = 65535;
  glslToSpirvLimitTable.maxComputeWorkGroupCountZ                   = 65535;
  glslToSpirvLimitTable.maxComputeWorkGroupSizeX                    = 1024;
  glslToSpirvLimitTable.maxComputeWorkGroupSizeY                    = 1024;
  glslToSpirvLimitTable.maxComputeWorkGroupSizeZ                    = 64;
  glslToSpirvLimitTable.maxComputeUniformComponents                 = 1024;
  glslToSpirvLimitTable.maxComputeTextureImageUnits                 = 16;
  glslToSpirvLimitTable.maxComputeImageUniforms                     = 8;
  glslToSpirvLimitTable.maxComputeAtomicCounters                    = 8;
  glslToSpirvLimitTable.maxComputeAtomicCounterBuffers              = 1;
  glslToSpirvLimitTable.maxVaryingComponents                        = 60;
  glslToSpirvLimitTable.maxVertexOutputComponents                   = 64;
  glslToSpirvLimitTable.maxGeometryInputComponents                  = 64;
  glslToSpirvLimitTable.maxGeometryOutputComponents                 = 128;
  glslToSpirvLimitTable.maxFragmentInputComponents                  = 128;
  glslToSpirvLimitTable.maxImageUnits                               = 8;
  glslToSpirvLimitTable.maxCombinedImageUnitsAndFragmentOutputs     = 8;
  glslToSpirvLimitTable.maxCombinedShaderOutputResources            = 8;
  glslToSpirvLimitTable.maxImageSamples                             = 0;
  glslToSpirvLimitTable.maxVertexImageUniforms                      = 0;
  glslToSpirvLimitTable.maxTessControlImageUniforms                 = 0;
  glslToSpirvLimitTable.maxTessEvaluationImageUniforms              = 0;
  glslToSpirvLimitTable.maxGeometryImageUniforms                    = 0;
  glslToSpirvLimitTable.maxFragmentImageUniforms                    = 8;
  glslToSpirvLimitTable.maxCombinedImageUniforms                    = 8;
  glslToSpirvLimitTable.maxGeometryTextureImageUnits                = 16;
  glslToSpirvLimitTable.maxGeometryOutputVertices                   = 256;
  glslToSpirvLimitTable.maxGeometryTotalOutputComponents            = 1024;
  glslToSpirvLimitTable.maxGeometryUniformComponents                = 1024;
  glslToSpirvLimitTable.maxGeometryVaryingComponents                = 64;
  glslToSpirvLimitTable.maxTessControlInputComponents               = 128;
  glslToSpirvLimitTable.maxTessControlOutputComponents              = 128;
  glslToSpirvLimitTable.maxTessControlTextureImageUnits             = 16;
  glslToSpirvLimitTable.maxTessControlUniformComponents             = 1024;
  glslToSpirvLimitTable.maxTessControlTotalOutputComponents         = 4096;
  glslToSpirvLimitTable.maxTessEvaluationInputComponents            = 128;
  glslToSpirvLimitTable.maxTessEvaluationOutputComponents           = 128;
  glslToSpirvLimitTable.maxTessEvaluationTextureImageUnits          = 16;
  glslToSpirvLimitTable.maxTessEvaluationUniformComponents          = 1024;
  glslToSpirvLimitTable.maxTessPatchComponents                      = 120;
  glslToSpirvLimitTable.maxPatchVertices                            = 32;
  glslToSpirvLimitTable.maxTessGenLevel                             = 64;
  glslToSpirvLimitTable.maxViewports                                = 16;
  glslToSpirvLimitTable.maxVertexAtomicCounters                     = 0;
  glslToSpirvLimitTable.maxTessControlAtomicCounters                = 0;
  glslToSpirvLimitTable.maxTessEvaluationAtomicCounters             = 0;
  glslToSpirvLimitTable.maxGeometryAtomicCounters                   = 0;
  glslToSpirvLimitTable.maxFragmentAtomicCounters                   = 8;
  glslToSpirvLimitTable.maxCombinedAtomicCounters                   = 8;
  glslToSpirvLimitTable.maxAtomicCounterBindings                    = 1;
  glslToSpirvLimitTable.maxVertexAtomicCounterBuffers               = 0;
  glslToSpirvLimitTable.maxTessControlAtomicCounterBuffers          = 0;
  glslToSpirvLimitTable.maxTessEvaluationAtomicCounterBuffers       = 0;
  glslToSpirvLimitTable.maxGeometryAtomicCounterBuffers             = 0;
  glslToSpirvLimitTable.maxFragmentAtomicCounterBuffers             = 1;
  glslToSpirvLimitTable.maxCombinedAtomicCounterBuffers             = 1;
  glslToSpirvLimitTable.maxAtomicCounterBufferSize                  = 16384;
  glslToSpirvLimitTable.maxTransformFeedbackBuffers                 = 4;
  glslToSpirvLimitTable.maxTransformFeedbackInterleavedComponents   = 64;
  glslToSpirvLimitTable.maxCullDistances                            = 8;
  glslToSpirvLimitTable.maxCombinedClipAndCullDistances             = 8;
  glslToSpirvLimitTable.maxSamples                                  = 4;
  glslToSpirvLimitTable.maxMeshOutputVerticesNV                     = 256;
  glslToSpirvLimitTable.maxMeshOutputPrimitivesNV                   = 512;
  glslToSpirvLimitTable.maxMeshWorkGroupSizeX_NV                    = 32;
  glslToSpirvLimitTable.maxMeshWorkGroupSizeY_NV                    = 1;
  glslToSpirvLimitTable.maxMeshWorkGroupSizeZ_NV                    = 1;
  glslToSpirvLimitTable.maxTaskWorkGroupSizeX_NV                    = 32;
  glslToSpirvLimitTable.maxTaskWorkGroupSizeY_NV                    = 1;
  glslToSpirvLimitTable.maxTaskWorkGroupSizeZ_NV                    = 1;
  glslToSpirvLimitTable.maxMeshViewCountNV                          = 4;
  glslToSpirvLimitTable.limits.nonInductiveForLoops                 = 1;
  glslToSpirvLimitTable.limits.whileLoops                           = 1;
  glslToSpirvLimitTable.limits.doWhileLoops                         = 1;
  glslToSpirvLimitTable.limits.generalUniformIndexing               = 1;
  glslToSpirvLimitTable.limits.generalAttributeMatrixVectorIndexing = 1;
  glslToSpirvLimitTable.limits.generalVaryingIndexing               = 1;
  glslToSpirvLimitTable.limits.generalSamplerIndexing               = 1;
  glslToSpirvLimitTable.limits.generalVariableIndexing              = 1;
  glslToSpirvLimitTable.limits.generalConstantMatrixVectorIndexing  = 1;
}