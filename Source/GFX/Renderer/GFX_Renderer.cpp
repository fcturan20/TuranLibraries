#include "GFX_Renderer.h"
#include <string>

namespace GFX_API {
	Renderer::Renderer() {}
	Renderer::~Renderer() {
	}
	void Renderer::Set_NextFrameIndex() {
		FrameIndex = (FrameIndex + 1) % FrameCount;
	}
	unsigned char Renderer::GetCurrentFrameIndex() {
		return FrameIndex;
	}


}