#pragma once
#include "Vulkan/VulkanSource/Vulkan_Includes.h"
#include "GFX/Renderer/GFX_Resource.h"


namespace Vulkan {
	struct VK_API VK_Texture {
		VkImage Image = {};
		VkImageView ImageView = {};
	};

	struct VK_API VK_ShaderProgram {
		VkPipelineLayout PipelineLayout;
		VkPipeline PipelineObject;
		VkDescriptorSetLayout MaterialData_DescSetLayout;
	};

	struct VK_API VK_VertexAttribLayout {
		VkVertexInputBindingDescription BindingDesc;	//Currently, only one binding is supported because I didn't understand bindings properly.
		VkVertexInputAttributeDescription* AttribDescs;
		unsigned char AttribDesc_Count;
	};

	struct VK_API VK_MemoryAllocation {
		unsigned int FullSize, UnusedSize;
		VkBufferUsageFlags Usage;
		VkBuffer Base_Buffer;
		VkMemoryRequirements Requirements;
		VkDeviceMemory Allocated_Memory;
	};

	struct VK_API VK_MemoryBlock {
		unsigned int Size = 0, Offset = 0;
		bool isEmpty = true;
	};

	struct VK_API VK_Mesh {
		unsigned int Vertex_Count, Index_Count;
		VkDeviceSize VertexBufOffset, IndexBufOffset;
		GFX_API::VertexAttributeLayout* Layout;
	};

	struct VK_API VK_DrawPass {
		VkRenderPass RenderPassObject;
		vector<VkFramebuffer> FramebufferObjects;
	};

	struct VK_API VK_TransferPacket {

	};

	struct VK_API VK_DescPool {
		VkDescriptorPool pool;
		unsigned int REMAINING_SET, REMAINING_UBUFFER, REMAINING_SBUFFER, REMAINING_SAMPLER, REMAINING_IMAGE;
	};
}