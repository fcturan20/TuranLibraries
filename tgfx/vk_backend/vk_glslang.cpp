// glslang was using overriden "new" operator when started in TGFXVulkan.dll
// So I moved glslang to its own dynamic dll
#include <glslang/SPIRV/GlslangToSpv.h>
#include <predefinitions_tapi.h>

void start() {
  // Start glslang
  glslang::InitializeProcess();
}

FUNC_DLIB_EXPORT void startGlslang() { start(); }