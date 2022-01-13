#pragma once
#include "includes.h"
#include <vector>
#include <tgfx_forwarddeclarations.h>

typedef struct drawpass_vk drawpass_vk;
typedef struct transferpass_vk transferpass_vk;
typedef struct windowpass_vk windowpass_vk;
typedef struct computepass_vk computepass_vk;

//Renderer data that other parts of the backend can access
class renderer_public {
private:
	std::vector<drawpass_vk*> DrawPasses;
	std::vector<transferpass_vk*> TransferPasses;
	std::vector<windowpass_vk*> WindowPasses;
	std::vector<computepass_vk*> ComputePasses;
	unsigned char FrameIndex;
public:
	inline unsigned char Get_FrameIndex(bool is_LastFrame){
		return (is_LastFrame) ? ((FrameIndex + 1) % 2) : (FrameIndex);
	}
};