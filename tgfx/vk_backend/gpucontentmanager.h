#pragma once
#include "predefinitions_vk.h"
#include "resource.h"

struct gpudatamanager_private;
struct gpudatamanager_public {
	gpudatamanager_private* hidden;
	//Create Global descriptor sets
	void Resource_Finalizations();
	//Apply changes in Descriptor Sets, RTSlotSets
	void Apply_ResourceChanges();

	VK_LINEAR_OBJARRAY<VERTEXBUFFER_VKOBJ, 1 << 24>& GETVB_ARRAY();
	VK_LINEAR_OBJARRAY<INDEXBUFFER_VKOBJ>& GETIB_ARRAY();
	VK_LINEAR_OBJARRAY<GLOBALSSBO_VKOBJ>& GETGLOBALSSBO_ARRAY();
	VK_LINEAR_OBJARRAY<RTSLOTSET_VKOBJ, 1 << 10>& GETRTSLOTSET_ARRAY();
	VK_LINEAR_OBJARRAY<IRTSLOTSET_VKOBJ, 1 << 10>& GETIRTSLOTSET_ARRAY();
	VK_LINEAR_OBJARRAY<COMPUTETYPE_VKOBJ>& GETCOMPUTETYPE_ARRAY();
	VK_LINEAR_OBJARRAY<COMPUTEINST_VKOBJ>& GETCOMPUTEINST_ARRAY();
	VK_LINEAR_OBJARRAY<TEXTURE_VKOBJ, 1 << 24>& GETTEXTURES_ARRAY();
	VK_LINEAR_OBJARRAY<GRAPHICSPIPELINETYPE_VKOBJ, 1 << 24>& GETGRAPHICSPIPETYPE_ARRAY();
	VK_LINEAR_OBJARRAY<GRAPHICSPIPELINEINST_VKOBJ, 1 << 24>& GETGRAPHICSPIPEINST_ARRAY();
	VK_LINEAR_OBJARRAY< DESCSET_VKOBJ, 1 << 16>& GETDESCSET_ARRAY();
};