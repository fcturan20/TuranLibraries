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
	
	//Create Global Buffers before the RenderGraph Construction
	GFX_API::GFXHandle FirstGlobalBuffer;
	GFXContentManager->Create_GlobalBuffer("FirstGlobalBuffer", 16, 1, true, GFX_API::Create_ShaderStageFlag(true, false, false, false, false),
		GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, FirstGlobalBuffer);


	//Useful rendergraph handles to use later
	GFX_API::GFXHandle FIRSTRT, SubpassID, ISlotSetID, WP_ID, FirstBarrierTP_ID, UploadTP_ID, FinalBarrierTP_ID; 

	//RenderGraph Construction
	{
		//Handles that're not used outside of the function
		GFX_API::GFXHandle FIRSTDRAWPASS_ID, RTSlotSet_ID;

		GFXRENDERER->Start_RenderGraphConstruction();

		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "Uploader", UploadTP_ID);

		//First Barrier TP
		//This pass depends on both the uploader (changes the layouts of the uploaded textures)
		//and also 2 frames ago's swapchain display (because this'll change the layout from SWPCHN_DSPLY to RTCOLORATTACHMENT)
		{
			GFX_API::PassWait_Description Upload_dep;
			Upload_dep.WaitedPass = &UploadTP_ID;
			Upload_dep.WaitedStage.TRANSFERCMD = true;
			Upload_dep.WaitLastFramesPass = false;

			GFX_API::PassWait_Description WP_dep;
			WP_dep.WaitedPass = &WP_ID;
			WP_dep.WaitedStage.SWAPCHAINDISPLAY = true;
			WP_dep.WaitLastFramesPass = false;

			GFXRENDERER->Create_TransferPass({ Upload_dep, WP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "First Barrier TP", FirstBarrierTP_ID);
		}

		vector<GFX_API::RTSLOT_Description> RTSlots;
		//Create RT, Base RTSlotSet and the inherited one!
		{
			GFX_API::Texture_Resource COLORRT;
			COLORRT.WIDTH = 1280;
			COLORRT.HEIGHT = 720;
			COLORRT.USAGE.isCopiableFrom = true;
			COLORRT.USAGE.isCopiableTo = false;
			COLORRT.USAGE.isRandomlyWrittenTo = false;
			COLORRT.USAGE.isRenderableTo = true;
			COLORRT.USAGE.isSampledReadOnly = false;
			COLORRT.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA32F;
			COLORRT.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D;
			COLORRT.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP;
			COLORRT.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
			if (GFXContentManager->Create_Texture(COLORRT, GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, FIRSTRT) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("First RT creation has failed!");
			}


			GFX_API::RTSLOT_Description SWPCHNSLOT;
			SWPCHNSLOT.CLEAR_VALUE = vec4(0.1f, 0.5f, 0.5f, 1.0f);
			SWPCHNSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			SWPCHNSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			SWPCHNSLOT.TextureHandles[0] = SwapchainTextures[0];
			SWPCHNSLOT.TextureHandles[1] = SwapchainTextures[1];
			SWPCHNSLOT.SLOTINDEX = 0;
			RTSlots.push_back(SWPCHNSLOT);

			GFX_API::RTSLOT_Description EXTRASLOT;
			EXTRASLOT.CLEAR_VALUE = vec4(0.6f, 0.5f, 0.5f, 1.0f);
			EXTRASLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			EXTRASLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			EXTRASLOT.TextureHandles[0] = FIRSTRT;
			EXTRASLOT.TextureHandles[1] = FIRSTRT;
			EXTRASLOT.SLOTINDEX = 1;
			RTSlots.push_back(EXTRASLOT);

			GFXContentManager->Create_RTSlotset(RTSlots, RTSlotSet_ID);


			GFX_API::RTSLOTUSAGE_Description IRT_SWPCHNSLOT;
			IRT_SWPCHNSLOT.IS_DEPTH = false;
			IRT_SWPCHNSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_SWPCHNSLOT.SLOTINDEX = 0;
			GFX_API::RTSLOTUSAGE_Description IRT_EXTRASLOT;
			IRT_EXTRASLOT.IS_DEPTH = false;
			IRT_EXTRASLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_EXTRASLOT.SLOTINDEX = 1;
			GFXContentManager->Inherite_RTSlotSet({ IRT_SWPCHNSLOT, IRT_EXTRASLOT }, RTSlotSet_ID, ISlotSetID);
		}

		//Create Draw Pass
		{
			GFX_API::SubDrawPass_Description Subpass_desc;
			Subpass_desc.INHERITEDSLOTSET = ISlotSetID;
			Subpass_desc.SubDrawPass_Index = 0;
			vector<GFX_API::SubDrawPass_Description> DESCS{ Subpass_desc };
			vector<GFX_API::GFXHandle> SPs_ofFIRSTDP;


			GFX_API::PassWait_Description FirstBarrierTP_dep;
			FirstBarrierTP_dep.WaitedPass = &FirstBarrierTP_ID;
			FirstBarrierTP_dep.WaitedStage.TRANSFERCMD = true;
			FirstBarrierTP_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_DrawPass(DESCS, RTSlotSet_ID, { FirstBarrierTP_dep }, "FirstDP", SPs_ofFIRSTDP, FIRSTDRAWPASS_ID);
			SubpassID = SPs_ofFIRSTDP[0];
		}

		//Create Final Barrier TP (to change layout of the Swapchain texture)
		{
			GFX_API::PassWait_Description DP_dep;
			DP_dep.WaitedPass = &FIRSTDRAWPASS_ID;
			DP_dep.WaitedStage.COLORRTOUTPUT = true;
			DP_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_TransferPass({ DP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "Final Barrier TP", FinalBarrierTP_ID);
		}

		//Create Window Pass
		{
			GFX_API::PassWait_Description FinalBarrier_dep;
			FinalBarrier_dep.WaitedPass = &FinalBarrierTP_ID;
			FinalBarrier_dep.WaitedStage.TRANSFERCMD = true;
			FinalBarrier_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_WindowPass({ FinalBarrier_dep }, "First WP", WP_ID);
		}

		GFXRENDERER->Finish_RenderGraphConstruction();
	}


	GFX_API::GFXHandle StagingBuffer;
	if (GFXContentManager->Create_StagingBuffer(1024 * 1024 * 10, GFX_API::SUBALLOCATEBUFFERTYPEs::HOSTVISIBLE, StagingBuffer) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Staging buffer creation has failed, fix it!");
	}
	
	//Create first attribute layout and mesh buffer for first triangle
	GFX_API::GFXHandle VERTEXBUFFER_ID = 0, VAL_ID = 0;
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
		if (GFXContentManager->Create_VertexBuffer(VAL_ID, 3, GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, VERTEXBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Vertex Buffer creation has failed!");
		}

		
		if (GFXContentManager->Uploadto_StagingBuffer(StagingBuffer, VertexData, 60, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex buffer to staging buffer has failed!");
		}
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
		MATTYPE.MATERIALTYPEDATA.clear();
		
		{
			GFX_API::MaterialDataDescriptor first_desc;
			first_desc.BINDINGPOINT = 0;
			first_desc.DATA_SIZE = 16;
			first_desc.NAME = "FirstUniformInput";
			first_desc.SHADERSTAGEs.VERTEXSHADER = true;
			first_desc.TYPE = GFX_API::MATERIALDATA_TYPE::CONSTUBUFFER_G;
			MATTYPE.MATERIALTYPEDATA.push_back(first_desc);
		}
		MATTYPE.ATTRIBUTELAYOUT_ID = VAL_ID;
		GFX_API::GFXHandle MATTYPE_ID;
		if (GFXContentManager->Link_MaterialType(MATTYPE, MATTYPE_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Link MaterialType has failed!");
		}
		GFXContentManager->SetMaterial_UniformBuffer(MATTYPE_ID, true, false, 0, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, 60);
		vec3 FragColor(1.0f, 0.0f, 0.0f);
		if (GFXContentManager->Uploadto_StagingBuffer(StagingBuffer, &FragColor, 12, 60) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex color to staging buffer has failed!");
		}
		GFXContentManager->Create_MaterialInst(MATTYPE_ID, FIRSTMATINST_ID);
	}

	GFXRENDERER->ImageBarrier(FIRSTRT, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VERTEXBUFFER_ID, GFX_API::BUFFER_TYPE::VERTEX, 0, 0, 60);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, FirstGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, 60, 0, 12);
	GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();


	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();


	unsigned int i = 0;
	while (true) {
		TURAN_PROFILE_SCOPE_MCS("Run Loop");

		GFXRENDERER->ImageBarrier(SwapchainTextures[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
		GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, nullptr, FIRSTMATINST_ID, SubpassID);
		GFXRENDERER->ImageBarrier(SwapchainTextures[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
		GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
		GFXRENDERER->Run();


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