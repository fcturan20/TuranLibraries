#include <predefinitions_tapi.h>



struct texture_vk {
	struct MemoryBlock {
		unsigned int MemAllocIndex = UINT32_MAX;
		VkDeviceSize Offset;
	};
	unsigned int WIDTH, HEIGHT, DATA_SIZE;
	unsigned char MIPCOUNT;
	texture_channels_tgfx CHANNELs;
	VkImageUsageFlags USAGE;
	texture_dimensions_tgfx DIMENSION;

	VkImage Image = {};
	VkImageView ImageView = {};
	MemoryBlock Block;
};
