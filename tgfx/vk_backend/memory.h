#pragma once
#include "predefinitions_vk.h"



struct memorytype_vk {
	unsigned int memorytypeindex;
	memoryallocationtype_tgfx alloctype_tgfx;

};

struct allocatorsys_privatefuncs;
struct allocatorsys_vk {
	void analize_gpumemory(gpu_public* gpu);
};