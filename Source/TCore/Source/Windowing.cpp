#include <cstring>
#include <GLFW/glfw3.h>
#include <new>

#define TCORE_USE_CPP_WRAPPER
#include <Windowing.h>
#include <CppGenerics.h>
#include <TString.h>
#include <Allocator.h>
#include <UnitTestSystem.h>
#include <Logger.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCWindowing)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_INIT(TCStringSys)
TCORE_PLUGIN_INIT(TCLog)
TCORE_PLUGIN_MEMORY_BLOCK_INIT()

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCWindowing)
TCORE_PLUGIN_HARD_DEPENDENCY(TCAllocator, TCAllocator_PLUGIN_VERSION);
TCORE_PLUGIN_HARD_DEPENDENCY(TCStringSys, TCStringSys_PLUGIN_VERSION);
TCORE_PLUGIN_HARD_DEPENDENCY(TCLog, TCLog_PLUGIN_VERSION);
TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(1 << 20);
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Windowing
{

enum class WindowingHandleTypes : unsigned short
{
	Window,
	Monitor
};

void* GetPointer(TU8 memOffset)
{
	TCORE_SOFT_CHECK(memOffset, "Invalid pointer");
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GSuperMemoryBlock) + memOffset);
}

TU8 GetMemOffset(const void* ptr)
{
	TU8 target = reinterpret_cast<uintptr_t>(ptr);
	TU8 superMemBlock = reinterpret_cast<uintptr_t>(GSuperMemoryBlock);
	TCORE_SOFT_CHECK(superMemBlock < target, "Invalid pointer");
	return target - superMemBlock;
}

TCORE_DEFINE_OPAQUE_HANDLE_SYSTEM(TCW, WindowingHandleTypes)

struct UVec2
{
	unsigned int X, Y;
};

struct Monitor : TCWObjectBase<Monitor, TCMonitor, WindowingHandleTypes::Monitor>
{
	uint16_t GetExtraFlags() { return 0; }
	UVec2 res, physicalSize;
	unsigned int color_bites = 0, refresh_rate = 00;
	const wchar_t* name = NULL;
	GLFWmonitor* GlfwMonitor = NULL;
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Monitor, TCW)

struct Window : TCWObjectBase<Window, TCWindow, WindowingHandleTypes::Window>
{
	Window() = default;
	uint16_t GetExtraFlags() { return 0; }
	unsigned int m_lastWidth, m_lastHeight, m_newWidth, m_newHeight;
	TCWindowMode DisplayMode = TC_WINDOWMODE_WINDOWED;
	Monitor* Monitor = nullptr;
	TCore::String Name;
	TCWindowResizeCallback ResizeCallback = nullptr;
	TCWindowKeyCallback KeyCallback = nullptr;
	TCWindowCloseCallback CloseCallback = nullptr;
	void* UserPtr = nullptr;
	bool IsResized = false, IsSwapped = false;
	// Presentation Fences should only be used for CPU to wait
	bool IsMouseButtonPressed[3] = {};

	GLFWwindow* GlfwWindow = {};
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(Window, TCW)

TCKey GetTcKeyFromGlfwKey(int glfwKey)
{
	switch (glfwKey)
	{
	case GLFW_KEY_UNKNOWN: return TC_KEY_UNKNOWN;
	case GLFW_KEY_SPACE: return TC_KEY_SPACE;
	case GLFW_KEY_APOSTROPHE: return TC_KEY_APOSTROPHE;
	case GLFW_KEY_COMMA: return TC_KEY_COMMA;
	case GLFW_KEY_MINUS: return TC_KEY_MINUS;
	case GLFW_KEY_PERIOD: return TC_KEY_PERIOD;
	case GLFW_KEY_SLASH: return TC_KEY_SLASH;
	case GLFW_KEY_0: return TC_KEY_0;
	case GLFW_KEY_1: return TC_KEY_1;
	case GLFW_KEY_2: return TC_KEY_2;
	case GLFW_KEY_3: return TC_KEY_3;
	case GLFW_KEY_4: return TC_KEY_4;
	case GLFW_KEY_5: return TC_KEY_5;
	case GLFW_KEY_6: return TC_KEY_6;
	case GLFW_KEY_7: return TC_KEY_7;
	case GLFW_KEY_8: return TC_KEY_8;
	case GLFW_KEY_9: return TC_KEY_9;
	case GLFW_KEY_SEMICOLON: return TC_KEY_SEMICOLON;
	case GLFW_KEY_EQUAL: return TC_KEY_EQUAL;
	case GLFW_KEY_A: return TC_KEY_A;
	case GLFW_KEY_B: return TC_KEY_B;
	case GLFW_KEY_C: return TC_KEY_C;
	case GLFW_KEY_D: return TC_KEY_D;
	case GLFW_KEY_E: return TC_KEY_E;
	case GLFW_KEY_F: return TC_KEY_F;
	case GLFW_KEY_G: return TC_KEY_G;
	case GLFW_KEY_H: return TC_KEY_H;
	case GLFW_KEY_I: return TC_KEY_I;
	case GLFW_KEY_J: return TC_KEY_J;
	case GLFW_KEY_K: return TC_KEY_K;
	case GLFW_KEY_L: return TC_KEY_L;
	case GLFW_KEY_M: return TC_KEY_M;
	case GLFW_KEY_N: return TC_KEY_N;
	case GLFW_KEY_O: return TC_KEY_O;
	case GLFW_KEY_P: return TC_KEY_P;
	case GLFW_KEY_Q: return TC_KEY_Q;
	case GLFW_KEY_R: return TC_KEY_R;
	case GLFW_KEY_S: return TC_KEY_S;
	case GLFW_KEY_T: return TC_KEY_T;
	case GLFW_KEY_U: return TC_KEY_U;
	case GLFW_KEY_V: return TC_KEY_V;
	case GLFW_KEY_W: return TC_KEY_W;
	case GLFW_KEY_X: return TC_KEY_X;
	case GLFW_KEY_Y: return TC_KEY_Y;
	case GLFW_KEY_Z: return TC_KEY_Z;
	case GLFW_KEY_LEFT_BRACKET: return TC_KEY_LEFT_BRACKET;
	case GLFW_KEY_BACKSLASH: return TC_KEY_BACKSLASH;
	case GLFW_KEY_RIGHT_BRACKET: return TC_KEY_RIGHT_BRACKET;
	case GLFW_KEY_GRAVE_ACCENT: return TC_KEY_GRAVE_ACCENT;
	case GLFW_KEY_WORLD_1: return TC_KEY_WORLD_1;
	case GLFW_KEY_WORLD_2: return TC_KEY_WORLD_2;
	case GLFW_KEY_ESCAPE: return TC_KEY_ESCAPE;
	case GLFW_KEY_ENTER: return TC_KEY_ENTER;
	case GLFW_KEY_TAB: return TC_KEY_TAB;
	case GLFW_KEY_BACKSPACE: return TC_KEY_BACKSPACE;
	case GLFW_KEY_INSERT: return TC_KEY_INSERT;
	case GLFW_KEY_DELETE: return TC_KEY_DELETE;
	case GLFW_KEY_RIGHT: return TC_KEY_RIGHT;
	case GLFW_KEY_LEFT: return TC_KEY_LEFT;
	case GLFW_KEY_DOWN: return TC_KEY_DOWN;
	case GLFW_KEY_UP: return TC_KEY_UP;
	case GLFW_KEY_PAGE_UP: return TC_KEY_PAGE_UP;
	case GLFW_KEY_PAGE_DOWN: return TC_KEY_PAGE_DOWN;
	case GLFW_KEY_HOME: return TC_KEY_HOME;
	case GLFW_KEY_END: return TC_KEY_END;
	case GLFW_KEY_CAPS_LOCK: return TC_KEY_CAPS_LOCK;
	case GLFW_KEY_SCROLL_LOCK: return TC_KEY_SCROLL_LOCK;
	case GLFW_KEY_NUM_LOCK: return TC_KEY_NUM_LOCK;
	case GLFW_KEY_PRINT_SCREEN: return TC_KEY_PRINT_SCREEN;
	case GLFW_KEY_PAUSE: return TC_KEY_PAUSE;
	case GLFW_KEY_F1: return TC_KEY_F1;
	case GLFW_KEY_F2: return TC_KEY_F2;
	case GLFW_KEY_F3: return TC_KEY_F3;
	case GLFW_KEY_F4: return TC_KEY_F4;
	case GLFW_KEY_F5: return TC_KEY_F5;
	case GLFW_KEY_F6: return TC_KEY_F6;
	case GLFW_KEY_F7: return TC_KEY_F7;
	case GLFW_KEY_F8: return TC_KEY_F8;
	case GLFW_KEY_F9: return TC_KEY_F9;
	case GLFW_KEY_F10: return TC_KEY_F10;
	case GLFW_KEY_F11: return TC_KEY_F11;
	case GLFW_KEY_F12: return TC_KEY_F12;
	case GLFW_KEY_F13: return TC_KEY_F13;
	case GLFW_KEY_F14: return TC_KEY_F14;
	case GLFW_KEY_F15: return TC_KEY_F15;
	case GLFW_KEY_F16: return TC_KEY_F16;
	case GLFW_KEY_F17: return TC_KEY_F17;
	case GLFW_KEY_F18: return TC_KEY_F18;
	case GLFW_KEY_F19: return TC_KEY_F19;
	case GLFW_KEY_F20: return TC_KEY_F20;
	case GLFW_KEY_F21: return TC_KEY_F21;
	case GLFW_KEY_F22: return TC_KEY_F22;
	case GLFW_KEY_F23: return TC_KEY_F23;
	case GLFW_KEY_F24: return TC_KEY_F24;
	case GLFW_KEY_F25: return TC_KEY_F25;
	case GLFW_KEY_KP_0: return TC_KEY_KP_0;
	case GLFW_KEY_KP_1: return TC_KEY_KP_1;
	case GLFW_KEY_KP_2: return TC_KEY_KP_2;
	case GLFW_KEY_KP_3: return TC_KEY_KP_3;
	case GLFW_KEY_KP_4: return TC_KEY_KP_4;
	case GLFW_KEY_KP_5: return TC_KEY_KP_5;
	case GLFW_KEY_KP_6: return TC_KEY_KP_6;
	case GLFW_KEY_KP_7: return TC_KEY_KP_7;
	case GLFW_KEY_KP_8: return TC_KEY_KP_8;
	case GLFW_KEY_KP_9: return TC_KEY_KP_9;
	case GLFW_KEY_KP_DECIMAL: return TC_KEY_KP_DECIMAL;
	case GLFW_KEY_KP_DIVIDE: return TC_KEY_KP_DIVIDE;
	case GLFW_KEY_KP_MULTIPLY: return TC_KEY_KP_MULTIPLY;
	case GLFW_KEY_KP_SUBTRACT: return TC_KEY_KP_SUBTRACT;
	case GLFW_KEY_KP_ADD: return TC_KEY_KP_ADD;
	case GLFW_KEY_KP_ENTER: return TC_KEY_KP_ENTER;
	case GLFW_KEY_KP_EQUAL: return TC_KEY_KP_EQUAL;
	case GLFW_KEY_LEFT_SHIFT: return TC_KEY_LEFT_SHIFT;
	case GLFW_KEY_LEFT_CONTROL: return TC_KEY_LEFT_CONTROL;
	case GLFW_KEY_LEFT_ALT: return TC_KEY_LEFT_ALT;
	case GLFW_KEY_LEFT_SUPER: return TC_KEY_LEFT_SUPER;
	case GLFW_KEY_RIGHT_SHIFT: return TC_KEY_RIGHT_SHIFT;
	case GLFW_KEY_RIGHT_CONTROL: return TC_KEY_RIGHT_CONTROL;
	case GLFW_KEY_RIGHT_ALT: return TC_KEY_RIGHT_ALT;
	case GLFW_KEY_RIGHT_SUPER: return TC_KEY_RIGHT_SUPER;
	case GLFW_KEY_MENU: return TC_KEY_MENU;
	case GLFW_MOUSE_BUTTON_LEFT: return TC_KEY_MOUSE_LEFT;
	case GLFW_MOUSE_BUTTON_MIDDLE: return TC_KEY_MOUSE_MIDDLE;
	case GLFW_MOUSE_BUTTON_RIGHT: return TC_KEY_MOUSE_RIGHT;
	}
	return (TCKey)INT32_MAX;
}

TCKeyAction GetTcKeyActionFromGlfw(int glfwAction)
{
	switch (glfwAction)
	{
	case GLFW_PRESS: return TC_KEYACTION_PRESS;
	case GLFW_RELEASE: return TC_KEYACTION_RELEASE;
	case GLFW_REPEAT: return TC_KEYACTION_REPEAT;
	}
	return (TCKeyAction)INT32_MAX;
}

TCKeyModifier GetTcKeyModifierFromGlfw(int glfwMod)
{
	switch (glfwMod)
	{
	case GLFW_MOD_SHIFT: return TC_KEYMODIFIER_SHIFT;
	case GLFW_MOD_ALT: return TC_KEYMODIFIER_ALT;
	case GLFW_MOD_CAPS_LOCK: return TC_KEYMODIFIER_CAPSLOCK;
	case GLFW_MOD_CONTROL: return TC_KEYMODIFIER_CONTROL;
	case GLFW_MOD_NUM_LOCK: return TC_KEYMODIFIER_NUMLOCK;
	case GLFW_MOD_SUPER: return TC_KEYMODIFIER_SUPER;
	}
	return (TCKeyModifier)INT32_MAX;
}

struct WindowingContext* GContext = nullptr;
struct WindowingContext
{
	WindowingContext() { GContext = this; }

	TCResult Initialize()
	{
		glfwInit();
		return {TC_RESULTSTATE_SUCCESS, 0};
	}

	static void GlfwWindowResizeCallback(GLFWwindow* glfwwindow, int width, int height)
	{
		auto w = (Window*)glfwGetWindowUserPointer(glfwwindow);
		UVec2 res = {width, height};
		w->m_newWidth = width;
		w->m_newHeight = height;
		w->ResizeCallback(GetOpaqueHandle(w), w->UserPtr, res.X, res.Y);
	}

	static void GlfwWindowKeyCallback(GLFWwindow* glfwWindow, int key, int scan, int action, int mode)
	{
		auto w = (Window*)glfwGetWindowUserPointer(glfwWindow);
		w->KeyCallback(GetOpaqueHandle(w),
					   w->UserPtr,
					   GetTcKeyFromGlfwKey(key),
					   scan,
					   GetTcKeyActionFromGlfw(action),
					   GetTcKeyModifierFromGlfw(mode));
	}

	static void GlfwWindowCloseCallback(GLFWwindow* glfwWindow, int key, int scan, int action, int mode)
	{
		auto w = (Window*)glfwGetWindowUserPointer(glfwWindow);
		w->CloseCallback(GetOpaqueHandle(w));
	}

	static void GLFWErrorCallback(int error_code, const char* description)
	{
		static constexpr unsigned int maxLength = 1ull << 12;
		char maxLog[maxLength] = {"GLFW: "};
		strcpy_s(&maxLog[6], maxLength - 7, description);
		// vkPrint(16, maxLog);
	}

	static void CreateWindow(const TCWindowDescription* desc, void* user_ptr, TCWindow* oWindow)
	{
		if (desc->ResizeCallback)
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		else
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		GLFWmonitor* monitor = nullptr;
		if (auto m = GetTCWObject(desc->Monitor); m && desc->Mode == TC_WINDOWMODE_FULLSCREEN)
			monitor = m->GlfwMonitor;

		GLFWwindow* glfwWndw = glfwCreateWindow(desc->Width, desc->Height, desc->Name, monitor, nullptr);

		// Check and Report if GLFW fails
		if (glfwWndw == NULL)
			return;

		auto w = TCStdAllocator->Malloc(GSuperMemoryBlock, sizeof(Window), "Window");
		auto window = new (w) Window;
		window->m_lastWidth = desc->Width;
		window->m_newWidth = desc->Width;
		window->m_lastHeight = desc->Height;
		window->m_newHeight = desc->Height;
		window->DisplayMode = desc->Mode;
		window->Monitor = nullptr;
		window->Name = desc->Name;
		window->GlfwWindow = glfwWndw;
		window->ResizeCallback = desc->ResizeCallback;
		window->KeyCallback = desc->KeyCallback;
		window->CloseCallback = desc->CloseCallback;
		window->UserPtr = user_ptr;

		glfwSetWindowUserPointer(window->GlfwWindow, window);
		if (desc->ResizeCallback)
			glfwSetWindowSizeCallback(window->GlfwWindow, GlfwWindowResizeCallback);
		if (desc->KeyCallback)
			glfwSetKeyCallback(window->GlfwWindow, GlfwWindowKeyCallback);
		if (desc->CloseCallback)
			glfwSetKeyCallback(window->GlfwWindow, GlfwWindowCloseCallback);

		*oWindow = GetOpaqueHandle(window);
	}

	static void GetMonitorInfos(TU8* monitorCount, TCMonitorInfo* monitorInfos)
	{
		int count{};
		auto monitors = glfwGetMonitors(&count);
		*monitorCount = count;

		if (!monitorInfos)
			return;

		for (int i = 0; i < count; i++)
		{
			auto& m = monitors[i];
			auto& out = monitorInfos[i];
			out.Name = glfwGetMonitorName(m);

			auto vMode = glfwGetVideoMode(m);
			monitorInfos->Width = vMode->width;
			monitorInfos->Height = vMode->height;
			monitorInfos->RefreshRate = vMode->refreshRate;
			monitorInfos->RedColorBits = vMode->redBits;
			monitorInfos->GreenColorBits = vMode->greenBits;
			monitorInfos->BlueColorBits = vMode->blueBits;
		}
	}

	static void DestroyWindow(TCWindow window) { glfwDestroyWindow(GetTCWObject(window)->GlfwWindow); }
};

static void RegisterUnitTests()
{
	// TEST 0
	{
		TCUnitTestDescription desc{};
		desc.Name = "TCWindowing_Test_0";
		desc.GlobalCategoryName = "TCore";
		desc.Test = [](TCReadBuffer inputData) -> TCResult {
			TU8 monitorCount = 0;
			TCMonitorInfo monitorInfos[16];
			TCWindowing->GetMonitorInfos(&monitorCount, monitorInfos);
			if (!monitorCount)
			{
				te() << "No monitor found!";
				return {TC_RESULTSTATE_NOT_FOUND, 1};
			}

			// Create windowed
			TCWindowDescription desc;
			TCWindow w;
			desc.Height = 540;
			desc.Width = 960;
			desc.Mode = TC_WINDOWMODE_WINDOWED;
			desc.Name = "Test Window";
			TCWindowing->CreateWindow(&desc, nullptr, &w);
			auto input = TCUnitTest->WaitForInput(TTRUE, "Is window visible? Yes (y), No (n)");
			TCWindowing->DestroyWindow(w);
			if (!input || input[0] != 'y')
				return {TC_RESULTSTATE_FAILURE, 2};

			// Create fullscreen
			desc.Height = monitorInfos[0].Height;
			desc.Width = monitorInfos[0].Width;
			desc.Mode = TC_WINDOWMODE_FULLSCREEN;
			desc.Name = "Test Window";
			TCWindowing->CreateWindow(&desc, nullptr, &w);
			input = TCUnitTest->WaitForInput(TTRUE, "Is window visible? Yes (y), No (n)");
			TCWindowing->DestroyWindow(w);
			if (!input || input[0] != 'y')
				return {TC_RESULTSTATE_FAILURE, 2};

			return {TC_RESULTSTATE_SUCCESS, 0};
		};
		TCUnitTest->RegisterTest(&desc);
	}
}
} // namespace Windowing
} // namespace TCore

TCResult TCWindowing_Initialize(const void** outPluginAPI)
{
	new TCore::Windowing::WindowingContext();
	if (auto res = TCore::Windowing::GContext->Initialize(); res.State != TC_RESULTSTATE_SUCCESS)
		return res;

	auto api = new ITCWindowing;
	api->CreateWindow = TCore::Windowing::WindowingContext::CreateWindow;
	api->DestroyWindow = TCore::Windowing::WindowingContext::DestroyWindow;
	api->GetMonitorInfos = TCore::Windowing::WindowingContext::GetMonitorInfos;
	TCWindowing = api;
	*outPluginAPI = TCWindowing;

	if (TCORE_SOFT_DEPENDENCY(TCUnitTest).State == TC_RESULTSTATE_SUCCESS)
		TCore::Windowing::RegisterUnitTests();
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