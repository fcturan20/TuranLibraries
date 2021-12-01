#pragma once
#include "GFX_Includes.h"


#if defined(__cplusplus)
extern "C" {
#endif

//This 2 class is to manage windows easier
typedef struct tgfx_IMGUI_WINDOW {
	unsigned char isWindowOpen;
	const char* WindowName;
	void (*RunWindow)();
} tgfx_IMGUI_WINDOW;


typedef struct tgfx_IMGUI_WindowManager {
	void (*Run_IMGUI_WINDOWs)();
	void (*Register_WINDOW)(tgfx_IMGUI_WINDOW* WINDOW);
	void (*Delete_WINDOW)(tgfx_IMGUI_WINDOW* WINDOW);
} tgfx_IMGUI_WindowManager;


typedef struct tgfx_imguicore {
	tgfx_IMGUI_WindowManager WindowManager;
	unsigned char (*Check_IMGUI_Version)();
	void (*Destroy_IMGUI_Resources)();
	void (*Set_as_MainViewport)();

	unsigned char (*Show_DemoWindow)();
	unsigned char (*Show_MetricsWindow)();

	//IMGUI FUNCTIONALITY!
	unsigned char (*Create_Window)(const char* title, unsigned char should_close, unsigned char has_menubar);
	void (*End_Window)();
	void (*Text)(const char* text);
	unsigned char (*Button)(const char* button_name);
	unsigned char (*Checkbox)(const char* name, unsigned char* variable);
	//This is not const char*, because dear ImGui needs a buffer to set a char!
	unsigned char (*Input_Text)(const char* name, char** Text);
	//Create a menubar for a IMGUI window!
	unsigned char (*Begin_Menubar)();
	void (*End_Menubar)();
	//Create a menu button! Returns if it is clicked!
	unsigned char (*Begin_Menu)(const char* name);
	void (*End_Menu)();
	//Create a item for a menu! 
	unsigned char (*Menu_Item)(const char* name);
	//Write a paragraph text!
	unsigned char (*Input_Paragraph_Text)(const char* name, char** Text);
	//Put the next item to the same line with last created item
	//Use between after last item's end - before next item's begin!
	void (*Same_Line)();
	unsigned char (*Begin_Tree)(const char* name);
	void (*End_Tree)();
	//Create a select list that extends when clicked and get the selected_index in one-line of code!
	//Returns if any item is selected in the list! Selected item's index is the selected_index's pointer's value!
	unsigned char (*SelectList_OneLine)(const char* name, unsigned int* selected_index, const char* const* item_names);
	void (*Selectable)(const char* name, unsigned char* is_selected);
	//Create a box of selectable items in one-line of code!
	//Returns if any item is selected in the list! Selected item's index is the selected_index's pointer's value!
	unsigned char (*Selectable_ListBox)(const char* name, int* selected_index, const char* const* item_names);
	//Display a texture that is in the GPU memory, for example a Render Target or a Texture
	void (*Display_Texture)(tgfx_texture TextureHandle, unsigned int Display_WIDTH, unsigned int Display_HEIGHT, unsigned char should_Flip_Vertically);
	unsigned char (*Begin_TabBar)();
	void (*End_TabBar)();
	unsigned char (*Begin_TabItem)(const char* name);
	void (*End_TabItem)();
	void (*Separator)();
	tgfx_vec2 (*GetLastItemRectMin)();
	tgfx_vec2 (*GetLastItemRectMax)();
	tgfx_vec2 (*GetItemWindowPos)();
	tgfx_vec2 (*GetMouseWindowPos)();

	//Add here Unsigned Int, Unsigned Short & Short, Unsigned Char & Char sliders too!

	unsigned char (*Slider_Int)(const char* name, int* data, int min, int max);
	unsigned char (*Slider_Float)(const char* name, float* data, float min, float max);
	unsigned char (*Slider_Vec2)(const char* name, tgfx_vec2* data, float min, float max);
	unsigned char (*Slider_Vec3)(const char* name, tgfx_vec3* data, float min, float max);
	unsigned char (*Slider_Vec4)(const char* name, tgfx_vec4* data, float min, float max);


} tgfx_imguicore;


#if defined(__cplusplus)
}
#endif