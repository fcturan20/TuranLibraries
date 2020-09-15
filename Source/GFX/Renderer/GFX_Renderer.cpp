#include "GFX_Renderer.h"
#include <string>

namespace GFX_API {
	Renderer::Renderer() : DrawPassID_BITSET(8), SubDrawPassID_BITSET(8) {}
	Renderer::~Renderer() {}


	unsigned int Renderer::Create_DrawPassID() {
		unsigned int ID = DrawPassID_BITSET.GetIndex_FirstFalse() + 1;
		DrawPassID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void Renderer::Delete_DrawPassID(unsigned int ID) {
		DrawPassID_BITSET.SetBit_False(ID - 1);
	}

	unsigned int Renderer::Create_SubDrawPassID() {
		unsigned int ID = SubDrawPassID_BITSET.GetIndex_FirstFalse() + 1;
		SubDrawPassID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void Renderer::Delete_SubDrawPassID(unsigned int ID) {
		SubDrawPassID_BITSET.SetBit_False(ID - 1);
	}
}