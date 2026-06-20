#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCThreading, "tcThreading", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCThread);
typedef struct ITCThreading
{
	TCThreadHandle (*Create)(void (*threadFunc)(TCBuffer), TCBuffer threadData, const char* threadName);
	void (*Join)(TCThreadHandle thread);
	void (*Destroy)(TCThreadHandle thread);
	void (*Sleep)(unsigned int milliseconds);
	size_t (*GetCurrentThreadIndex)();
} ITCThreading;

typedef struct ITCTask
{

} ITCTask;

TCORE_END_C_LINKAGE