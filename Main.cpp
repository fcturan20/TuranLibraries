#include "Source/Main.h"
using namespace TuranEditor;

void FirstMain(TuranAPI::Threading::JobSystem* JobSystem) {
	Editor_System Systems(JobSystem);


	//Create Window
	GFX_API::GFXHandle SwapchainTextures[2], WindowHandle;
	{
		GFX_API::WindowDescription WindowDesc;
		WindowDesc.WIDTH = 1280; WindowDesc.HEIGHT = 720; WindowDesc.MODE = GFX_API::WINDOW_MODE::WINDOWED;
		WindowDesc.MONITOR = Systems.Monitors[0].Handle; WindowDesc.NAME = "Vulkan Window";
		WindowDesc.SWAPCHAINUSAGEs.isCopiableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRenderableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRandomlyWrittenTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isSampledReadOnly = true;
		GFX_API::Texture_Properties SwapchainProperties;
		WindowHandle = GFX->CreateWindow(WindowDesc, SwapchainTextures, SwapchainProperties);
	}

	//Construct RenderGraph
	GFX_API::GFXHandle SubpassID, ISlotSetID, WP_ID, FirstBarrierTP_ID, UploadTP_ID, FinalBarrierTP_ID;
	RenderGraphConstruction_DrawPassed(SwapchainTextures[0], SwapchainTextures[1], SubpassID, ISlotSetID, WP_ID, FirstBarrierTP_ID, UploadTP_ID, FinalBarrierTP_ID);

	GFX_API::GFXHandle StagingBuffer;
	if (GFXContentManager->Create_StagingBuffer(1024 * 1024 * 10, GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE, StagingBuffer) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Staging buffer creation has failed, fix it!");
	}
	
	//Create first attribute layout and mesh buffer for first triangle
	GFX_API::GFXHandle MESHBUFFER_ID = 0, VAL_ID = 0;
	{
		struct Vertex {
			vec2 Pos;
			vec3 Color;
		};
		Vertex VertexData[3]{
			{vec2(-0.5f, 0.5f), vec3(1,0,0)},
			{vec2(0.5f, 0.5f), vec3(0,0,1)},
			{vec2(0.0f, -0.5f), vec3(0,1,0)}
		};

		GFX_API::GFXHandle PositionVA_ID, ColorVA_ID;
		GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC2, true, PositionVA_ID);
		GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC3, true, ColorVA_ID);
		vector<GFX_API::GFXHandle> VAs{ PositionVA_ID, ColorVA_ID };
		GFXContentManager->Create_VertexAttributeLayout(VAs, VAL_ID);
		if (GFXContentManager->Create_VertexBuffer(VAL_ID, 3, GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, MESHBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Vertex Buffer creation has failed!");
		}

		/*
		if (GFXContentManager->Uploadto_StagingBuffer(StagingBuffer, VertexData, 60, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex buffer to staging buffer has failed!");
		}*/
	}

	//Create and Link first material type
	GFX_API::GFXHandle FIRSTMATINST_ID;
	{
		GFX_API::ShaderSource_Resource VS, FS;
		unsigned int VS_CODESIZE = 0, FS_CODESIZE = 0;
		char* VS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/FirstVert.spv", VS_CODESIZE);
		char* FS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/FirstFrag.spv", FS_CODESIZE);
		VS.LANGUAGE = GFX_API::SHADER_LANGUAGEs::SPIRV; FS.LANGUAGE = GFX_API::SHADER_LANGUAGEs::SPIRV;
		VS.STAGE.VERTEXSHADER = true; FS.STAGE.VERTEXSHADER = false;
		VS.STAGE.FRAGMENTSHADER = false; FS.STAGE.FRAGMENTSHADER = true;
		VS.SOURCE_DATA = VS_CODE; VS.DATA_SIZE = VS_CODESIZE;
		FS.SOURCE_DATA = FS_CODE; FS.DATA_SIZE = FS_CODESIZE;
		GFX_API::GFXHandle VS_ID, FS_ID;
		GFXContentManager->Compile_ShaderSource(&VS, VS_ID);
		GFXContentManager->Compile_ShaderSource(&FS, FS_ID);
		GFX_API::Material_Type MATTYPE;
		MATTYPE.VERTEXSOURCE_ID = VS_ID;
		MATTYPE.FRAGMENTSOURCE_ID = FS_ID;
		MATTYPE.SubDrawPass_ID = SubpassID;
		MATTYPE.MATERIALTYPEDATA.clear();	//There is no buffer or texture access for now!
		MATTYPE.IRTSLOTSET_ID = ISlotSetID;
		MATTYPE.ATTRIBUTELAYOUT_ID = VAL_ID;
		GFX_API::GFXHandle MATTYPE_ID;
		GFXContentManager->Link_MaterialType(MATTYPE, MATTYPE_ID);
		GFX_API::Material_Instance MATINST;
		MATINST.MATERIALDATAs.clear();		//There is no buffer or texture access for now!
		MATINST.Material_Type = MATTYPE_ID;
		GFXContentManager->Create_MaterialInst(MATINST, FIRSTMATINST_ID);
	}

	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->Render_DrawCall(MESHBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();

	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->Render_DrawCall(MESHBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();

	unsigned int i = 0;
	while (true) {
		TURAN_PROFILE_SCOPE_MCS("Run Loop");


		GFXRENDERER->ImageBarrier(SwapchainTextures[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
		GFXRENDERER->Render_DrawCall(MESHBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
		GFXRENDERER->ImageBarrier(SwapchainTextures[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
		GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
		GFXRENDERER->Run();
		//IMGUI->New_Frame();
		//IMGUI_RUNWINDOWS();


		//Take inputs by GFX API specific library that supports input (For now, just GLFW for OpenGL4) and send it to Engine!
		//In final product, directly OS should be used to take inputs!
		Editor_System::Take_Inputs();

		//Save logs to disk each 25th frame!
		i++;
		if (!(i % 25)) {
			i = 0;
			WRITE_LOGs_toFILEs_TAPI();
		}
	}

}

int main() {
	TuranAPI::Threading::JobSystem* JobSys = nullptr;
	new TuranAPI::Threading::JobSystem([&JobSys]()
		{FirstMain(JobSys); }
	, JobSys);
	
}