#include "Main.h"
using namespace TuranEditor;

int main() {
	Editor_System Systems;

	//Create RenderGraph
	GFX_API::GFXHandle RTSlotSet_ID;
	GFX_API::GFXHandle FIRSTSUBPASS_ID, FIRSTDRAWPASS_ID;

	RenderGraphConstruction_LastFrameUT();

	/*
	{
		GFXRENDERER->Start_RenderGraphConstruction();
		vector<GFX_API::RTSLOT_Description> RTSlots;

		//Create RT with First Base RTSlotSet
		{
			GFX_API::Texture_Description COLORRT;
			COLORRT.WIDTH = GFX->Main_Window->Get_Window_Mode().x;
			COLORRT.HEIGHT = GFX->Main_Window->Get_Window_Mode().y;
			COLORRT.USAGE.isCopiableFrom = true;
			COLORRT.USAGE.isCopiableTo = false;
			COLORRT.USAGE.isRandomlyWrittenTo = false;
			COLORRT.USAGE.isRenderableTo = true;
			COLORRT.USAGE.isSampledReadOnly = false;
			COLORRT.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB;
			COLORRT.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D;
			COLORRT.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP;
			COLORRT.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
			GFX_API::GFXHandle FIRSTRT = GFXContentManager->Create_Texture(COLORRT, 0);

			GFX_API::RTSLOT_Description FirstRTSLOT;
			FirstRTSLOT.CLEAR_VALUE = vec4(0.1f, 0.5f, 0.5f, 1.0f);
			FirstRTSLOT.IS_SWAPCHAIN = GFX_API::SWAPCHAIN_IDENTIFIER::CURRENTFRAME_SWPCHN;
			FirstRTSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			FirstRTSLOT.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
			//FirstRTSLOT.RT_Handle = FIRSTRT;
			FirstRTSLOT.SLOTINDEX = 0;
			RTSlots.push_back(FirstRTSLOT);
		}
		RTSlotSet_ID = GFXContentManager->Create_RTSlotSet(RTSlots);

		GFX_API::IRTSLOT_Description IRTSlot;
		IRTSlot.IS_DEPTH = false;
		IRTSlot.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
		IRTSlot.SLOTINDEX = 0;
		GFX_API::GFXHandle ISLOTSET_ID = GFXContentManager->Inherite_RTSlotSet(RTSlotSet_ID, { IRTSlot });

		GFX_API::SubDrawPass_Description Subpass_desc;
		Subpass_desc.INHERITEDSLOTSET = ISLOTSET_ID;
		Subpass_desc.SubDrawPass_Index = 0;
		vector<GFX_API::SubDrawPass_Description> DESCS{ Subpass_desc };
		FIRSTDRAWPASS_ID = GFXRENDERER->Create_DrawPass(DESCS, RTSlotSet_ID, {}, &FIRSTSUBPASS_ID, "FirstDP");
		GFXRENDERER->Finish_RenderGraphConstruction();
	}*/
	
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

		GFX_API::GFXHandle PositionVA_ID = GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC2, true);
		GFX_API::GFXHandle ColorVA_ID = GFXContentManager->Create_VertexAttribute(GFX_API::DATA_TYPE::VAR_VEC3, true);
		vector<GFX_API::GFXHandle> VAs{ PositionVA_ID, ColorVA_ID };
		VAL_ID = GFXContentManager->Create_VertexAttributeLayout(VAs);
		MESHBUFFER_ID = GFXContentManager->Create_MeshBuffer(VAL_ID, VertexData, 3, nullptr, 0, nullptr //WorldMeshBufferTransferPass
		);
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
		GFX_API::GFXHandle VS_ID = GFXContentManager->Compile_ShaderSource(&VS);
		GFX_API::GFXHandle FS_ID = GFXContentManager->Compile_ShaderSource(&FS);
		GFX_API::Material_Type MATTYPE;
		MATTYPE.VERTEXSOURCE_ID = VS_ID;
		MATTYPE.FRAGMENTSOURCE_ID = FS_ID;
		MATTYPE.SubDrawPass_ID = FIRSTSUBPASS_ID;
		MATTYPE.MATERIALTYPEDATA.clear();	//There is no buffer or texture access for now!
		MATTYPE.RTSLOTSET_ID = RTSlotSet_ID;
		MATTYPE.ATTRIBUTELAYOUT_ID = VAL_ID;
		GFX_API::GFXHandle MATTYPE_ID = GFXContentManager->Link_MaterialType(&MATTYPE);
		GFX_API::Material_Instance MATINST;
		MATINST.MATERIALDATAs.clear();		//There is no buffer or texture access for now!
		MATINST.Material_Type = MATTYPE_ID;
		FIRSTMATINST_ID = GFXContentManager->Create_MaterialInst(&MATINST);
	}
	vector<GFX_API::GFXHandle> DrawCalls;
	//Cook Draw Call
	{
		GFX_API::DrawCall_Description DrawCall_desc;
		DrawCall_desc.MeshBuffer_ID = MESHBUFFER_ID;
		DrawCall_desc.ShaderInstance_ID = FIRSTMATINST_ID;
		DrawCall_desc.SubDrawPass_ID = FIRSTSUBPASS_ID;
		GFXRENDERER->Render_DrawCall(DrawCall_desc);
	}

	unsigned int i = 0;
	while (true) {
		TURAN_PROFILE_SCOPE("Run Loop");


		//Cook Draw Call
		{
			GFX_API::DrawCall_Description DrawCall_desc;
			DrawCall_desc.MeshBuffer_ID = MESHBUFFER_ID;
			DrawCall_desc.ShaderInstance_ID = FIRSTMATINST_ID;
			DrawCall_desc.SubDrawPass_ID = FIRSTSUBPASS_ID;
			GFXRENDERER->Render_DrawCall(DrawCall_desc);
		}
		GFXRENDERER->Run();
		//IMGUI->New_Frame();
		//IMGUI_RUNWINDOWS();
		GFX->Swapbuffers_ofMainWindow();


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

	return 1;
}