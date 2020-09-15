#include "Main.h"
using namespace TuranEditor;

int main() {
	Editor_System Systems;


	while (true) {
		TURAN_PROFILE_SCOPE("Run Loop");


		//IMGUI->New_Frame();
		//IMGUI_RUNWINDOWS();
		GFX->Swapbuffers_ofMainWindow();


		//Take inputs by GFX API specific library that supports input (For now, just GLFW for OpenGL4) and send it to Engine!
		//In final product, directly OS should be used to take inputs!
		Editor_System::Take_Inputs();
	}

	return 1;
}