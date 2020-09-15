#include "Main.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"

#include "RenderContext/Editor_DataManager.h"

namespace TuranEditor {
	Editor_System::Editor_System() {
		new Vulkan::Vulkan_Core;
		Editor_RendererDataManager::Start_RenderingDataManagement();
	}
	Editor_System::~Editor_System() {
		delete GFX;
	}
	void Editor_System::Take_Inputs() {
		GFX->Take_Inputs();
	}
	bool Editor_System::Should_EditorClose() {
		return Should_EditorClose;
	}
	bool Editor_System::Should_Close = false;
}
