#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"

// This class is to manage windows easier
typedef struct tgfx_imgui_window {
  unsigned char isWindowOpen;
  const char*   WindowName;
  void (*RunWindow)(struct tgfx_imgui_window* windowdata);
  void* userdata;
} imguiWindow_tgfx;

typedef struct tgfx_dearimgui {
  // WINDOW MANAGEMENT

  void (*Run_IMGUI_WINDOWs)();
  void (*Register_WINDOW)(imguiWindow_tgfx* WINDOW);
  void (*Delete_WINDOW)(imguiWindow_tgfx* WINDOW);

  // INITIALIZATION

  unsigned char (*Check_IMGUI_Version)();
  void (*Destroy_IMGUI_Resources)();
  void (*Set_as_MainViewport)();

  unsigned char (*Show_DemoWindow)();
  unsigned char (*Show_MetricsWindow)();

  // IMGUI FUNCTIONALITY!

  unsigned char (*Create_Window)(const char* title, unsigned char* should_close,
                                 unsigned char has_menubar);
  void (*End_Window)();
  void (*Text)(const char* text);
  unsigned char (*Button)(const char* button_name);
  unsigned char (*Checkbox)(const char* name, unsigned char* variable);
  // This is not const char*, because dear ImGui needs a buffer to set a char!
  unsigned char (*Input_Text)(const char* name, char** Text);
  // Create a menubar for a IMGUI window!
  unsigned char (*Begin_Menubar)();
  void (*End_Menubar)();
  // Create a menu button! Returns if it is clicked!
  unsigned char (*Begin_Menu)(const char* name);
  void (*End_Menu)();
  // Create a item for a menu!
  unsigned char (*Menu_Item)(const char* name);
  // Write a paragraph text!
  unsigned char (*Input_Paragraph_Text)(const char* name, char** Text);
  // Put the next item to the same line with last created item
  // Use between after last item's end - before next item's begin!
  void (*Same_Line)();
  unsigned char (*Begin_Tree)(const char* name);
  void (*End_Tree)();
  // Create a box of selectable items in one-line of code!
  // @return 1: Any item is selected.
  //		Selected item's index is the selected_index's pointer's value!
  unsigned char (*SelectList_OneLine)(const char* name, unsigned int* selected_index,
                                      const char* const* names);
  void (*Selectable)(const char* name, unsigned char* is_selected);
  // Create a box of selectable items in one-line of code!
  // @return 1: Any item is selected.
  //		Selected item's index is the selected_index's pointer's value!
  unsigned char (*Selectable_ListBox)(const char* name, int* selected_index,
                                      const char* const* item_names);
  void (*Display_Texture)(texture_tgfxhnd hnd, unsigned int width, unsigned int height,
                          unsigned char should_Flip_Vertically);
  unsigned char (*Begin_TabBar)();
  void (*End_TabBar)();
  unsigned char (*Begin_TabItem)(const char* name);
  void (*End_TabItem)();
  void (*Separator)();
  vec2_tgfx (*GetLastItemRectMin)();
  vec2_tgfx (*GetLastItemRectMax)();
  vec2_tgfx (*GetItemWindowPos)();
  vec2_tgfx (*GetMouseWindowPos)();

  // Add here UInt, UShort & Short, UChar & Char sliders too!
  /////////////////////////////////////////////////////////

  unsigned char (*Slider_Int)(const char* name, int* data, int min, int max);
  unsigned char (*Slider_Float)(const char* name, float* data, float min, float max);
  unsigned char (*Slider_Vec2)(const char* name, vec2_tgfx* data, float min, float max);
  unsigned char (*Slider_Vec3)(const char* name, vec3_tgfx* data, float min, float max);
  unsigned char (*Slider_Vec4)(const char* name, vec4_tgfx* data, float min, float max);
} dearimgui_tgfx;
