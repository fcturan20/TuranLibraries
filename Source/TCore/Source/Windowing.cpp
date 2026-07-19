#include <Windowing.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCWindowing)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCWindowing)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Windowing
{
struct WindowingContext* GContext = nullptr;
struct WindowingContext
{
	WindowingContext() { GContext = this; }

	static void GLFWErrorCallback(int error_code, const char* description)
	{
		wchar_t maxLog[1 << 12] = {"GLFW: "};
		mbstowcs(&maxLog[6], description, (1ull << 10) - 1);
		vkPrint(16, maxLog);
	}
	static void CreateWindow(const tgfx_windowDescription* desc, void* user_ptr, struct tgfx_window** window)
	{
		if (desc->resizeCb)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}
		GLFWmonitor* monitor = nullptr;
		if (desc->monitor && desc->mode == windowmode_tgfx_FULLSCREEN)
		{
			monitor = getOBJ<MONITOR_VKOBJ>(desc->monitor)->monitorobj;
		}
		static constexpr uint32_t VKCONST_MAX_WINDOW_NAME_CHAR = 256;
		char windowCName[VKCONST_MAX_WINDOW_NAME_CHAR] = {};
		stringSys->convertString(
			tlStringUTF16, desc->name, tlStringUTF8, windowCName, VKCONST_MAX_WINDOW_NAME_CHAR - 1);
		GLFWwindow* glfwWndw = glfwCreateWindow(desc->size.x, desc->size.y, windowCName, monitor, nullptr);

		// Check and Report if GLFW fails
		if (glfwWndw == NULL)
		{
			vkPrint(18);
			return;
		}

		// Window VulkanSurface Creation
		VkSurfaceKHR wndwSurface = {};
		if (glfwCreateWindowSurface(GVkInstance, glfwWndw, nullptr, &wndwSurface) != VK_SUCCESS)
		{
			vkPrint(16, "vkSurfaceKHR creation failed at glfwCreateWindowSurface");
			return;
		}

		WINDOW_VKOBJ* vkWindow = priv->WINDOWs.create_OBJ();
		vkWindow->m_lastWidth = desc->size.x;
		vkWindow->m_newWidth = desc->size.x;
		vkWindow->m_lastHeight = desc->size.y;
		vkWindow->m_newHeight = desc->size.y;
		vkWindow->m_displayMode = desc->mode;
		vkWindow->m_monitor = nullptr;
		vkWindow->m_name = desc->name;
		vkWindow->vk_glfwWindow = glfwWndw;
		vkWindow->vk_swapchainTextureUsage = 0; // This will be set while creating swapchain
		vkWindow->m_resizeFnc = desc->resizeCb;
		vkWindow->m_keyFnc = desc->keyCb;
		vkWindow->m_closeFnc = desc->closeCb;
		vkWindow->m_userData = user_ptr;
		vkWindow->vk_surface = wndwSurface;

		glfwSetWindowUserPointer(vkWindow->vk_glfwWindow, vkWindow);
		if (desc->resizeCb)
		{
			glfwSetWindowSizeCallback(vkWindow->vk_glfwWindow, glfwWindowResizeCallback);
		}
		if (desc->keyCb)
		{
			glfwSetKeyCallback(vkWindow->vk_glfwWindow, glfwWindowKeyCallback);
		}
		if (desc->closeCb)
		{
			glfwSetWindowCloseCallback(vkWindow->vk_glfwWindow, glfwWindowCloseCallback);
		}

		*window = getHANDLE<struct tgfx_window*>(vkWindow);
	}
};
} // namespace Windowing
} // namespace TCore

TCResult TCWindowing_Initialize(const void** outPluginAPI)
{
	new TCore::Windowing::WindowingContext();
	*outPluginAPI = TCWindowing;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCWindowing_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCWindowing_Shutdown()
{
	if (TCore::Windowing::GContext)
	{
		delete TCore::Windowing::GContext;
		TCore::Windowing::GContext = nullptr;
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCWindowing_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This function is called when a plugin is loaded or unloaded. You can use this to check if a
	// plugin that your plugin depends on is loaded or not. If a plugin that your plugin depends on
	// is unloaded, you can disable your plugin's functionality that depends on that plugin.
}