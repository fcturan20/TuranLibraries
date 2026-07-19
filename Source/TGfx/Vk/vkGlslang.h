#include <vk_predefinitions.h>

namespace TGFX
{
namespace Vulkan
{
class GLSLang
{
public:
	static void Initialize();
	static void* Compile(TGfxShaderStage tgfxstage,
						 const void* i_DATA,
						 unsigned int i_DATA_SIZE,
						 unsigned int* compiledbinary_datasize);
};
} // namespace Vulkan
} // namespace TGFX