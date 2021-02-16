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
		0, FirstGlobalBuffer);


	//Useful rendergraph handles to use later
	GFX_API::GFXHandle COLOR2RT, DEPTHRT, SubpassID, ISlotSetID, WP_ID, FirstBarrierTP_ID, UploadTP_ID, FinalBarrierTP_ID; 

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
		//Create RTs, Base RTSlotSet and the inherited one!
		{
			GFX_API::Texture_Description COLORRT;
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
			if (GFXContentManager->Create_Texture(COLORRT, 0, COLOR2RT) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("First RT creation has failed!");
			}

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
			SWPCHNSLOT.TextureHandles[0] = SwapchainTextures[0];
			SWPCHNSLOT.TextureHandles[1] = SwapchainTextures[1];
			SWPCHNSLOT.isUSEDLATER = true;
			SWPCHNSLOT.SLOTINDEX = 0;
			RTSlots.push_back(SWPCHNSLOT);

			GFX_API::RTSLOT_Description EXTRASLOT;
			EXTRASLOT.CLEAR_VALUE = vec4(0.6f, 0.5f, 0.5f, 1.0f);
			EXTRASLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			EXTRASLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			EXTRASLOT.TextureHandles[0] = COLOR2RT;
			EXTRASLOT.TextureHandles[1] = COLOR2RT;
			EXTRASLOT.isUSEDLATER = true;
			EXTRASLOT.SLOTINDEX = 1;
			RTSlots.push_back(EXTRASLOT);

			GFX_API::RTSLOT_Description DEPTHSLOT;
			DEPTHSLOT.CLEAR_VALUE = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			DEPTHSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
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
			GFX_API::RTSLOTUSAGE_Description IRT_EXTRASLOT;
			IRT_EXTRASLOT.IS_DEPTH = false;
			IRT_EXTRASLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_EXTRASLOT.SLOTINDEX = 1;
			GFX_API::RTSLOTUSAGE_Description IRT_DEPTHSLOT;
			IRT_DEPTHSLOT.IS_DEPTH = true;
			IRT_DEPTHSLOT.OPTYPE = GFX_API::OPERATION_TYPE::READ_AND_WRITE;
			IRT_DEPTHSLOT.SLOTINDEX = 0;
			GFXContentManager->Inherite_RTSlotSet({ IRT_SWPCHNSLOT, IRT_EXTRASLOT, IRT_DEPTHSLOT }, RTSlotSet_ID, ISlotSetID);
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
	if (GFXContentManager->Create_StagingBuffer(1024 * 1024 * 10, 3, StagingBuffer) != TAPI_SUCCESS) {
		LOG_CRASHING_TAPI("Staging buffer creation has failed, fix it!");
	}
	
	//Create first attribute layout and mesh buffer for first triangle
	//Also create index buffer
	GFX_API::GFXHandle VERTEXBUFFER_ID = nullptr, INDEXBUFFER_ID = nullptr, VAL_ID = nullptr;
	struct Vertex {
		vec2 Pos;
		vec2 TextCoord;
	};
	{
		Vertex VertexData[4]{
			{vec2(-1.0f, 1.0f), vec2(0.0f,0.0f)},
			{vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f)},
			{vec2(1.0f, 1.0f), vec2(1.0f, 0.0f)},
			{vec2(1.0f, -1.0f), vec2(1.0f, 1.0f)}
		};
		unsigned int IndexData[6]{ 0, 1, 2, 1, 3, 2 };

		GFX_API::GFXHandle PositionVA_ID, ColorVA_ID;
		GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC2, true, PositionVA_ID);
		GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC2, true, ColorVA_ID);
		vector<GFX_API::GFXHandle> VAs{ PositionVA_ID, ColorVA_ID };
		GFXContentManager->Create_VertexAttributeLayout(VAs, GFX_API::VERTEXLIST_TYPEs::TRIANGLELIST, VAL_ID);
		if (GFXContentManager->Create_VertexBuffer(VAL_ID, 4, 0, VERTEXBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Vertex Buffer creation has failed!");
		}
		
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VertexData, sizeof(Vertex) * 4, 0) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex buffer to staging buffer has failed!");
		}

		if (GFXContentManager->Create_IndexBuffer(GFX_API::DATA_TYPE::VAR_UINT32, 6, 0, INDEXBUFFER_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("First Index Buffer creation has failed!");
		}

		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, IndexData, 24, 64) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading index buffer to staging buffer has failed!");
		}
	}

	//Create and upload first sampled texture in host visible memory
	GFX_API::GFXHandle AlitaTexture, GokuBlackTexture;
	unsigned int AlitaSize = 0, AlitaOffset = 0, GokuBlackOffset = 0, GokuBlackSize;
	{
		GFX_API::Texture_Description im_desc;
		void* DATA = nullptr;
		if (TuranEditor::Texture_Loader::Import_Texture("C:/Users/furka/Pictures/Wallpapers/Thumbnail.png", im_desc, DATA) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture import has failed!");
		}

		if (GFXContentManager->Create_Texture(im_desc, 0, AlitaTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Alita texture creation has failed!");
		}
		AlitaSize = im_desc.WIDTH * im_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(im_desc.Properties.CHANNEL_TYPE);
		AlitaOffset = 104;
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, DATA, AlitaSize, AlitaOffset) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the Alita texture has failed!");
		}
		delete DATA;
		DATA = nullptr;

		if (TuranEditor::Texture_Loader::Import_Texture("C:/Users/furka/Pictures/Wallpapers/Goku Black SSRose.jpg", im_desc, DATA) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Texture import has failed!");
		}

		if (GFXContentManager->Create_Texture(im_desc, 0, GokuBlackTexture) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Goku Black texture creation has failed!");
		}
		GokuBlackSize = im_desc.WIDTH * im_desc.HEIGHT * GFX_API::GetByteSizeOf_TextureChannels(im_desc.Properties.CHANNEL_TYPE);
		GokuBlackOffset = AlitaOffset + AlitaSize;
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, DATA, GokuBlackSize, GokuBlackOffset) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading the Goku Black texture has failed!");
		}
		delete DATA;
	}
	
	//Create and Link Goku Black material type/inst
	GFX_API::GFXHandle VS_ID, FS_ID, FIRSTSAMPLINGTYPE_ID;
	GFX_API::GFXHandle TEXTUREDISPLAY_MATTYPE, GOKUBLACK_MATINST, ALITA_MATINST;
	{
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

			GFX_API::MaterialDataDescriptor second_desc;
			second_desc.BINDINGPOINT = 1;
			second_desc.DATA_SIZE = 0;
			second_desc.NAME = "FirstSampledTexture";
			second_desc.SHADERSTAGEs.FRAGMENTSHADER = true;
			second_desc.TYPE = GFX_API::MATERIALDATA_TYPE::CONSTSAMPLER_PI;
			MATTYPE.MATERIALTYPEDATA.push_back(second_desc);
		}
		MATTYPE.ATTRIBUTELAYOUT_ID = VAL_ID;
		MATTYPE.culling = GFX_API::CULL_MODE::CULL_BACK;
		MATTYPE.polygon = GFX_API::POLYGON_MODE::FILL;
		MATTYPE.depthtest = GFX_API::DEPTH_TESTs::DEPTH_TEST_ALWAYS;
		MATTYPE.depthmode = GFX_API::DEPTH_MODEs::DEPTH_READ_WRITE;
		MATTYPE.frontfacedstencil.CompareOperation = GFX_API::STENCIL_COMPARE::ALWAYS_PASS;
		MATTYPE.frontfacedstencil.DepthFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.DepthSuccess = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.STENCILCOMPAREMASK = 0xFF;
		MATTYPE.frontfacedstencil.StencilFailed = GFX_API::STENCIL_OP::DONT_CHANGE;
		MATTYPE.frontfacedstencil.STENCILVALUE = 255;
		MATTYPE.frontfacedstencil.STENCILWRITEMASK = 0xFF;
		//Blending Infos
		{
			GFX_API::ATTACHMENT_BLENDING blendinginfo;
			blendinginfo.BLENDMODE_ALPHA = GFX_API::BLEND_MODE::ADDITIVE;
			blendinginfo.BLENDMODE_COLOR = GFX_API::BLEND_MODE::ADDITIVE;
			blendinginfo.CONSTANT.r = 1.0f; blendinginfo.CONSTANT.g = 1.0f;
			blendinginfo.CONSTANT.b = 0.0f; blendinginfo.CONSTANT.a = 1.0f;
			blendinginfo.COLORSLOT_INDEX = 0;
			blendinginfo.DISTANCEFACTOR_ALPHA = GFX_API::BLEND_FACTOR::ZERO;
			blendinginfo.DISTANCEFACTOR_COLOR = GFX_API::BLEND_FACTOR::CONST_1MINUSCOLOR;
			blendinginfo.SOURCEFACTOR_ALPHA = GFX_API::BLEND_FACTOR::ONE;
			blendinginfo.SOURCEFACTOR_COLOR = GFX_API::BLEND_FACTOR::CONST_COLOR;
			MATTYPE.BLENDINGINFOS.push_back(blendinginfo);
		}
		if (GFXContentManager->Link_MaterialType(MATTYPE, TEXTUREDISPLAY_MATTYPE) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Link MaterialType has failed!");
		}
		if (GFXContentManager->Create_SamplingType(GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D, 0, 0, GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP,
			GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_NEAREST_FROM_1MIP, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT,
			GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT, FIRSTSAMPLINGTYPE_ID) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Creation of sampling type has failed, so application has!");
		}
		GFXContentManager->SetMaterial_UniformBuffer(TEXTUREDISPLAY_MATTYPE, true, false, 0, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, 88);
		vec3 FragColor(1.0f, 0.0f, 0.0f);
		if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, &FragColor, 12, 88) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Uploading vertex color to staging buffer has failed!");
		}


		if (GFXContentManager->Create_MaterialInst(TEXTUREDISPLAY_MATTYPE, GOKUBLACK_MATINST) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Goku Black Material Instance creation has failed!");
		}
		GFXContentManager->SetMaterial_SampledTexture(GOKUBLACK_MATINST, false, false, 1, GokuBlackTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);


		if (GFXContentManager->Create_MaterialInst(TEXTUREDISPLAY_MATTYPE, ALITA_MATINST) != TAPI_SUCCESS) {
			LOG_CRASHING_TAPI("Alita Material Instance creation has failed!");
		}
		GFXContentManager->SetMaterial_SampledTexture(ALITA_MATINST, false, false, 1, AlitaTexture, FIRSTSAMPLINGTYPE_ID, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY);
	}

	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, VERTEXBUFFER_ID, GFX_API::BUFFER_TYPE::VERTEX, 0, 0, sizeof(Vertex) * 4);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, FirstGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, 88, 0, 12);
	GFXRENDERER->CopyBuffer_toBuffer(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, INDEXBUFFER_ID, GFX_API::BUFFER_TYPE::INDEX, 64, 0, 24);
	GFXRENDERER->ImageBarrier(DEPTHRT, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::DEPTHSTENCIL_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(COLOR2RT, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(GokuBlackTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, FirstBarrierTP_ID);
	GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, INDEXBUFFER_ID, GOKUBLACK_MATINST, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[0], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->ImageBarrier(GokuBlackTexture, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, FinalBarrierTP_ID);
	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();


	GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, GokuBlackTexture, GokuBlackOffset, 0, 0, 0, 0, 0, 0);
	GFXRENDERER->CopyBuffer_toImage(UploadTP_ID, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, AlitaTexture, AlitaOffset, 0, 0, 0, 0, 0, 0);
	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::NO_ACCESS, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(GokuBlackTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, FirstBarrierTP_ID);
	GFXRENDERER->ImageBarrier(AlitaTexture, GFX_API::IMAGE_ACCESS::TRANSFER_DIST, GFX_API::IMAGE_ACCESS::SHADER_SAMPLEONLY, FirstBarrierTP_ID);
	GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, INDEXBUFFER_ID, GOKUBLACK_MATINST, SubpassID);
	GFXRENDERER->ImageBarrier(SwapchainTextures[1], GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, FinalBarrierTP_ID);
	GFXRENDERER->SwapBuffers(WindowHandle, WP_ID);
	GFXRENDERER->Run();
	Editor_System::Take_Inputs();


	unsigned int i = 0;
	while (true) {
		TURAN_PROFILE_SCOPE_MCS("Run Loop");

		if (i % 2) {
			GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, INDEXBUFFER_ID, ALITA_MATINST, SubpassID);
		}
		else {
			GFXRENDERER->Render_DrawCall(VERTEXBUFFER_ID, INDEXBUFFER_ID, ALITA_MATINST, SubpassID);
		}
		GFXRENDERER->ImageBarrier(SwapchainTextures[GFXRENDERER->GetCurrentFrameIndex()], GFX_API::IMAGE_ACCESS::SWAPCHAIN_DISPLAY, GFX_API::IMAGE_ACCESS::RTCOLOR_READWRITE, FirstBarrierTP_ID);
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