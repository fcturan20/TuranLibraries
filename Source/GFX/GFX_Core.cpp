#include "GFX_Core.h"
#include "TuranAPI/Logger_Core.h"

namespace GFX_API {

	GFX_Core* GFX_Core::SELF = nullptr;
	GFX_Core::GFX_Core(vector<MonitorDescription>& Monitors, vector<GPUDescription>& GPUs, TuranAPI::Threading::JobSystem* JobSystem) : RENDERER(nullptr),
		FOCUSED_WINDOW_index(0), GPU_TO_RENDER(nullptr), JobSys(JobSystem) {
		LOG_STATUS_TAPI("GFX Core systems are starting!");

		GFX = this;

		IMGUI = new GFX_API::IMGUI_Core;

		LOG_STATUS_TAPI("GFX Core systems are started!");
	}
}
