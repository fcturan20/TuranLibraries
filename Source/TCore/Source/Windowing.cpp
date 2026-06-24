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
};
} // namespace Windowing
} // namespace TCore

TCResult TCWindowing_Initialize(const void** outPluginAPI)
{
	new TCore::Windowing::WindowingContext();
	*outPluginAPI = TCWindowing;
	return TC_RESULT_SUCCESS;
}

TCResult TCWindowing_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCWindowing_Shutdown()
{
	if (TCore::Windowing::GContext)
	{
		delete TCore::Windowing::GContext;
		TCore::Windowing::GContext = nullptr;
	}
	return TC_RESULT_SUCCESS;
}

void TCWindowing_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This function is called when a plugin is loaded or unloaded. You can use this to check if a
	// plugin that your plugin depends on is loaded or not. If a plugin that your plugin depends on
	// is unloaded, you can disable your plugin's functionality that depends on that plugin.
}