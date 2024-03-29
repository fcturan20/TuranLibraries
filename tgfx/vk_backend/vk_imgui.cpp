#include "vk_imgui.h"

#include <imgui.h>
#include <stdio.h>
#include <tgfx_core.h>
#include <tgfx_imgui.h>

#include "vk_core.h"
#include "vk_queue.h"
#include "vk_renderer.h"
#ifndef NO_IMGUI

unsigned char Check_IMGUI_Version();
void          Destroy_IMGUI_Resources();
void          Set_as_MainViewport();

unsigned char Show_DemoWindow();
unsigned char Show_MetricsWindow();

// IMGUI FUNCTIONALITY!
unsigned char Create_Window(const char* title, unsigned char* should_close,
                            unsigned char has_menubar);
void          End_Window();
void          Text(const char* text);
unsigned char Button(const char* button_name);
unsigned char Checkbox(const char* name, unsigned char* variable);
// void CheckListBox(const char* name, Bitset items_status, const char* const* item_names);
// This is not const char*, because dear ImGui needs a buffer to set a char!
unsigned char Input_Text(const char* name, char** Text);
// Create a menubar for a IMGUI window!
unsigned char Begin_Menubar();
void          End_Menubar();
// Create a menu button! Returns if it is clicked!
unsigned char Begin_Menu(const char* name);
void          End_Menu();
// Create a item for a menu!
unsigned char Menu_Item(const char* name);
// Write a paragraph text!
unsigned char Input_Paragraph_Text(const char* name, char** Text);
// Put the next item to the same line with last created item
// Use between after last item's end - before next item's begin!
void          Same_Line();
unsigned char Begin_Tree(const char* name);
void          End_Tree();
// Create a select list that extends when clicked and get the selected_index in one-line of code!
// Returns if any item is selected in the list! Selected item's index is the selected_index's
// pointer's value!
unsigned char SelectList_OneLine(const char* name, unsigned int* selected_index,
                                 const char* const* item_names);
void          Selectable(const char* name, unsigned char* is_selected);
// Create a box of selectable items in one-line of code!
// Returns if any item is selected in the list! Selected item's index is the selected_index's
// pointer's value!
unsigned char Selectable_ListBox(const char* name, int* selected_index,
                                 const char* const* item_names);
// Display a texture that is in the GPU memory, for example a Render Target or a Texture
// void Display_Texture(tgfx_texture TextureHandle, unsigned int Display_WIDTH, unsigned int
// Display_HEIGHT, unsigned char should_Flip_Vertically);
unsigned char Begin_TabBar();
void          End_TabBar();
unsigned char Begin_TabItem(const char* name);
void          End_TabItem();
void          Separator();
vec2_tgfx     GetLastItemRectMin();
vec2_tgfx     GetLastItemRectMax();
vec2_tgfx     GetItemWindowPos();
vec2_tgfx     GetMouseWindowPos();

// Add here Unsigned Int, Unsigned Short & Short, Unsigned Char & Char sliders too!

unsigned char Slider_Int(const char* name, int* data, int min, int max);
unsigned char Slider_Float(const char* name, float* data, float min, float max);
unsigned char Slider_Vec2(const char* name, vec2_tgfx* data, float min, float max);
unsigned char Slider_Vec3(const char* name, vec3_tgfx* data, float min, float max);
unsigned char Slider_Vec4(const char* name, vec4_tgfx* data, float min, float max);

void Delete_WINDOW(imguiWindow_tgfx* Window);
void Register_WINDOW(imguiWindow_tgfx* Window);
void Run_IMGUI_WINDOWs();

void set_imguifuncptrs() {
  core_tgfx_main->imgui->Begin_Menu    = Begin_Menu;
  core_tgfx_main->imgui->Begin_Menubar = Begin_Menubar;
  core_tgfx_main->imgui->Begin_TabBar  = Begin_TabBar;
  core_tgfx_main->imgui->Begin_TabItem = Begin_TabItem;
  core_tgfx_main->imgui->Begin_Tree    = Begin_Tree;
  core_tgfx_main->imgui->Button        = Button;
  core_tgfx_main->imgui->Checkbox      = Checkbox;
  // core_tgfx_main->imgui->CheckListBox = CheckListBox;
  core_tgfx_main->imgui->Check_IMGUI_Version     = Check_IMGUI_Version;
  core_tgfx_main->imgui->Create_Window           = Create_Window;
  core_tgfx_main->imgui->Destroy_IMGUI_Resources = Destroy_IMGUI_Resources;
  // core_tgfx_main->imgui->Display_Texture = Display_Texture;
  core_tgfx_main->imgui->End_Menu             = End_Menu;
  core_tgfx_main->imgui->End_Menubar          = End_Menubar;
  core_tgfx_main->imgui->End_TabBar           = End_TabBar;
  core_tgfx_main->imgui->End_TabItem          = End_TabItem;
  core_tgfx_main->imgui->End_Tree             = End_Tree;
  core_tgfx_main->imgui->End_Window           = End_Window;
  core_tgfx_main->imgui->GetItemWindowPos     = GetItemWindowPos;
  core_tgfx_main->imgui->GetLastItemRectMax   = GetLastItemRectMax;
  core_tgfx_main->imgui->GetLastItemRectMin   = GetLastItemRectMin;
  core_tgfx_main->imgui->GetMouseWindowPos    = GetMouseWindowPos;
  core_tgfx_main->imgui->Input_Paragraph_Text = Input_Paragraph_Text;
  core_tgfx_main->imgui->Input_Text           = Input_Text;
  core_tgfx_main->imgui->Menu_Item            = Menu_Item;
  core_tgfx_main->imgui->Same_Line            = Same_Line;
  core_tgfx_main->imgui->Selectable           = Selectable;
  core_tgfx_main->imgui->Selectable_ListBox   = Selectable_ListBox;
  core_tgfx_main->imgui->SelectList_OneLine   = SelectList_OneLine;
  core_tgfx_main->imgui->Separator            = Separator;
  core_tgfx_main->imgui->Set_as_MainViewport  = Set_as_MainViewport;
  core_tgfx_main->imgui->Show_DemoWindow      = Show_DemoWindow;
  core_tgfx_main->imgui->Show_MetricsWindow   = Show_MetricsWindow;
  core_tgfx_main->imgui->Slider_Float         = Slider_Float;
  core_tgfx_main->imgui->Slider_Int           = Slider_Int;
  core_tgfx_main->imgui->Slider_Vec2          = Slider_Vec2;
  core_tgfx_main->imgui->Slider_Vec3          = Slider_Vec3;
  core_tgfx_main->imgui->Slider_Vec4          = Slider_Vec4;
  core_tgfx_main->imgui->Text                 = Text;

  core_tgfx_main->imgui->Run_IMGUI_WINDOWs = Run_IMGUI_WINDOWs;
  core_tgfx_main->imgui->Register_WINDOW   = Register_WINDOW;
  core_tgfx_main->imgui->Delete_WINDOW     = Delete_WINDOW;
}

void CheckIMGUIVKResults(VkResult result) {
  if (result != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "IMGUI's Vulkan backend has failed, please report!");
  }
}
void imgui_vk::Change_DrawPass(renderSubPass_tgfxhnd Subpass) {
  printer(result_tgfx_NOTCODED, "imgui_vk->ChangeDrawPass() isn't coded");
}
void imgui_vk::NewFrame() {
  if (STAT == IMGUI_STATUS::NEW_FRAME) {
    printer(
      result_tgfx_WARNING,
      "You have mismatching IMGUI_VK calls, NewFrame() called twice without calling Render()!");
    return;
  }
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  STAT = IMGUI_STATUS::NEW_FRAME;
}

void imgui_vk::Render_AdditionalWindows() {
  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
}
void imgui_vk::Render_toCB(VkCommandBuffer cb) {
  if (STAT == IMGUI_STATUS::RENDERED) {
    return;
  }
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
  STAT = IMGUI_STATUS::RENDERED;
}

void imgui_vk::UploadFontTextures() {
  // Upload font textures
  VkCommandPoolCreateInfo cp_ci = {};
  cp_ci.flags                   = 0;
  cp_ci.pNext                   = nullptr;
  cp_ci.queueFamilyIndex = queuesys->get_queuefam_index(rendergpu, rendergpu->GRAPHICSQUEUEFAM());
  cp_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  VkCommandPool cp;
  if (vkCreateCommandPool(rendergpu->devLogical, &cp_ci, nullptr, &cp) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "Creation of Command Pool for dear IMGUI Font Upload has failed!");
  }

  VkCommandBufferAllocateInfo cb_ai = {};
  cb_ai.commandBufferCount          = 1;
  cb_ai.commandPool                 = cp;
  cb_ai.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cb_ai.pNext                       = nullptr;
  cb_ai.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  VkCommandBuffer cb;
  if (vkAllocateCommandBuffers(rendergpu->devLogical, &cb_ai, &cb) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "Creation of Command Buffer for dear IMGUI Font Upload has failed!");
  }

  if (vkResetCommandPool(rendergpu->devLogical, cp,
                         VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "Resetting of Command Pool for dear IMGUI Font Upload has failed!");
  }
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(cb, &begin_info) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "vkBeginCommandBuffer() for dear IMGUI Font Upload has failed!");
  }

  ImGui_ImplVulkan_CreateFontsTexture(cb);

  VkPipelineStageFlags flag = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "vkEndCommandBuffer() for dear IMGUI Font Upload has failed!");
  }
  if (queuesys->queueSubmit(rendergpu, rendergpu->GRAPHICSQUEUEFAM(), {}, {}, &cb, &flag, 1) ==
      invalid_fenceid) {
    printer(result_tgfx_FAIL, "VkQueueSubmit() for dear IMGUI Font Upload has failed!");
  }

  if (vkDeviceWaitIdle(rendergpu->devLogical) != VK_SUCCESS) {
    printer(result_tgfx_FAIL, "vkDeviceWaitIdle() for dear IMGUI Font Upload has failed!");
  }
  ImGui_ImplVulkan_DestroyFontUploadObjects();

  if (vkResetCommandPool(rendergpu->devLogical, cp, 0) != VK_SUCCESS) {
    printer(result_tgfx_FAIL,
            "Resetting of Command Pool for destruction of dear IMGUI Font Upload has failed!");
  }
  vkDestroyCommandPool(rendergpu->devLogical, cp, nullptr);
}

void imgui_vk::Destroy_IMGUIResources() {
  // Resources to destroy when the program ends
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

struct imgui_vk::imgui_vk_hidden {
  ImGuiContext* Context = nullptr;

  std::vector<imguiWindow_tgfx*> ALL_IMGUI_WINDOWs, Windows_toClose, Windows_toOpen;
};
static imgui_vk::imgui_vk_hidden* hidden = nullptr;

extern void Create_IMGUI() {
  imgui         = new imgui_vk;
  imgui->hidden = new imgui_vk::imgui_vk_hidden;
  hidden        = imgui->hidden;
  set_imguifuncptrs();
  imgui->STAT = imgui_vk::IMGUI_STATUS::UNINITIALIZED;

  // Create Context here!
  IMGUI_CHECKVERSION();
  hidden->Context = ImGui::CreateContext();
  if (hidden->Context == nullptr) {
    printer(result_tgfx_FAIL, "dear ImGui Context is nullptr after creation!");
    delete imgui->hidden;
    delete imgui;
    imgui  = nullptr;
    hidden = nullptr;
    return;
  }

  // Set Input Handling settings here!
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Set color style to dark by default for now!
  ImGui::StyleColorsDark();
}

void Run_IMGUI_WINDOWs() {
  for (unsigned int i = 0; i < hidden->ALL_IMGUI_WINDOWs.size(); i++) {
    imguiWindow_tgfx* window = hidden->ALL_IMGUI_WINDOWs[i];
    window->RunWindow(window);
  }

  if (hidden->Windows_toClose.size() > 0) {
    for (unsigned int window_delete_i = 0; window_delete_i < hidden->Windows_toClose.size();
         window_delete_i++) {
      imguiWindow_tgfx* window_to_close = hidden->Windows_toClose[window_delete_i];
      for (unsigned int deleted_window_main_i = 0;
           deleted_window_main_i < hidden->ALL_IMGUI_WINDOWs.size(); deleted_window_main_i++) {
        if (window_to_close == hidden->ALL_IMGUI_WINDOWs[deleted_window_main_i]) {
          hidden->ALL_IMGUI_WINDOWs.erase(hidden->ALL_IMGUI_WINDOWs.begin() +
                                          deleted_window_main_i);
          // Inner loop will break because we found the deleted window in the main list!
          break;
        }
      }
    }

    hidden->Windows_toClose.clear();
  }

  if (hidden->Windows_toOpen.size() > 0) {
    unsigned int previous_size = hidden->ALL_IMGUI_WINDOWs.size();

    for (unsigned int i = 0; i < hidden->Windows_toOpen.size(); i++) {
      imguiWindow_tgfx* window_to_open = hidden->Windows_toOpen[i];
      hidden->ALL_IMGUI_WINDOWs.push_back(window_to_open);
    }
    hidden->Windows_toOpen.clear();
    // We should run new windows!
    for (previous_size; previous_size < hidden->ALL_IMGUI_WINDOWs.size(); previous_size++) {
      hidden->ALL_IMGUI_WINDOWs[previous_size]->RunWindow(hidden->ALL_IMGUI_WINDOWs[previous_size]);
    }
  }
}
void Register_WINDOW(imguiWindow_tgfx* Window) { hidden->Windows_toOpen.push_back(Window); }
void Delete_WINDOW(imguiWindow_tgfx* Window) { hidden->Windows_toClose.push_back(Window); }

unsigned char Check_IMGUI_Version() {
  // Check version here, I don't care for now!
  return IMGUI_CHECKVERSION();
}

void Set_as_MainViewport() {
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
}

void Destroy_IMGUI_Resources() { ImGui::DestroyContext(); }

unsigned char Show_DemoWindow() {
  bool x = true;
  ImGui::ShowDemoWindow(&x);
  return x;
}

unsigned char Show_MetricsWindow() {
  bool x = true;
  ImGui::ShowMetricsWindow(&x);
  return x;
}

unsigned char Create_Window(const char* title, unsigned char* should_close,
                            unsigned char has_menubar) {
  bool             close_boolean = should_close;
  ImGuiWindowFlags window_flags  = 0;
  window_flags |= (has_menubar ? ImGuiWindowFlags_MenuBar : 0);
  bool result   = ImGui::Begin(title, &close_boolean, window_flags);
  *should_close = close_boolean;
  return result;
}

void End_Window() { ImGui::End(); }

void Text(const char* text) { ImGui::Text(text); }

unsigned char Button(const char* button_name) { return ImGui::Button(button_name); }

unsigned char Checkbox(const char* name, unsigned char* variable) {
  bool x      = *variable;
  bool result = ImGui::Checkbox(name, &x);
  *variable   = x;
  return result;
}

unsigned char Input_Text(const char* name, char** text) {
  printer(result_tgfx_NOTCODED, "Vulkan::tgfx_imguicore->Input_Text() is not coded!");
  /*
  if (ImGui::InputText(name, text, ImGuiInputTextFlags_EnterReturnsTrue)) {
    return true;
  }*/
  return false;
}

unsigned char Begin_Menubar() { return ImGui::BeginMenuBar(); }

void End_Menubar() { ImGui::EndMenuBar(); }

unsigned char Begin_Menu(const char* name) { return ImGui::BeginMenu(name); }

void End_Menu() { ImGui::EndMenu(); }

unsigned char Menu_Item(const char* name) { return ImGui::MenuItem(name); }

unsigned char Input_Paragraph_Text(const char* name, char** Text) {
  printer(result_tgfx_NOTCODED, "Vulkan::IMGUI::Input_Paragraph_Text() isn't coded!");
  /*
  if (ImGui::InputTextMultiline(name, Text, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue)) {
    return true;
  }*/
  return false;
}

// Puts the next item to the same line with last created item
// Use between after last item's end - before next item's begin!
void Same_Line() { ImGui::SameLine(); }

unsigned char Begin_Tree(const char* name) { return ImGui::TreeNode(name); }

void End_Tree() { ImGui::TreePop(); }

unsigned char SelectList_OneLine(const char* name, unsigned int* selected_index,
                                 const char* const* item_names) {
  unsigned char is_new_item_selected = false;
  const char*   preview_str          = item_names[*selected_index];
  if (ImGui::BeginCombo(name, preview_str)) // The second parameter is the index of the label
                                            // previewed before opening the combo.
  {
    unsigned int i = 0;
    while (item_names[i] != nullptr) {
      unsigned char is_selected = (*selected_index == i);
      if (ImGui::Selectable(item_names[i], is_selected)) {
        *selected_index      = i;
        is_new_item_selected = true;
      }
      i++;
    }
    ImGui::EndCombo();
  }
  return is_new_item_selected;
}

// If selected, argument "is_selected" is set to its opposite!
void Selectable(const char* name, unsigned char* is_selected) {
  ImGui::Selectable(name, is_selected);
}

unsigned char Selectable_ListBox(const char* name, int* selected_index,
                                 const char* const* item_names) {
  int           already_selected_index = *selected_index;
  unsigned char is_new_selected        = false;
  if (ImGui::ListBoxHeader(name)) {
    unsigned int i = 0;
    while (item_names[i] != nullptr) {
      unsigned char is_selected = false;
      const char*   item_name   = item_names[i];
      Selectable(item_name, &is_selected);
      if (is_selected && (already_selected_index != i)) {
        *selected_index = i;
        is_new_selected = true;
      }
      i++;
    }

    ImGui::ListBoxFooter();
  }
  return is_new_selected;
}
/*
void CheckListBox(const char* name, Bitset items_status, const char* const* item_names) {
  if (ImGui::ListBoxHeader(name)) {
    unsigned int i = 0;
    while (item_names[i] != nullptr) {
      unsigned char x = GetBit_Value(items_status, i);
      Checkbox(item_names[i], &x);
      x ? SetBit_True(items_status, i) : SetBit_False(items_status, i);
      printer(result_tgfx_SUCCESS, ("Current Index: " + std::to_string(i)).c_str());
      printer(result_tgfx_SUCCESS, ("Current Name: " + std::string(item_names[i])).c_str());
      printer(result_tgfx_SUCCESS, ("Current Value: " + std::to_string(x)).c_str());
      i++;
    }
    ImGui::ListBoxFooter();
  }
}*/

unsigned char Begin_TabBar() { return ImGui::BeginTabBar(""); }
void          End_TabBar() { ImGui::EndTabBar(); }
unsigned char Begin_TabItem(const char* name) { return ImGui::BeginTabItem(name); }
void          End_TabItem() { ImGui::EndTabItem(); }
void          Separator() { ImGui::Separator(); }

vec2_tgfx GetLastItemRectMin() {
  vec2_tgfx vecvar;
  vecvar.x = ImGui::GetItemRectMin().x;
  vecvar.y = ImGui::GetItemRectMin().y;
  return vecvar;
}
vec2_tgfx GetLastItemRectMax() {
  vec2_tgfx vecvar;
  vecvar.x = ImGui::GetItemRectMax().x;
  vecvar.y = ImGui::GetItemRectMax().y;
  return vecvar;
}
vec2_tgfx GetItemWindowPos() {
  vec2_tgfx vecvar;
  vecvar.x = ImGui::GetCursorScreenPos().x;
  vecvar.y = ImGui::GetCursorScreenPos().y;
  return vecvar;
}
vec2_tgfx GetMouseWindowPos() {
  vec2_tgfx vecvar;
  vecvar.x = ImGui::GetMousePos().x;
  vecvar.y = ImGui::GetMousePos().y;
  return vecvar;
}

unsigned char Slider_Int(const char* name, int* data, int min, int max) {
  return ImGui::SliderInt(name, data, min, max);
}
unsigned char Slider_Float(const char* name, float* data, float min, float max) {
  return ImGui::SliderFloat(name, data, min, max);
}
unsigned char Slider_Vec2(const char* name, vec2_tgfx* data, float min, float max) {
  return ImGui::SliderFloat2(name, ( float* )data, min, max);
}
unsigned char Slider_Vec3(const char* name, vec3_tgfx* data, float min, float max) {
  return ImGui::SliderFloat3(name, ( float* )data, min, max);
}
unsigned char Slider_Vec4(const char* name, vec4_tgfx* data, float min, float max) {
  return ImGui::SliderFloat4(name, ( float* )data, min, max);
}

#endif // !NO_IMGUI
