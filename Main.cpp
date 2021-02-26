#include "Source/Main.h"
using namespace TuranEditor;

void FirstMain(TuranAPI::Threading::JobSystem* JobSystem) {
	Editor_System Systems(JobSystem);


	//Create Alita Window
	GFX_API::GFXHandle AlitaSwapchains[2], AlitaWindowHandle;
	{
		GFX_API::WindowDescription WindowDesc;
		WindowDesc.WIDTH = 1280; WindowDesc.HEIGHT = 720; WindowDesc.MODE = GFX_API::WINDOW_MODE::WINDOWED;
		WindowDesc.MONITOR = Systems.Monitors[0].Handle; WindowDesc.NAME = "Vulkan Window";
		WindowDesc.SWAPCHAINUSAGEs.isCopiableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRenderableTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isRandomlyWrittenTo = true;
		WindowDesc.SWAPCHAINUSAGEs.isSampledReadOnly = true;
		GFX_API::Texture_Properties SwapchainProperties;
		AlitaWindowHandle = GFX->CreateWindow(WindowDesc, AlitaSwapchains, SwapchainProperties);
	}
	
	//Create Global Buffers before the RenderGraph Construction
	GFX_API::GFXHandle FirstGlobalBuffer;
	GFXContentManager->Create_GlobalBuffer("CameraData", 256, 1, true, GFX_API::Create_ShaderStageFlag(true, false, false, false, false),
		3, FirstGlobalBuffer);


	//Useful rendergraph handles to use later
	GFX_API::GFXHandle DEPTHRT, WorldSubpassID, IMGUISubpassID, WP_ID, BarrierBeforeUpload_ID, UploadTP_ID, BarrierAfterUpload_ID, BarrierAfterDraw_ID;

	//RenderGraph Construction
	{
		//Handles that're not used outside of the function
		GFX_API::GFXHandle FIRSTDRAWPASS_ID, RTSlotSet_ID;

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

		vector<GFX_API::RTSLOT_Description> RTSlots;
		//Create RTs, Base RTSlotSet and the inherited one!
		GFX_API::GFXHandle ISlotSetID;
		{
			GFX_API::Texture_Description DEPTH_desc;
			DEPTH_desc.WIDTH = 1280;
			DEPTH_desc.HEIGHT = 720;
			DEPTH_desc.USAGE.isRenderableTo = true;
			DEPTH_desc.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_D24S8;
			DEPTH_desc.Properties.DATAORDER = GFX_API::TEXTURE_ORDER::SWIZZLE;
			DEPTH_desc.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D;
			DEPTH_desc.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP;
			DEPTH_desc.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
			if (GFXContentManager->Create_Texture(DEPTH_desc, 0, DEPTHRT) != TAPI_SUCCESS) {
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
			DEPTHSLOT.TextureHandles[0] = DEPTHRT;
			DEPTHSLOT.TextureHandles[1] = DEPTHRT;
			RTSlots.push_back(DEPTHSLOT);

			if (GFXContentManager->Create_RTSlotset(RTSlots, RTSlotSet_ID) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("RT SlotSet creation has failed!");
			}


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
			IMGUISubpass_desc.ContinueOp = GFX_API::SUBPASS_ACCESS::ALLCOMMANDS;
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


	GFX_API::GFXHandle StagingBuffer;
	if (GFXContentManager->Create_StagingBuffer(1024 * 1024 * 110, 3, StagingBuffer) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Staging buffer creation has failed!");
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


	//Create and upload first sampled texture in host visible memory
	GFX_API::GFXHandle AlitaTexture;
	unsigned int AlitaSize = 0, AlitaOffset = 0;
	{
		GFX_API::Texture_Description im_desc;
		void* DATA = nullptr;
		if (TuranEditor::Texture_Loader::Import_Texture("C:/Users/furka/Pictures/Wallpapers/Thumbnail.png", im_desc, DATA) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture import has failed!");
		}

		im_desc.USAGE.isRandomlyWrittenTo = true;
		if (GFXContentManager->Create_Texture(im_desc, 0, AlitaTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Alita texture creation has failed!");
		}
		AlitaSize = im_desc.WIDTH * im_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(im_desc.Properties.CHANNEL_TYPE);
		AlitaOffset = 944;
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, DATA, AlitaSize, AlitaOffset) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the Alita texture has failed!");
		}
		delete DATA;
	}


	//Create and Link Texture Display Material Type/Instance
	GFX_API::GFXHandle FIRSTSAMPLINGTYPE_ID, TEXTUREDISPLAY_MATTYPE, TEXTUREDISPLAY_MATINST;
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
		if (GFXContentManager->Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D, 0, 0, GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP,
			GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT,
			GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, FIRSTSAMPLINGTYPE_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Creation of sampling type has failed, so application has!");
		}


		if (GFXContentManager->Create_MaterialInst(TEXTUREDISPLAY_MATTYPE, TEXTUREDISPLAY_MATINST) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture Display Material Instance creation has failed!");
		}
		if (GFXContentManager->SetMaterial_ImageTexture(TEXTUREDISPLAY_MATTYPE, true, false, 0, 0, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEWRITE) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture Display Material Type's Alita image texture setting has failed!");
		}
	}
	//Upload Camera Data
	{
		mat4 matrixes[4];
		matrixes[0] = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		matrixes[1] = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		matrixes[2] = mat4(mat3(matrixes[1]));
		matrixes[3] = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10000.0f);

		if (GFXContentManager->Upload_toBuffer(FirstGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, &matrixes, 256, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the world matrix data has failed!");
		}
	}




	//Create skybox's attribute layout and mesh buffer for the skybox cube
	GFX_API::GFXHandle SKYBOXVB_ID = nullptr, SKYBOXVAL = nullptr;
	{
		float skyboxVertices[] = {
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		GFXContentManager->Create_VertexAttributeLayout({ GFX_API::DATA_TYPE::VAR_VEC3 }, GFX_API::VERTEXLIST_TYPEs::TRIANGLELIST, SKYBOXVAL);
		if (GFXContentManager->Create_VertexBuffer(SKYBOXVAL, 36, 0, SKYBOXVB_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Skybox Vertex Buffer creation has failed!");
		}

		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, skyboxVertices, sizeof(vec3) * 36, 512) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex buffer to staging buffer has failed!");
		}
	}

	//Create and upload skybox cubemap texture
	GFX_API::GFXHandle SkyboxTexture = nullptr;
	unsigned int SkyBoxSize = 0, SkyBoxOffset = 0;
	{
		GFX_API::Texture_Description im_desc;

		im_desc.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_CUBE;
		im_desc.USAGE.isSampledReadOnly = true;
		im_desc.HEIGHT = 2048;
		im_desc.WIDTH = 2048;
		im_desc.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB;
		im_desc.Properties.DATAORDER = GFX_API::TEXTURE_ORDER::SWIZZLE;
		im_desc.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP;
		im_desc.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
		if (GFXContentManager->Create_Texture(im_desc, 0, SkyboxTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Skybox texture creation has failed!");
		}
		SkyBoxSize = im_desc.WIDTH * im_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(im_desc.Properties.CHANNEL_TYPE) * 6;
		SkyBoxOffset = AlitaOffset + AlitaSize;

		for (unsigned int i = 0; i < 6; i++) {
			void* DATA = nullptr;
			if (TuranEditor::Texture_Loader::Import_Texture(("C:/Users/furka/Desktop/" + to_string(i) + ".jpg").c_str(), im_desc, DATA, false) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("Skybox texture import has failed!");
			}

			if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, DATA, SkyBoxSize / 6, SkyBoxOffset + (SkyBoxSize * i) / 6) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("Uploading the Skybox texture has failed!");
			}

			GFXRENDERER->ImageBarrier(SkyboxTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, i, BarrierBeforeUpload_ID);
			GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, SkyboxTexture, SkyBoxOffset + (SkyBoxSize * i) / 6, { 0,0,0,0,0,0 }, i);
			GFXRENDERER->ImageBarrier(SkyboxTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, i, BarrierAfterUpload_ID);
			delete DATA;
		}

	}

	//Create and Link Skybox Display Material Type/Instance
	GFX_API::GFXHandle SKYBOXDISPLAY_MATTYPE, SKYBOXDISPLAY_MATINST;
	{
		GFX_API::GFXHandle VS_ID, FS_ID;
		unsigned int VS_CODESIZE = 0, FS_CODESIZE = 0;
		char* VS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/SkyBoxVert.spv", VS_CODESIZE);
		char* FS_CODE = (char*)TAPIFILESYSTEM::Read_BinaryFile("C:/dev/VulkanRenderer/Content/SkyBoxFrag.spv", FS_CODESIZE);
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
			first_desc.NAME = "SkyBoxTexture";
			first_desc.SHADERSTAGEs.FRAGMENTSHADER = true;
			first_desc.TYPE = GFX_API::SHADERINPUT_TYPE::SAMPLER_G;
			MATTYPE.MATERIALTYPEDATA.push_back(first_desc);
		}
		MATTYPE.ATTRIBUTELAYOUT_ID = SKYBOXVAL;
		MATTYPE.culling = GFX_API::CULL_MODE::CULL_OFF;
		MATTYPE.polygon = GFX_API::POLYGON_MODE::FILL;
		MATTYPE.depthtest = GFX_API::DEPTH_TESTs::DEPTH_TEST_LESS;
		MATTYPE.depthmode = GFX_API::DEPTH_MODEs::DEPTH_READ_ONLY;
		MATTYPE.frontfacedstencil.CompareOperation = GFX_API::STENCIL_COMPARE::ALWAYS_PASS;
		MATTYPE.frontfacedstencil.DepthFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.DepthSuccess = GFX_API::STENCIL_OP::CHANGE;
		MATTYPE.frontfacedstencil.STENCILCOMPAREMASK = 0xFF;
		MATTYPE.frontfacedstencil.StencilFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.STENCILVALUE = 255;
		MATTYPE.frontfacedstencil.STENCILWRITEMASK = 0xFF;
		if (GFXContentManager->Link_MaterialType(MATTYPE, SKYBOXDISPLAY_MATTYPE) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Link MaterialType has failed!");
		}


		if (GFXContentManager->Create_MaterialInst(SKYBOXDISPLAY_MATTYPE, SKYBOXDISPLAY_MATINST) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("SkyBox Display Material Instance creation has failed!");
		}
		if (GFXContentManager->SetMaterial_SampledTexture(SKYBOXDISPLAY_MATTYPE, true, false, 0, 0, SkyboxTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("SkyBox Display Material Type's Alita image texture setting has failed!");
		}
	}




	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, 0, BarrierBeforeUpload_ID);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VERTEXBUFFER_ID, GFX_API::BUFFER_TYPE::VERTEX, 0, 0, sizeof(Vertex) * 4);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, SKYBOXVB_ID, GFX_API::BUFFER_TYPE::VERTEX, 512, 0, sizeof(vec3) * 36);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, INDEXBUFFER_ID, GFX_API::BUFFER_TYPE::INDEX, 80, 0, 24);
	GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, AlitaTexture, AlitaOffset, { 0,0,0,0,0,0 }, 0);
	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEWRITE, 0, BarrierAfterUpload_ID);
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

	new Main_Window;
	unsigned int i = 0;
	while (true) {
		TURAN_PROFILE_SCOPE_MCS("Run Loop");

		IMGUI_RUNWINDOWS();
		GFXRENDERER->ImageBarrier(AlitaSwapchains[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, 0, BarrierBeforeUpload_ID);
		GFXRENDERER->DrawDirect(VERTEXBUFFER_ID, INDEXBUFFER_ID, 0, 0, 0, 1, 0, TEXTUREDISPLAY_MATINST, WorldSubpassID);
		GFXRENDERER->DrawDirect(SKYBOXVB_ID, nullptr, 0, 0, 0, 1, 0, SKYBOXDISPLAY_MATINST, WorldSubpassID);
		GFXRENDERER->ImageBarrier(AlitaSwapchains[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, 0, BarrierAfterDraw_ID);
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
	
}