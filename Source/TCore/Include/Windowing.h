#pragma once
#include <TCore.h>

TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCWindowing, "TCWindowing", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));

typedef struct TCWindowDescription
{
	const char* title;
	unsigned int width;
	unsigned int height;
	unsigned char resizable;
	unsigned char fullscreen;
	unsigned char borderless;
	unsigned char alwaysOnTop;
	unsigned char transparent;
} TCWindowDescription;

typedef struct ITCWindowing
{
	// Create a new window. The window is not visible until ShowWindow() is called.
	// The window is not resizable by default. To make it resizable, set the resizable flag in the
	// description.
	// The window is not fullscreen by default. To make it fullscreen, set the fullscreen flag in the
	// description.
	// The window is not borderless by default. To make it borderless, set the borderless flag in the
	// description.
	// The window is not always on top by default. To make it always on top, set the alwaysOnTop flag in
	// the description.
	// The window is not transparent by default. To make it transparent, set the transparent flag in the
	// description.
	void (*CreateWindow)(const TCWindowDescription* desc, void* user, struct twindow** outWindowHandle);
} ITCWindowing;

TCORE_END_C_LINKAGE