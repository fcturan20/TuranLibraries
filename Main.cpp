#include "Source/Main.h"
using namespace TuranEditor;

//Useful rendergraph handles to use later
GFX_API::GFXHandle DEPTHRT, FIRSTDRAWPASS_ID, WorldSubpassID, IMGUISubpassID, WP_ID, BarrierBeforeUpload_ID, UploadTP_ID, BarrierAfterUpload_ID, BarrierAfterDraw_ID, RTSlotSet_ID;
//Window handles
GFX_API::GFXHandle AlitaSwapchains[2], AlitaWindowHandle;

void Create_RTSlotSet_forFirstDrawPass(unsigned int WIDTH, unsigned int HEIGHT, GFX_API::GFXHandle& SlotSetID, GFX_API::GFXHandle& DepthRT) {
	vector<GFX_API::RTSLOT_Description> RTSlots;
	GFX_API::Texture_Description DEPTH_desc;
	DEPTH_desc.WIDTH = WIDTH;
	DEPTH_desc.HEIGHT = HEIGHT;
	DEPTH_desc.USAGE.isRenderableTo = true;
	DEPTH_desc.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8;
	DEPTH_desc.Properties.DATAORDER = GFX_API::TEXTURE_ORDER::SWIZZLE;
	DEPTH_desc.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D;
	DEPTH_desc.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP;
	DEPTH_desc.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
	if (GFXContentManager->Create_Texture(DEPTH_desc, 0, DepthRT) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Creation of the depth RT has failed!");
	}


	GFX_API::RTSLOT_Description SWPCHNSLOT;
	SWPCHNSLOT.CLEAR_VALUE = vec4(0.1f, 0.5f, 0.5f, 1.0f);
	SWPCHNSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
	SWPCHNSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
	SWPCHNSLOT.TextureHandles[0] = AlitaSwapchains[0];
	SWPCHNSLOT.TextureHandles[1] = AlitaSwapchains[1];
	SWPCHNSLOT.isUSEDLATER = true;
	SWPCHNSLOT.SLOTINDEX = 0;
	RTSlots.push_back(SWPCHNSLOT);


	GFX_API::RTSLOT_Description DEPTHSLOT;
	DEPTHSLOT.CLEAR_VALUE = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	DEPTHSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
	DEPTHSLOT.LOADOPSTENCIL = GFX_API::DRAWPASS_LOAD::LOAD;
	DEPTHSLOT.OPTYPESTENCIL = GFX_API::OPERATION_TYPE::READ_ONLY;
	DEPTHSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
	DEPTHSLOT.isUSEDLATER = true;
	DEPTHSLOT.SLOTINDEX = 0;	//Depth RTSlot's SLOTINDEX is ignored because you can only have one depth buffer at the same slot set
	DEPTHSLOT.TextureHandles[0] = DepthRT;
	DEPTHSLOT.TextureHandles[1] = DepthRT;
	RTSlots.push_back(DEPTHSLOT);

	if (GFXContentManager->Create_RTSlotset(RTSlots, SlotSetID) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("RT SlotSet creation has failed!");
	}
}

bool isWindowResized = false;
void WindowResizeCallback(GFX_API::GFXHandle WindowHandle, void* UserPointer, unsigned int WIDTH, unsigned int HEIGHT, GFX_API::GFXHandle* SwapchainTextureHandles) {
	AlitaSwapchains[0] = SwapchainTextureHandles[0];
	AlitaSwapchains[1] = SwapchainTextureHandles[1];

	GFX_API::GFXHandle NewSlotSetID, NewDepthRT;
	Create_RTSlotSet_forFirstDrawPass(WIDTH, HEIGHT, NewSlotSetID, NewDepthRT);
	GFXRENDERER->ChangeDrawPass_RTSlotSet(FIRSTDRAWPASS_ID, NewSlotSetID);
	//GFXContentManager->Delete_RTSlotSet(RTSlotSet_ID);
	//GFXContentManager->Delete_Texture(DEPTHRT, true);
	DEPTHRT = NewDepthRT;
	RTSlotSet_ID = NewSlotSetID;
	GFXRENDERER->ImageBarrier(DEPTHRT, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::DEPTHREADWRITE_STENCILREAD, 0, BarrierAfterUpload_ID);
	GFXRENDERER->ImageBarrier(AlitaSwapchains[0], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierAfterUpload_ID);
	GFXRENDERER->ImageBarrier(AlitaSwapchains[1], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierAfterUpload_ID);

	isWindowResized = true;
}

void FirstMain(TuranAPI::Threading::JobSystem* JobSystem) {
	Editor_System Systems(JobSystem);


	//Create Alita Window
	{
		GFX_API::WindowDescription WindowDesc;
		WindowDesc.WIDTH = 1280; WindowDesc.HEIGHT = 720; WindowDesc.MODE = GFX_API::WINDOW_MODE::WINDOWED;
		WindowDesc.MONITOR = Systems.Monitors[0].Handle; WindowDesc.NAME = "Vulkan Window";
		WindowDesc.SWAPCHAINUSAGEs.isCopiableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRenderableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRandomlyWrittenTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isSampledReadOnly = true;
		WindowDesc.resize_cb = WindowResizeCallback;
		GFX_API::Texture_Properties SwapchainProperties;
		AlitaWindowHandle = GFX->CreateWindow(WindowDesc, AlitaSwapchains, SwapchainProperties);
	}
	
	//Create Global Shader Inputs before the RenderGraph Construction
	GFX_API::GFXHandle FirstGlobalBuffer, FirstGlobalTexture;
	GFXContentManager->Create_GlobalBuffer("CameraData", 256, 1, true, GFX_API::Create_ShaderStageFlag(true, false, false, false, false),
		3, FirstGlobalBuffer);
	GFXContentManager->Create_GlobalTexture("FirstGlobTexture", true, 0, 2, GFX_API::Create_ShaderStageFlag(false, true, false, false, false), FirstGlobalTexture);


	GFX_API::GFXHandle StagingBuffer;
	if (GFXContentManager->Create_StagingBuffer(1024 * 1024 * 50, 3, StagingBuffer) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Staging buffer creation has failed!");
	}


	//Create and upload first sampled texture in host visible memory
	GFX_API::GFXHandle FIRSTSAMPLINGTYPE_ID, AlitaTexture, GokuBlackTexture;
	unsigned int AlitaSize = 0, AlitaOffset = 0, GokuBlackSize = 0, GokuBlackOffset = 0;
	{
		if (GFXContentManager->Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D, 0, 0, GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP,
			GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT,
			GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, FIRSTSAMPLINGTYPE_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Creation of sampling type has failed, so application has!");
		}

		GFX_API::Texture_Description alita_desc;
		void* ALITADATA = nullptr;
		if (TuranEditor::Texture_Loader::Import_Texture("C:/Users/furka/Pictures/Wallpapers/Thumbnail.png", alita_desc, ALITADATA) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Alita Texture import has failed!");
		}

		alita_desc.USAGE.isSampledReadOnly = true;
		if (GFXContentManager->Create_Texture(alita_desc, 0, AlitaTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Alita texture creation has failed!");
		}
		AlitaSize = alita_desc.WIDTH * alita_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(alita_desc.Properties.CHANNEL_TYPE);
		AlitaOffset = 944;
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, ALITADATA, AlitaSize, AlitaOffset) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the Alita texture has failed!");
		}
		delete ALITADATA;




		GFX_API::Texture_Description gokublack_desc;
		void* GOKUBLACKDATA = nullptr;
		if (TuranEditor::Texture_Loader::Import_Texture("C:/Users/furka/Pictures/Wallpapers/Goku Black SSRose.jpg", gokublack_desc, GOKUBLACKDATA) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Goku Black Texture import has failed!");
		}

		gokublack_desc.USAGE.isSampledReadOnly = true;
		if (GFXContentManager->Create_Texture(gokublack_desc, 0, GokuBlackTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Goku Black texture creation has failed!");
		}
		GokuBlackSize = gokublack_desc.WIDTH * gokublack_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(gokublack_desc.Properties.CHANNEL_TYPE);
		GokuBlackOffset = AlitaOffset + AlitaSize;
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, GOKUBLACKDATA, GokuBlackSize, GokuBlackOffset) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the Goku Black texture has failed!");
		}
		delete GOKUBLACKDATA;
	}
	GFXContentManager->SetGlobal_SampledTexture(FirstGlobalTexture, 0, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);
	GFXContentManager->SetGlobal_SampledTexture(FirstGlobalTexture, 1, GokuBlackTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);


	//RenderGraph Construction
	{
		GFXRENDERER->Start_RenderGraphConstruction();

		{
			GFX_API::PassWait_Description LastFrameBarrierWait;
			LastFrameBarrierWait.WaitedPass = &BarrierAfterDraw_ID;
			LastFrameBarrierWait.WaitedStage.TRANSFERCMD = true;
			LastFrameBarrierWait.WaitLastFramesPass = true;

			if (GFXRENDERER->Create_TransferPass({ LastFrameBarrierWait }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "BarrierBeforeUpload", BarrierBeforeUpload_ID) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("BarrierBeforeUpload creation has failed!");
			}
		}

		//Create Upload TP
		{
			GFX_API::PassWait_Description BBU_desc;
			BBU_desc.WaitedPass = &BarrierBeforeUpload_ID;
			BBU_desc.WaitedStage.TRANSFERCMD = true;
			BBU_desc.WaitLastFramesPass = false;
			if (GFXRENDERER->Create_TransferPass({ BBU_desc }, GFX_API::TRANFERPASS_TYPE::TP_COPY, "Uploader", UploadTP_ID)) {
				LOG_CRASHING_TAPI("Upload TP creation has failed!");
			}
		}

		//Barrier After Upload TP
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

			GFXRENDERER->Create_TransferPass({ Upload_dep, WP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "BarrierAfterUpload", BarrierAfterUpload_ID);
		}

		//Create RTs, Base RTSlotSet and the inherited one!
		GFX_API::GFXHandle ISlotSetID;
		{
			Create_RTSlotSet_forFirstDrawPass(1280, 720, RTSlotSet_ID, DEPTHRT);

			GFX_API::RTSLOTUSAGE_Description IRT_SWPCHNSLOT;
			IRT_SWPCHNSLOT.IS_DEPTH = false;
			IRT_SWPCHNSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_SWPCHNSLOT.SLOTINDEX = 0;
			GFX_API::RTSLOTUSAGE_Description IRT_DEPTHSLOT;
			IRT_DEPTHSLOT.IS_DEPTH = true;
			IRT_DEPTHSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_DEPTHSLOT.OPTYPESTENCIL = GFX_API::OPERATION_TYPE::READ_ONLY;
			IRT_DEPTHSLOT.SLOTINDEX = 0;
			GFXContentManager->Inherite_RTSlotSet({ IRT_SWPCHNSLOT, IRT_DEPTHSLOT }, RTSlotSet_ID, ISlotSetID);
		}

		//Create Draw Pass
		{
			GFX_API::SubDrawPass_Description WorldSubpass_desc;
			WorldSubpass_desc.INHERITEDSLOTSET = ISlotSetID;
			WorldSubpass_desc.SubDrawPass_Index = 0;
			GFX_API::SubDrawPass_Description IMGUISubpass_desc;
			IMGUISubpass_desc.INHERITEDSLOTSET = ISlotSetID;
			IMGUISubpass_desc.SubDrawPass_Index = 1;
			IMGUISubpass_desc.WaitOp = GFX_API::SUBPASS_ACCESS::LATE_Z_READWRITE;
			IMGUISubpass_desc.ContinueOp = GFX_API::SUBPASS_ACCESS::FRAGMENTRT_WRITEONLY;
			vector<GFX_API::SubDrawPass_Description> DESCS{ WorldSubpass_desc, IMGUISubpass_desc };
			vector<GFX_API::GFXHandle> SPs_ofFIRSTDP;


			GFX_API::PassWait_Description FirstBarrierTP_dep;
			FirstBarrierTP_dep.WaitedPass = &BarrierAfterUpload_ID;
			FirstBarrierTP_dep.WaitedStage.TRANSFERCMD = true;
			FirstBarrierTP_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_DrawPass(DESCS, RTSlotSet_ID, { FirstBarrierTP_dep }, "FirstDP", SPs_ofFIRSTDP, FIRSTDRAWPASS_ID);
			WorldSubpassID = SPs_ofFIRSTDP[0];
			IMGUISubpassID = SPs_ofFIRSTDP[1];
		}

		//Create Final Barrier TP (to change layout of the Swapchain texture)
		{
			GFX_API::PassWait_Description DP_dep;
			DP_dep.WaitedPass = &FIRSTDRAWPASS_ID;
			DP_dep.WaitedStage.COLORRTOUTPUT = true;
			DP_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_TransferPass({ DP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "Final Barrier TP", BarrierAfterDraw_ID);
		}

		//Create Window Pass
		{
			GFX_API::PassWait_Description FinalBarrier_dep;
			FinalBarrier_dep.WaitedPass = &BarrierAfterDraw_ID;
			FinalBarrier_dep.WaitedStage.TRANSFERCMD = true;
			FinalBarrier_dep.WaitLastFramesPass = false;
			GFXRENDERER->Create_WindowPass({ FinalBarrier_dep }, "First WP", WP_ID);
		}

		GFXRENDERER->Finish_RenderGraphConstruction(IMGUISubpassID);
	}


	//Create first attribute layout and mesh buffer for first quad
	//Also create index buffer
	GFX_API::GFXHandle VERTEXBUFFER_ID = nullptr, INDEXBUFFER_ID = nullptr, MESH_VAL= nullptr;
	struct Vertex {
		vec3 Pos;
		vec2 TextCoord;
	};
	{
		Vertex VertexData[4]{
			{vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f,0.0f)},
			{vec3(0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f)},
			{vec3(0.5f, 0.5f, 0.0f), vec2(1.0f, 1.0f)},
			{vec3(-0.5f, 0.5f, 0.0f), vec2(0.0f, 1.0f)},
		};
		unsigned int IndexData[6]{ 0, 1, 2, 2, 3, 0};

		GFXContentManager->Create_VertexAttributeLayout({ GFX_API::DATA_TYPE::VAR_VEC3, GFX_API::DATA_TYPE::VAR_VEC2 }, GFX_API::VERTEXLIST_TYPEs::TRIANGLELIST, MESH_VAL);
		if (GFXContentManager->Create_VertexBuffer(MESH_VAL, 4, 0, VERTEXBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Vertex Buffer creation has failed!");
		}
		
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VertexData, sizeof(Vertex) * 4, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex buffer to staging buffer has failed!");
		}

		if (GFXContentManager->Create_IndexBuffer(GFX_API::DATA_TYPE::VAR_UINT32, 6, 0, INDEXBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Index Buffer creation has failed!");
		}

		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, IndexData, 24, 80) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading index buffer to staging buffer has failed!");
		}
	}




	//Create and Link Texture Display Material Type/Instance
	GFX_API::GFXHandle TEXTUREDISPLAY_MATTYPE, TEXTUREDISPLAY_MATINST;
	{
		GFX_API::GFXHandle VS_ID, FS_ID;
		unsigned int VS_CODESIZE = 0, FS_CODESIZE = 0;
		char* VS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/FirstVert.spv", VS_CODESIZE);
		char* FS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/FirstFrag.spv", FS_CODESIZE);
		GFX_API::ShaderSource_Resource VS, FS;
		VS.LANGUAGE = GFX_API::SHADER_LANGUAGEs::SPIRV; FS.LANGUAGE = GFX_API::SHADER_LANGUAGEs::SPIRV;
		VS.STAGE.VERTEXSHADER = true; FS.STAGE.VERTEXSHADER = false;
		VS.STAGE.FRAGMENTSHADER = false; FS.STAGE.FRAGMENTSHADER = true;
		VS.SOURCE_DATA = VS_CODE; VS.DATA_SIZE = VS_CODESIZE;
		FS.SOURCE_DATA = FS_CODE; FS.DATA_SIZE = FS_CODESIZE;
		GFXContentManager->Compile_ShaderSource(&VS, VS_ID);
		GFXContentManager->Compile_ShaderSource(&FS, FS_ID);
		GFX_API::Material_Type MATTYPE;
		MATTYPE.VERTEXSOURCE_ID = VS_ID;
		MATTYPE.FRAGMENTSOURCE_ID = FS_ID;
		MATTYPE.SubDrawPass_ID = WorldSubpassID;
		MATTYPE.MATERIALTYPEDATA.clear();
		
		{
			GFX_API::ShaderInput_Description first_desc;
			first_desc.BINDINGPOINT = 0;
			first_desc.ELEMENTCOUNT = 1;
			first_desc.NAME = "FirstSampledTexture";
			first_desc.SHADERSTAGEs.FRAGMENTSHADER = true;
			first_desc.TYPE = GFX_API::SHADERINPUT_TYPE::IMAGE_G;
			MATTYPE.MATERIALTYPEDATA.push_back(first_desc);
		}
		MATTYPE.ATTRIBUTELAYOUT_ID = MESH_VAL;
		MATTYPE.culling = GFX_API::CULL_MODE::CULL_OFF;
		MATTYPE.polygon = GFX_API::POLYGON_MODE::FILL;
		MATTYPE.depthtest = GFX_API::DEPTH_TESTs::DEPTH_TEST_LESS;
		MATTYPE.depthmode = GFX_API::DEPTH_MODEs::DEPTH_READ_WRITE;
		MATTYPE.frontfacedstencil.CompareOperation = GFX_API::STENCIL_COMPARE::ALWAYS_PASS;
		MATTYPE.frontfacedstencil.DepthFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.DepthSuccess = GFX_API::STENCIL_OP::CHANGE;
		MATTYPE.frontfacedstencil.STENCILCOMPAREMASK = 0xFF;
		MATTYPE.frontfacedstencil.StencilFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.STENCILVALUE = 255;
		MATTYPE.frontfacedstencil.STENCILWRITEMASK = 0xFF;
		if (GFXContentManager->Link_MaterialType(MATTYPE, TEXTUREDISPLAY_MATTYPE) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Link MaterialType has failed!");
		}

		if (GFXContentManager->Create_MaterialInst(TEXTUREDISPLAY_MATTYPE, TEXTUREDISPLAY_MATINST) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture Display Material Instance creation has failed!");
		}
		/*
		if (GFXContentManager->SetMaterial_ImageTexture(TEXTUREDISPLAY_MATTYPE, true, false, 0, 0, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEWRITE) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture Display Material Instance's Alita image texture setting has failed!");
		}*/
	}
	//Upload Camera Data
	{
		mat4 matrixes[4];
		matrixes[0] = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		matrixes[1] = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		matrixes[2] = mat4(mat3(matrixes[1]));	//For Skybox
		matrixes[3] = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10000.0f);

		if (GFXContentManager->Upload_toBuffer(FirstGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, &matrixes, 256, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the world matrix data has failed!");
		}
	}

	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, 0, BarrierBeforeUpload_ID);
	GFXRENDERER->ImageBarrier(GokuBlackTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, 0, BarrierBeforeUpload_ID);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VERTEXBUFFER_ID, GFX_API::BUFFER_TYPE::VERTEX, 0, 0, sizeof(Vertex) * 4);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, INDEXBUFFER_ID, GFX_API::BUFFER_TYPE::INDEX, 80, 0, 24);
	GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, AlitaTexture, AlitaOffset, { 0,0,0,0,0,0 }, 0);
	GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, GokuBlackTexture, GokuBlackOffset, { 0,0,0,0,0,0 }, 0);
	GFXRENDERER->ImageBarrier(GokuBlackTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, 0, BarrierAfterUpload_ID);
	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, 0, BarrierAfterUpload_ID);
	GFXRENDERER->ImageBarrier(DEPTHRT, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::DEPTHREADWRITE_STENCILREAD, 0, BarrierAfterUpload_ID);
	GFXRENDERER->ImageBarrier(AlitaSwapchains[0], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierAfterUpload_ID);
	GFXRENDERER->DrawDirect(VERTEXBUFFER_ID, INDEXBUFFER_ID, 0, 0, 0, 2, 0, TEXTUREDISPLAY_MATINST, WorldSubpassID);
	GFXRENDERER->ImageBarrier(AlitaSwapchains[0], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, 0, BarrierAfterDraw_ID);
	GFXRENDERER->SwapBuffers(AlitaWindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();

	GFXRENDERER->ImageBarrier(AlitaSwapchains[1], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierAfterUpload_ID);
	GFXRENDERER->DrawDirect(VERTEXBUFFER_ID, INDEXBUFFER_ID, 0, 0, 0, 2, 0, TEXTUREDISPLAY_MATINST, WorldSubpassID);
	GFXRENDERER->ImageBarrier(AlitaSwapchains[1], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, 0, BarrierAfterDraw_ID);
	GFXRENDERER->SwapBuffers(AlitaWindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();

	Main_Window* MainWindow = new Main_Window;
	unsigned int i = 0;
	while (MainWindow->isWindowOpen()) {
		TURAN_PROFILE_SCOPE_MCS("Run Loop");
		
		if (i % 2) {
			GFXContentManager->SetGlobal_SampledTexture(FirstGlobalTexture, 1, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);
			//GFXContentManager->SetMaterial_ImageTexture(TEXTUREDISPLAY_MATTYPE, true, true, 0, 0, GokuBlackTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);
		}
		else {
			GFXContentManager->SetGlobal_SampledTexture(FirstGlobalTexture, 1, GokuBlackTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);
			//GFXContentManager->SetMaterial_ImageTexture(TEXTUREDISPLAY_MATTYPE, true, true, 0, 0, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEWRITE);
		}
		IMGUI_RUNWINDOWS();
		if (!isWindowResized) {
			GFXRENDERER->ImageBarrier(AlitaSwapchains[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierBeforeUpload_ID);
		}
		else {
			isWindowResized = false;
		}
		GFXRENDERER->DrawDirect(VERTEXBUFFER_ID, INDEXBUFFER_ID, 0, 0, 0, 1, 0, TEXTUREDISPLAY_MATINST, WorldSubpassID);
		GFXRENDERER->ImageBarrier(AlitaSwapchains[GFX->Get_WindowFrameIndex(AlitaWindowHandle)], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, 0, BarrierAfterDraw_ID);
		GFXRENDERER->SwapBuffers(AlitaWindowHandle, WP_ID);
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
	//Don't call LOG_xxx here because logging system is closed with EditorSystem
	std::cout << "Returned to main()!\n";
	delete JobSys;
	return 1;
}