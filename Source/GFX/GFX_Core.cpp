#include "GFX_Core.h"
#include "TuranAPI/Logger_Core.h"

namespace GFX_API {

	GFX_Core* GFX_Core::SELF = nullptr;
	GFX_Core::GFX_Core() : RENDERER(nullptr),
		FOCUSED_WINDOW_index(0), GPU_TO_RENDER(nullptr) {
		TuranAPI::LOG_STATUS("GFX Core systems are starting!");

		GFX = this;

		IMGUI = new GFX_API::IMGUI_Core;

		TuranAPI::LOG_STATUS("GFX Core systems are started!");
	}
	GFX_Core::~GFX_Core() {
		std::cout << "GFX_Core's destructor is called!\n";
	}


	MONITOR* GFX_Core::Create_MonitorOBJ(void* monitor, const char* name) { return new MONITOR(monitor, name); TuranAPI::LOG_STATUS("A monitor is added"); }

}
