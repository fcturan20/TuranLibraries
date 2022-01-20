#pragma once
#include "predefinitions_vk.h"


struct gpudatamanager_private;
struct gpudatamanager_public {
	gpudatamanager_private* hidden;
	void Resource_Finalizations();
};