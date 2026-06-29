#include "Profiler.h"

#include <assert.h>

#include <chrono>
#include <iostream>
#include <string>

#include "ECS.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCProfiler)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCProfiler)
TCORE_PLUGIN_ENTRY_POINT_END()

struct TCProfiledScope
{
	bool IsRecording : 1;
	TCDurationType DurationType : 2;
	unsigned long long startPoint;
	unsigned long long* duration;
	std::string name;
};

constexpr long long GetCurrentTime(TCDurationType durationType)
{
	switch (durationType)
	{
	case TC_DURATION_TYPE_NANOSECONDS:
		return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now())
			.time_since_epoch()
			.count();
		break;
	case TC_DURATION_TYPE_MICROSECONDS:
		return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
			.time_since_epoch()
			.count();
		break;
	case TC_DURATION_TYPE_MILLISECONDS:
		return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
			.time_since_epoch()
			.count();
		break;
	case TC_DURATION_TYPE_SECONDS:
		return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now())
			.time_since_epoch()
			.count();
		break;
	default: assert(0 && "Duration type is invalid!");
	}
}

static const char* GTimeNames[] = {"nanoseconds", "microseconds", "milliseconds", "seconds"};
struct TCProfilerContext* GContext = nullptr;
struct TCProfilerContext
{
	unsigned int ThreadCount;
	static TCProfiledScope* Begin(const char* name, unsigned long long* duration, TCDurationType durationType)
	{
		TCProfiledScope* profile = new TCProfiledScope;
		profile->startPoint = GetCurrentTime(durationType);
		profile->IsRecording = true;
		profile->duration = duration;
		profile->DurationType = durationType;
		profile->name = name;
		return profile;
	}

	static void Finish(TCProfiledScope* profil)
	{
		*profil->duration = GetCurrentTime(profil->DurationType) - profil->startPoint;
		delete profil;
	}
};

TCResult TCProfiler_Initialize(const void** outPluginAPI)
{
	auto services = new ITCProfiler;
	services->Begin = TCProfilerContext::Begin;
	services->End = TCProfilerContext::Finish;

	TCProfiler = services;
	*outPluginAPI = TCProfiler;
	GContext = new TCProfilerContext;
	return TC_RESULT_SUCCESS;
}

TCResult TCProfiler_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCProfiler_Shutdown()
{
	delete TCProfiler;
	TCProfiler = nullptr;
	return TC_RESULT_SUCCESS;
}

void TCProfiler_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded) {}