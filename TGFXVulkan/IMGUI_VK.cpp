#include "IMGUI_VK.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Vulkan_Core.h"
#include "Vulkan_Includes.h"
#include <iostream>
#define LOG VKCORE->TGFXCORE.DebugCallback
static ImGuiContext* Context = nullptr;


unsigned char Check_IMGUI_Version();
void Destroy_IMGUI_Resources();
void Set_as_MainViewport();

unsigned char Show_DemoWindow();
unsigned char Show_MetricsWindow();

//IMGUI FUNCTIONALITY!
unsigned char Create_Window(const char* title, unsigned char should_close, unsigned char has_menubar);
void End_Window();
void Text(const char* text);
unsigned char Button(const char* button_name);
unsigned char Checkbox(const char* name, unsigned char* variable);
//void CheckListBox(const char* name, Bitset items_status, const char* const* item_names);
//This is not const char*, because dear ImGui needs a buffer to set a char!
unsigned char Input_Text(const char* name, char** Text);
//Create a menubar for a IMGUI window!
unsigned char Begin_Menubar();
void End_Menubar();
//Create a menu button! Returns if it is clicked!
unsigned char Begin_Menu(const char* name);
void End_Menu();
//Create a item for a menu! 
unsigned char Menu_Item(const char* name);
//Write a paragraph text!
unsigned char Input_Paragraph_Text(const char* name, char** Text);
//Put the next item to the same line with last created item
//Use between after last item's end - before next item's begin!
void Same_Line();
unsigned char Begin_Tree(const char* name);
void End_Tree();
//Create a select list that extends when clicked and get the selected_index in one-line of code!
//Returns if any item is selected in the list! Selected item's index is the selected_index's pointer's value!
unsigned char SelectList_OneLine(const char* name, unsigned int* selected_index, const char* const* item_names);
void Selectable(const char* name, unsigned char* is_selected);
//Create a box of selectable items in one-line of code!
//Returns if any item is selected in the list! Selected item's index is the selected_index's pointer's value!
unsigned char Selectable_ListBox(const char* name, int* selected_index, const char* const* item_names);
//Display a texture that is in the GPU memory, for example a Render Target or a Texture
//void Display_Texture(tgfx_texture TextureHandle, unsigned int Display_WIDTH, unsigned int Display_HEIGHT, unsigned char should_Flip_Vertically);
unsigned char Begin_TabBar();
void End_TabBar();
unsigned char Begin_TabItem(const char* name);
void End_TabItem();
void Separator();
tgfx_vec2 GetLastItemRectMin();
tgfx_vec2 GetLastItemRectMax();
tgfx_vec2 GetItemWindowPos();
tgfx_vec2 GetMouseWindowPos();

//Add here Unsigned Int, Unsigned Short & Short, Unsigned Char & Char sliders too!

unsigned char Slider_Int(const char* name, int* data, int min, int max);
unsigned char Slider_Float(const char* name, float* data, float min, float max);
unsigned char Slider_Vec2(const char* name, tgfx_vec2* data, float min, float max);
unsigned char Slider_Vec3(const char* name, tgfx_vec3* data, float min, float max);
unsigned char Slider_Vec4(const char* name, tgfx_vec4* data, float min, float max);



void CheckIMGUIVKResults(VkResult result) {
	if (result != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "IMGUI's Vulkan backend has failed, please report!");
	}
}

void Delete_WINDOW(tgfx_IMGUI_WINDOW* Window);
void Register_WINDOW(tgfx_IMGUI_WINDOW* Window);
void Run_IMGUI_WINDOWs();

void Set_IMGUIFunctionPTRs() {
	VKCORE->TGFXCORE.IMGUI.Begin_Menu = Begin_Menu;
	VKCORE->TGFXCORE.IMGUI.Begin_Menubar = Begin_Menubar;
	VKCORE->TGFXCORE.IMGUI.Begin_TabBar = Begin_TabBar;
	VKCORE->TGFXCORE.IMGUI.Begin_TabItem = Begin_TabItem;
	VKCORE->TGFXCORE.IMGUI.Begin_Tree = Begin_Tree;
	VKCORE->TGFXCORE.IMGUI.Button = Button;
	VKCORE->TGFXCORE.IMGUI.Checkbox = Checkbox;
	//VKCORE->TGFXCORE.IMGUI.CheckListBox = CheckListBox;
	VKCORE->TGFXCORE.IMGUI.Check_IMGUI_Version = Check_IMGUI_Version;
	VKCORE->TGFXCORE.IMGUI.Create_Window = Create_Window;
	VKCORE->TGFXCORE.IMGUI.Destroy_IMGUI_Resources = Destroy_IMGUI_Resources;
	//VKCORE->TGFXCORE.IMGUI.Display_Texture = Display_Texture;
	VKCORE->TGFXCORE.IMGUI.End_Menu = End_Menu;
	VKCORE->TGFXCORE.IMGUI.End_Menubar = End_Menubar;
	VKCORE->TGFXCORE.IMGUI.End_TabBar = End_TabBar;
	VKCORE->TGFXCORE.IMGUI.End_TabItem = End_TabItem;
	VKCORE->TGFXCORE.IMGUI.End_Tree = End_Tree;
	VKCORE->TGFXCORE.IMGUI.End_Window = End_Window;
	VKCORE->TGFXCORE.IMGUI.GetItemWindowPos = GetItemWindowPos;
	VKCORE->TGFXCORE.IMGUI.GetLastItemRectMax = GetLastItemRectMax;
	VKCORE->TGFXCORE.IMGUI.GetLastItemRectMin = GetLastItemRectMin;
	VKCORE->TGFXCORE.IMGUI.GetMouseWindowPos = GetMouseWindowPos;
	VKCORE->TGFXCORE.IMGUI.Input_Paragraph_Text = Input_Paragraph_Text;
	VKCORE->TGFXCORE.IMGUI.Input_Text = Input_Text;
	VKCORE->TGFXCORE.IMGUI.Menu_Item = Menu_Item;
	VKCORE->TGFXCORE.IMGUI.Same_Line = Same_Line;
	VKCORE->TGFXCORE.IMGUI.Selectable = Selectable;
	VKCORE->TGFXCORE.IMGUI.Selectable_ListBox = Selectable_ListBox;
	VKCORE->TGFXCORE.IMGUI.SelectList_OneLine = SelectList_OneLine;
	VKCORE->TGFXCORE.IMGUI.Separator = Separator;
	VKCORE->TGFXCORE.IMGUI.Set_as_MainViewport = Set_as_MainViewport;
	VKCORE->TGFXCORE.IMGUI.Show_DemoWindow = Show_DemoWindow;
	VKCORE->TGFXCORE.IMGUI.Show_MetricsWindow = Show_MetricsWindow;
	VKCORE->TGFXCORE.IMGUI.Slider_Float = Slider_Float;
	VKCORE->TGFXCORE.IMGUI.Slider_Int = Slider_Int;
	VKCORE->TGFXCORE.IMGUI.Slider_Vec2 = Slider_Vec2;
	VKCORE->TGFXCORE.IMGUI.Slider_Vec3 = Slider_Vec3;
	VKCORE->TGFXCORE.IMGUI.Slider_Vec4 = Slider_Vec4;
	VKCORE->TGFXCORE.IMGUI.Text = Text;

	VKCORE->TGFXCORE.IMGUI.WindowManager.Delete_WINDOW = Delete_WINDOW;
	VKCORE->TGFXCORE.IMGUI.WindowManager.Register_WINDOW = Register_WINDOW;
	VKCORE->TGFXCORE.IMGUI.WindowManager.Run_IMGUI_WINDOWs = Run_IMGUI_WINDOWs;
}

tgfx_result IMGUI_VK::Initialize(tgfx_subdrawpass SubPass, VkDescriptorPool DescPool) {
	if (!VKCORE->WINDOWs.size()) {
		LOG(tgfx_result_FAIL, "Initialization of dear IMGUI has failed because you didn't create a window!");
		return tgfx_result_FAIL;
	}
	ImGui_ImplGlfw_InitForVulkan(VKCORE->WINDOWs[0]->GLFW_WINDOW, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = Vulkan_Instance;
	init_info.PhysicalDevice = RENDERGPU->PHYSICALDEVICE();
	init_info.Device = RENDERGPU->LOGICALDEVICE();
	for (unsigned int i = 0; i < RENDERGPU->QUEUEFAMS().size(); i++) {
		VK_QUEUEFAM& queue = RENDERGPU->QUEUEFAMS()[i];
		if (queue.SupportFlag.is_PRESENTATIONsupported && queue.SupportFlag.is_GRAPHICSsupported) {
			init_info.QueueFamily = queue.QueueFamilyIndex;
			init_info.Queue = queue.Queue;
		}
	}
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = DescPool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.CheckVkResultFn = CheckIMGUIVKResults;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	VK_SubDrawPass* SP = (VK_SubDrawPass*)SubPass;
	init_info.Subpass = static_cast<uint32_t>(SP->Binding_Index);
	ImGui_ImplVulkan_Init(&init_info, SP->DrawPass->RenderPassObject);
	STAT = IMGUI_STATUS::INITIALIZED;
	Subdrawpass = SubPass;

	Set_IMGUIFunctionPTRs();
}
tgfx_subdrawpass IMGUI_VK::Get_SubDrawPassHandle() const {
	return Subdrawpass;
}
IMGUI_VK::IMGUI_STATUS IMGUI_VK::Get_IMGUIStatus() {
	return STAT;
}

void IMGUI_VK::NewFrame() {
	if (STAT == IMGUI_STATUS::NEW_FRAME) {
		LOG(tgfx_result_WARNING, "You have mismatching IMGUI_VK calls, NewFrame() called twice without calling Render()!");
		return;
	}
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	STAT = IMGUI_STATUS::NEW_FRAME;
}

void IMGUI_VK::Render_AdditionalWindows() {
	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}
void IMGUI_VK::Render_toCB(VkCommandBuffer cb) {
	if (STAT == IMGUI_STATUS::RENDERED) {
		return;
	}
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
	STAT = IMGUI_STATUS::RENDERED;
}

void IMGUI_VK::UploadFontTextures() {

	//Upload font textures
	VkCommandPoolCreateInfo cp_ci = {};
	cp_ci.flags = 0;
	cp_ci.pNext = nullptr;
	cp_ci.queueFamilyIndex = RENDERGPU->QUEUEFAMS()[RENDERGPU->GRAPHICSQUEUEINDEX()].QueueFamilyIndex;
	cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	VkCommandPool cp;
	if (vkCreateCommandPool(RENDERGPU->LOGICALDEVICE(), &cp_ci, nullptr, &cp) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Creation of Command Pool for dear IMGUI Font Upload has failed!");
	}

	VkCommandBufferAllocateInfo cb_ai = {};
	cb_ai.commandBufferCount = 1;
	cb_ai.commandPool = cp;
	cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cb_ai.pNext = nullptr;
	cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	VkCommandBuffer cb;
	if (vkAllocateCommandBuffers(RENDERGPU->LOGICALDEVICE(), &cb_ai, &cb) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Creation of Command Buffer for dear IMGUI Font Upload has failed!");
	}



	if (vkResetCommandPool(RENDERGPU->LOGICALDEVICE(), cp, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Resetting of Command Pool for dear IMGUI Font Upload has failed!");
	}
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(cb, &begin_info) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "vkBeginCommandBuffer() for dear IMGUI Font Upload has failed!");
	}

	ImGui_ImplVulkan_CreateFontsTexture(cb);

	VkSubmitInfo end_info = {};
	end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &cb;
	if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "vkEndCommandBuffer() for dear IMGUI Font Upload has failed!");
	}
	if (vkQueueSubmit(RENDERGPU->QUEUEFAMS()[RENDERGPU->GRAPHICSQUEUEINDEX()].Queue, 1, &end_info, VK_NULL_HANDLE) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "VkQueueSubmit() for dear IMGUI Font Upload has failed!");
	}

	if (vkDeviceWaitIdle(RENDERGPU->LOGICALDEVICE()) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "vkDeviceWaitIdle() for dear IMGUI Font Upload has failed!");
	}
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	if (vkResetCommandPool(RENDERGPU->LOGICALDEVICE(), cp, 0) != VK_SUCCESS) {
		LOG(tgfx_result_FAIL, "Resetting of Command Pool for destruction of dear IMGUI Font Upload has failed!");
	}
	vkDestroyCommandPool(RENDERGPU->LOGICALDEVICE(), cp, nullptr);
}

void IMGUI_VK::Destroy_IMGUIResources() {
	// Resources to destroy when the program ends
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

struct VKIMGUI_WindowManager {
	std::vector<tgfx_IMGUI_WINDOW*> ALL_IMGUI_WINDOWs, Windows_toClose, Windows_toOpen;
};
static VKIMGUI_WindowManager* IMGUIWINDOWMANAGER = nullptr;

void Run_IMGUI_WINDOWs() {
	for (unsigned int i = 0; i < IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.size(); i++) {
		tgfx_IMGUI_WINDOW* window = IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs[i];
		LOG(tgfx_result_SUCCESS, ("Running the Window: " + std::string(window->WindowName)).c_str());
		window->RunWindow();
	}

	if (IMGUIWINDOWMANAGER->Windows_toClose.size() > 0) {
		for (unsigned int window_delete_i = 0; window_delete_i < IMGUIWINDOWMANAGER->Windows_toClose.size(); window_delete_i++) {
			tgfx_IMGUI_WINDOW* window_to_close = IMGUIWINDOWMANAGER->Windows_toClose[window_delete_i];
			for (unsigned int i = 0; i < IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.size(); i++) {
				if (window_to_close == IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs[i]) {
					delete window_to_close;
					window_to_close = nullptr;
					IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs[i] = nullptr;
					IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.erase(IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.begin() + i);

					//Inner loop will break!
					break;
				}
			}
		}

		IMGUIWINDOWMANAGER->Windows_toClose.clear();
	}

	if (IMGUIWINDOWMANAGER->Windows_toOpen.size() > 0) {
		unsigned int previous_size = IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.size();

		for (unsigned int i = 0; i < IMGUIWINDOWMANAGER->Windows_toOpen.size(); i++) {
			tgfx_IMGUI_WINDOW* window_to_open = IMGUIWINDOWMANAGER->Windows_toOpen[i];
			IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.push_back(window_to_open);
		}
		IMGUIWINDOWMANAGER->Windows_toOpen.clear();
		//We should run new windows!
		for (; previous_size < IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs.size(); previous_size++) {
			IMGUIWINDOWMANAGER->ALL_IMGUI_WINDOWs[previous_size]->RunWindow();
		}
	}
}
void Register_WINDOW(tgfx_IMGUI_WINDOW* Window) {
	LOG(tgfx_result_SUCCESS, "Registering a Window!");
	IMGUIWINDOWMANAGER->Windows_toOpen.push_back(Window);
}
void Delete_WINDOW(tgfx_IMGUI_WINDOW* Window) {
	IMGUIWINDOWMANAGER->Windows_toClose.push_back(Window);
}

void StartIMGUI() {
	LOG(tgfx_result_SUCCESS, "tgfx_imguicore's constructor has finished!");


	//Create Context here!
	IMGUI_CHECKVERSION();
	Context = ImGui::CreateContext();
	if (Context == nullptr) {
		LOG(tgfx_result_FAIL, "Dear IMGUI Context is nullptr after creation!\n");
	}

	//Set Input Handling settings here! 
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


	//Set color style to dark by default for now!
	ImGui::StyleColorsDark();
}
unsigned char Check_IMGUI_Version() {
	//Check version here, I don't care for now!
	return IMGUI_CHECKVERSION();
}

void Set_as_MainViewport() {
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
}

void Destroy_IMGUI_Resources() {
	LOG(tgfx_result_SUCCESS, "IMGUI resources are being destroyed!\n");
	ImGui::DestroyContext();
}

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


unsigned char Create_Window(const char* title, unsigned char should_close, unsigned char has_menubar) {
	bool boolean = should_close;
	ImGuiWindowFlags window_flags = 0;
	window_flags |= (has_menubar ? ImGuiWindowFlags_MenuBar : 0);
	bool result = ImGui::Begin(title, &boolean, window_flags);
	should_close = boolean;
	return result;
}

void End_Window() {
	ImGui::End();
}

void Text(const char* text) {
	ImGui::Text(text);
}

unsigned char Button(const char* button_name) {
	return ImGui::Button(button_name);
}

unsigned char Checkbox(const char* name, unsigned char* variable) {
	bool x = *variable;
	bool result = ImGui::Checkbox(name, &x);
	*variable = x;
	return result;
}

unsigned char Input_Text(const char* name, char** text) {
	LOG(tgfx_result_NOTCODED, "Vulkan::tgfx_imguicore->Input_Text() is not coded!");
	/*
	if (ImGui::InputText(name, text, ImGuiInputTextFlags_EnterReturnsTrue)) {
		return true;
	}*/
	return false;
}

unsigned char Begin_Menubar() {
	return ImGui::BeginMenuBar();
}

void End_Menubar() {
	ImGui::EndMenuBar();
}

unsigned char Begin_Menu(const char* name) {
	return ImGui::BeginMenu(name);
}

void End_Menu() {
	ImGui::EndMenu();
}

unsigned char Menu_Item(const char* name) {
	return ImGui::MenuItem(name);
}

unsigned char Input_Paragraph_Text(const char* name, char** Text) {
	LOG(tgfx_result_NOTCODED, "Vulkan::IMGUI::Input_Paragraph_Text() isn't coded!");
	/*
	if (ImGui::InputTextMultiline(name, Text, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue)) {
		return true;
	}*/
	return false;
}

//Puts the next item to the same line with last created item
//Use between after last item's end - before next item's begin!
void Same_Line() {
	ImGui::SameLine();
}

unsigned char Begin_Tree(const char* name) {
	return ImGui::TreeNode(name);
}

void End_Tree() {
	ImGui::TreePop();
}

unsigned char SelectList_OneLine(const char* name, unsigned int* selected_index, const char* const* item_names) {
	unsigned char is_new_item_selected = false;
	const char* preview_str = item_names[*selected_index];
	if (ImGui::BeginCombo(name, preview_str))	// The second parameter is the index of the label previewed before opening the combo.
	{
		unsigned int i = 0;
		while (item_names[i] != nullptr) {
			unsigned char is_selected = (*selected_index == i);
			if (ImGui::Selectable(item_names[i], is_selected)) {
				*selected_index = i;
				is_new_item_selected = true;
			}
			i++;
		}
		ImGui::EndCombo();
	}
	return is_new_item_selected;
}

//If selected, argument "is_selected" is set to its opposite!
void Selectable(const char* name, unsigned char* is_selected) {
	ImGui::Selectable(name, is_selected);
}

unsigned char Selectable_ListBox(const char* name, int* selected_index, const char* const* item_names) {
	int already_selected_index = *selected_index;
	unsigned char is_new_selected = false;
	if (ImGui::ListBoxHeader(name)) {
		unsigned int i = 0;
		while (item_names[i] != nullptr) {
			unsigned char is_selected = false;
			const char* item_name = item_names[i];
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
			LOG(tgfx_result_SUCCESS, ("Current Index: " + std::to_string(i)).c_str());
			LOG(tgfx_result_SUCCESS, ("Current Name: " + std::string(item_names[i])).c_str());
			LOG(tgfx_result_SUCCESS, ("Current Value: " + std::to_string(x)).c_str());
			i++;
		}
		ImGui::ListBoxFooter();
	}
}*/

unsigned char Begin_TabBar() {
	return ImGui::BeginTabBar("");
}
void End_TabBar() {
	ImGui::EndTabBar();
}
unsigned char Begin_TabItem(const char* name) {
	return ImGui::BeginTabItem(name);
}
void End_TabItem() {
	ImGui::EndTabItem();
}
void Separator() {
	ImGui::Separator();
}
tgfx_vec2 GetLastItemRectMin() {
	tgfx_vec2 vecvar;
	vecvar.x = ImGui::GetItemRectMin().x;
	vecvar.y = ImGui::GetItemRectMin().y;
	return vecvar;
}
tgfx_vec2 GetLastItemRectMax() {
	tgfx_vec2 vecvar;
	vecvar.x = ImGui::GetItemRectMax().x;
	vecvar.y = ImGui::GetItemRectMax().y;
	return vecvar;
}
tgfx_vec2 GetItemWindowPos() {
	tgfx_vec2 vecvar;
	vecvar.x = ImGui::GetCursorScreenPos().x;
	vecvar.y = ImGui::GetCursorScreenPos().y;
	return vecvar;
}
tgfx_vec2 GetMouseWindowPos() {
	tgfx_vec2 vecvar;
	vecvar.x = ImGui::GetMousePos().x;
	vecvar.y = ImGui::GetMousePos().y;
	return vecvar;
}







unsigned char Slider_Int(const char* name, int* data, int min, int max) { return ImGui::SliderInt(name, data, min, max); }
unsigned char Slider_Float(const char* name, float* data, float min, float max) { return ImGui::SliderFloat(name, data, min, max); }
unsigned char Slider_Vec2(const char* name, tgfx_vec2* data, float min, float max) { return ImGui::SliderFloat2(name, (float*)data, min, max); }
unsigned char Slider_Vec3(const char* name, tgfx_vec3* data, float min, float max) { return ImGui::SliderFloat3(name, (float*)data, min, max); }
unsigned char Slider_Vec4(const char* name, tgfx_vec4* data, float min, float max) { return ImGui::SliderFloat4(name, (float*)data, min, max); }