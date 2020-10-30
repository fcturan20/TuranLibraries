#include "GFX_Renderer.h"
#include <string>

namespace GFX_API {
	Renderer::Renderer() {}
	Renderer::~Renderer() {}
	unsigned char Renderer::Get_LastFrameIndex() {
		if (FrameIndex) {
			return FrameIndex - 1;
		}
		else {
			return FrameCount - 1;
		}
	}
	void Renderer::Set_NextFrameIndex() {
		FrameIndex = (FrameIndex + 1) % FrameCount;
	}


}