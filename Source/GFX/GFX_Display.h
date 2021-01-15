#pragma once
#include "GFX_Includes.h"

namespace GFX_API {
	struct GFXAPI MonitorDescription {
		GFXHandle Handle;
		string NAME;
		unsigned int WIDTH, HEIGHT, COLOR_BITES, REFRESH_RATE;
		int PHYSICAL_WIDTH, PHYSICAL_HEIGHT;	//milimeters
		WINDOW_MODE DESKTOP_MODE;
	};

	struct GFXAPI GPUDescription {
	public:
		string MODEL;
		uint32_t API_VERSION;
		uint32_t DRIVER_VERSION;
		GPU_TYPEs GPU_TYPE;
		bool is_GraphicOperations_Supported = false, is_ComputeOperations_Supported = false, is_TransferOperations_Supported = false;
	};

	struct GFXAPI WindowDescription {
		const char* NAME; 
		unsigned int WIDTH, HEIGHT;
		GFX_API::WINDOW_MODE MODE; 
		GFX_API::GFXHandle MONITOR;
	};
}