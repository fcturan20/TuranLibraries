#pragma once
#include <TCore.h>

TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCWindowing, "TCWindowing", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0));

TCORE_DEFINE_HANDLE(TCMonitor);
TCORE_DEFINE_HANDLE(TCWindow);

typedef struct TCMonitorInfo
{
	TU4 Width;
	TU4 Height;
	TU4 RedColorBits;
	TU4 GreenColorBits;
	TU4 BlueColorBits;
	TU4 RefreshRate;
	const char* Name;
	TCMonitor Monitor;
} TCMonitorInfo;

typedef enum TCWindowMode
{
	TC_WINDOWMODE_FULLSCREEN,
	TC_WINDOWMODE_WINDOWED
} TCWindowMode;

typedef enum TCKey
{
	/* The unknown key */
	TC_KEY_UNKNOWN,

	/* Printable keys */
	TC_KEY_SPACE,
	TC_KEY_APOSTROPHE, /* ' */
	TC_KEY_COMMA,	   /* , */
	TC_KEY_MINUS,	   /* - */
	TC_KEY_PERIOD,	   /* . */
	TC_KEY_SLASH,	   /* / */
	TC_KEY_0,
	TC_KEY_1,
	TC_KEY_2,
	TC_KEY_3,
	TC_KEY_4,
	TC_KEY_5,
	TC_KEY_6,
	TC_KEY_7,
	TC_KEY_8,
	TC_KEY_9,
	TC_KEY_SEMICOLON, /* ; */
	TC_KEY_EQUAL,	  /* = */
	TC_KEY_A,
	TC_KEY_B,
	TC_KEY_C,
	TC_KEY_D,
	TC_KEY_E,
	TC_KEY_F,
	TC_KEY_G,
	TC_KEY_H,
	TC_KEY_I,
	TC_KEY_J,
	TC_KEY_K,
	TC_KEY_L,
	TC_KEY_M,
	TC_KEY_N,
	TC_KEY_O,
	TC_KEY_P,
	TC_KEY_Q,
	TC_KEY_R,
	TC_KEY_S,
	TC_KEY_T,
	TC_KEY_U,
	TC_KEY_V,
	TC_KEY_W,
	TC_KEY_X,
	TC_KEY_Y,
	TC_KEY_Z,
	TC_KEY_LEFT_BRACKET,  /* [ */
	TC_KEY_BACKSLASH,	  /* \ */
	TC_KEY_RIGHT_BRACKET, /* ] */
	TC_KEY_GRAVE_ACCENT,  /* ` */
	TC_KEY_WORLD_1,		  /* non-US # */
	TC_KEY_WORLD_2,		  /* non-US #2 */

	/* Function keys */
	TC_KEY_ESCAPE,
	TC_KEY_ENTER,
	TC_KEY_TAB,
	TC_KEY_BACKSPACE,
	TC_KEY_INSERT,
	TC_KEY_DELETE,
	TC_KEY_RIGHT,
	TC_KEY_LEFT,
	TC_KEY_DOWN,
	TC_KEY_UP,
	TC_KEY_PAGE_UP,
	TC_KEY_PAGE_DOWN,
	TC_KEY_HOME,
	TC_KEY_END,
	TC_KEY_CAPS_LOCK,
	TC_KEY_SCROLL_LOCK,
	TC_KEY_NUM_LOCK,
	TC_KEY_PRINT_SCREEN,
	TC_KEY_PAUSE,
	TC_KEY_F1,
	TC_KEY_F2,
	TC_KEY_F3,
	TC_KEY_F4,
	TC_KEY_F5,
	TC_KEY_F6,
	TC_KEY_F7,
	TC_KEY_F8,
	TC_KEY_F9,
	TC_KEY_F10,
	TC_KEY_F11,
	TC_KEY_F12,
	TC_KEY_F13,
	TC_KEY_F14,
	TC_KEY_F15,
	TC_KEY_F16,
	TC_KEY_F17,
	TC_KEY_F18,
	TC_KEY_F19,
	TC_KEY_F20,
	TC_KEY_F21,
	TC_KEY_F22,
	TC_KEY_F23,
	TC_KEY_F24,
	TC_KEY_F25,
	TC_KEY_KP_0,
	TC_KEY_KP_1,
	TC_KEY_KP_2,
	TC_KEY_KP_3,
	TC_KEY_KP_4,
	TC_KEY_KP_5,
	TC_KEY_KP_6,
	TC_KEY_KP_7,
	TC_KEY_KP_8,
	TC_KEY_KP_9,
	TC_KEY_KP_DECIMAL,
	TC_KEY_KP_DIVIDE,
	TC_KEY_KP_MULTIPLY,
	TC_KEY_KP_SUBTRACT,
	TC_KEY_KP_ADD,
	TC_KEY_KP_ENTER,
	TC_KEY_KP_EQUAL,
	TC_KEY_LEFT_SHIFT,
	TC_KEY_LEFT_CONTROL,
	TC_KEY_LEFT_ALT,
	TC_KEY_LEFT_SUPER,
	TC_KEY_RIGHT_SHIFT,
	TC_KEY_RIGHT_CONTROL,
	TC_KEY_RIGHT_ALT,
	TC_KEY_RIGHT_SUPER,
	TC_KEY_MENU,
	TC_KEY_MOUSE_LEFT,
	TC_KEY_MOUSE_MIDDLE,
	TC_KEY_MOUSE_RIGHT,
	TC_KEY_MAX_ENUM
} TCKey;

typedef enum TCKeyAction
{
	TC_KEYACTION_RELEASE,
	TC_KEYACTION_PRESS,
	TC_KEYACTION_REPEAT
} TCKeyAction;

typedef enum TCKeyModifier
{
	TC_KEYMODIFIER_NONE,
	TC_KEYMODIFIER_SHIFT,
	TC_KEYMODIFIER_CONTROL,
	TC_KEYMODIFIER_ALT,
	TC_KEYMODIFIER_SUPER,
	TC_KEYMODIFIER_CAPSLOCK,
	TC_KEYMODIFIER_NUMLOCK
} TCKeyModifier;

typedef enum TCCursorMode
{
	TC_CURSORMODE_NORMAL,
	TC_CURSORMODE_HIDDEN,
	TC_CURSORMODE_DISABLED,
	TC_CURSORMODE_RAW
} TCCursorMode;

typedef void (*TCWindowResizeCallback)(TCWindow windowHnd,
									   void* userPtr,
									   unsigned int newWidth,
									   unsigned int newHeight);
// @param scanCode: System-specific scan code
typedef void (*TCWindowKeyCallback)(
	TCWindow windowHnd, void* userPointer, TCKey key, int scanCode, TCKeyAction action, TCKeyModifier modifier);
typedef void (*TCWindowCloseCallback)(TCWindow windowHnd);

typedef struct TCWindowDescription
{
	unsigned int Width, Height;
	TCMonitor Monitor;
	TCWindowMode Mode;
	const char* Name;
	TCWindowResizeCallback ResizeCallback;
	TCWindowKeyCallback KeyCallback;
	TCWindowCloseCallback CloseCallback;
} TCWindowDescription;

typedef struct ITCWindowing
{
	// Create a new window. The window is not visible until ShowWindow() is called.
	void (*CreateWindow)(const TCWindowDescription* desc, void* user, TCWindow* outWindowHnd);
	void (*GetMonitorInfos)(TU8* monitorCount, TCMonitorInfo* monitorInfos);
	void (*DestroyWindow)(TCWindow window);
} ITCWindowing;

TCORE_END_C_LINKAGE