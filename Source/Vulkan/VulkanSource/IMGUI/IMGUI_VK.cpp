#include "IMGUI_VK.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"
#include "GFX/GFX_Core.h"
#define VKCORE ((Vulkan_Core*)GFX)
#define VKSTATES ((Vulkan::Vulkan_Core*)GFX)->VK_States
#define VKGPU (((Vulkan::Vulkan_Core*)GFX)->VK_States.GPU_TO_RENDER)

namespace Vulkan {
	void CheckIMGUIVKResults(VkResult result) {
		if (result != VK_SUCCESS) {
			LOG_CRASHING_TAPI("IMGUI's Vulkan backend has failed, please report!");
		}
	}

	TAPIResult IMGUI_VK::Initialize(GFX_API::GFXHandle SubPass, VkDescriptorPool DescPool) {
		if (!VKCORE->Get_WindowHandles().size()) {
			LOG_ERROR_TAPI("Initialization of dear IMGUI has failed because you didn't create a window!");
			return TAPI_FAIL;
		}
		ImGui_ImplGlfw_InitForVulkan(GFXHandleConverter(WINDOW*, VKCORE->Get_WindowHandles()[0])->GLFW_WINDOW, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VKSTATES.Vulkan_Instance;
		init_info.PhysicalDevice = VKGPU->Physical_Device;
		init_info.Device = VKGPU->Logical_Device;
		for (unsigned int i = 0; i < VKGPU->QUEUEs.size(); i++) {
			VK_QUEUE& queue = VKGPU->QUEUEs[i];
			if (queue.SupportFlag.is_PRESENTATIONsupported && queue.SupportFlag.is_GRAPHICSsupported) {
				init_info.QueueFamily = queue.QueueFamilyIndex;
				init_info.Queue = queue.Queue;
			}
		}
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = DescPool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		init_info.ImageCount = 2;
		init_info.CheckVkResultFn = CheckIMGUIVKResults;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		VK_SubDrawPass* SP = GFXHandleConverter(VK_SubDrawPass*, SubPass);
		init_info.Subpass = static_cast<uint32_t>(SP->Binding_Index);
		ImGui_ImplVulkan_Init(&init_info, GFXHandleConverter(VK_DrawPass*, SP->DrawPass)->RenderPassObject);
	}

	void IMGUI_VK::NewFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void IMGUI_VK::Render_AdditionalWindows() {
		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
	void IMGUI_VK::Render_toCB(VkCommandBuffer cb) {
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
	}

	void IMGUI_VK::UploadFontTextures() {

		//Upload font textures
		VkCommandPoolCreateInfo cp_ci = {};
		cp_ci.flags = 0;
		cp_ci.pNext = nullptr;
		cp_ci.queueFamilyIndex = VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].QueueFamilyIndex;
		cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		VkCommandPool cp;
		if (vkCreateCommandPool(VKGPU->Logical_Device, &cp_ci, nullptr, &cp) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Creation of Command Pool for dear IMGUI Font Upload has failed!");
		}

		VkCommandBufferAllocateInfo cb_ai = {};
		cb_ai.commandBufferCount = 1;
		cb_ai.commandPool = cp;
		cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cb_ai.pNext = nullptr;
		cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		VkCommandBuffer cb;
		if (vkAllocateCommandBuffers(VKGPU->Logical_Device, &cb_ai, &cb) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Creation of Command Buffer for dear IMGUI Font Upload has failed!");
		}



		if (vkResetCommandPool(VKGPU->Logical_Device, cp, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Resetting of Command Pool for dear IMGUI Font Upload has failed!");
		}
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(cb, &begin_info) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("vkBeginCommandBuffer() for dear IMGUI Font Upload has failed!");
		}

		ImGui_ImplVulkan_CreateFontsTexture(cb);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &cb;
		if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("vkEndCommandBuffer() for dear IMGUI Font Upload has failed!");
		}
		if (vkQueueSubmit(VKGPU->QUEUEs[VKGPU->GRAPHICS_QUEUEIndex].Queue, 1, &end_info, VK_NULL_HANDLE) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("VkQueueSubmit() for dear IMGUI Font Upload has failed!");
		}

		if (vkDeviceWaitIdle(VKGPU->Logical_Device) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("vkDeviceWaitIdle() for dear IMGUI Font Upload has failed!");
		}
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		if (vkResetCommandPool(VKGPU->Logical_Device, cp, 0) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Resetting of Command Pool for destruction of dear IMGUI Font Upload has failed!");
		}
		vkDestroyCommandPool(VKGPU->Logical_Device, cp, nullptr);
	}

	void IMGUI_VK::Destroy_IMGUIResources() {
		// Resources to destroy when the program ends
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}