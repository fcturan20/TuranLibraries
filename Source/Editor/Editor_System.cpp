#include "Main.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"

namespace TuranEditor {
	Editor_System::Editor_System(TuranAPI::Threading::JobSystem& JobSystem) {
		std::cout << "Editor System Constructor is started!\n";
		new Vulkan::Vulkan_Core(Monitors, GPUs, JobSystem);
	}
	Editor_System::~Editor_System() {
		delete GFX;
	}
	void Editor_System::Take_Inputs() {
		GFX->Take_Inputs();
	}
	bool Editor_System::Should_EditorClose() {
		return Should_EditorClose;
	}
	bool Editor_System::Should_Close = false;
	//Unit Test for RenderGraph's Construction under complex Last Frame inputs
	void RenderGraphConstruction_LastFrameUT() {
		
		GFXRENDERER->Start_RenderGraphConstruction();
		GFX_API::GFXHandle TP1, TP2, TP3, DPA, DPB, DPC, DPD, DPE, DPF, WPA;

		//TP1
		{
			GFX_API::PassWait_Description WPAdep;
			WPAdep.WaitedPass = &WPA;
			WPAdep.WaitedStage.SWAPCHAINDISPLAY = true;
			WPAdep.WaitLastFramesPass = false;
			GFXRENDERER->Create_TransferPass({WPAdep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "TP1", TP1);
		}

		//TP2
		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "TP2", TP2);

		//TP3
		{
			GFX_API::PassWait_Description DPBdep;
			DPBdep.WaitLastFramesPass = false;
			DPBdep.WaitedStage.TRANSFERCMD = true;
			DPBdep.WaitedPass = &DPB;
			GFXRENDERER->Create_TransferPass({DPBdep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "TP3", TP3);
		}

		//DPA
		{
			GFX_API::PassWait_Description TP1dep;
			TP1dep.WaitLastFramesPass = false;
			TP1dep.WaitedStage.TRANSFERCMD = true;
			TP1dep.WaitedPass = &TP1;


			GFX_API::PassWait_Description DPDdep;
			DPDdep.WaitLastFramesPass = true;
			DPDdep.WaitedStage.TRANSFERCMD = true;
			DPDdep.WaitedPass = &DPD;
			
			GFXRENDERER->Create_TransferPass({TP1dep, DPDdep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPA", DPA);
		}
		//DPB
		{
			GFX_API::PassWait_Description DPAdep;
			DPAdep.WaitLastFramesPass = false;
			DPAdep.WaitedStage.TRANSFERCMD = true;
			DPAdep.WaitedPass = &DPA;
			GFXRENDERER->Create_TransferPass({DPAdep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPB", DPB);
		}
		//DPC
		{
			GFX_API::PassWait_Description TP1dep;
			TP1dep.WaitLastFramesPass = false;
			TP1dep.WaitedStage.TRANSFERCMD = true;
			TP1dep.WaitedPass = &TP1;


			GFX_API::PassWait_Description TP2dep;
			TP2dep.WaitLastFramesPass = false;
			TP2dep.WaitedStage.TRANSFERCMD = true;
			TP2dep.WaitedPass = &TP2;

			GFXRENDERER->Create_TransferPass({ TP1dep, TP2dep }, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPC", DPC);
		}
		//DPD
		{
			GFX_API::PassWait_Description DPBdep;
			DPBdep.WaitLastFramesPass = false;
			DPBdep.WaitedStage.TRANSFERCMD = true;
			DPBdep.WaitedPass = &DPB;


			GFX_API::PassWait_Description DPCdep;
			DPCdep.WaitLastFramesPass = false;
			DPCdep.WaitedStage.TRANSFERCMD = true;
			DPCdep.WaitedPass = &DPC;

			GFXRENDERER->Create_TransferPass({DPBdep, DPCdep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPD", DPD);
		}
		//DPE
		{
			GFX_API::PassWait_Description DPDdep;
			DPDdep.WaitLastFramesPass = false;
			DPDdep.WaitedStage.TRANSFERCMD = true;
			DPDdep.WaitedPass = &DPD;



			GFX_API::PassWait_Description TP3dep;
			TP3dep.WaitLastFramesPass = false;
			TP3dep.WaitedStage.TRANSFERCMD = true;
			TP3dep.WaitedPass = &TP3;

			GFXRENDERER->Create_TransferPass({DPDdep, TP3dep}, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPE", DPE);
		}
		//DPF
		{
			GFX_API::PassWait_Description DPEdep;
			DPEdep.WaitLastFramesPass = false;
			DPEdep.WaitedStage.TRANSFERCMD = true;
			DPEdep.WaitedPass = &DPE;
			GFXRENDERER->Create_TransferPass({ DPEdep }, GFX_API::TRANFERPASS_TYPE::TP_UPLOAD, "DPF", DPF);
		}
		//WPA
		{
			GFX_API::PassWait_Description DPFdep;
			DPFdep.WaitedPass = &DPF;
			DPFdep.WaitedStage.TRANSFERCMD = true;
			DPFdep.WaitLastFramesPass = false;
			GFXRENDERER->Create_WindowPass({ DPFdep }, "WPA", WPA);
		}
		GFXRENDERER->Finish_RenderGraphConstruction();
		LOG_CRASHING_TAPI("Application is successful, this is just to stop!");
		
	}
	void RenderGraphConstruction_DrawPassed()
	{
		GFX_API::GFXHandle RTSlotSet_ID;
		GFX_API::GFXHandle FIRSTSUBPASS_ID, FIRSTDRAWPASS_ID;

		GFXRENDERER->Start_RenderGraphConstruction();

		GFX_API::GFXHandle BarrierTP;
		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "Resource Creation TP", BarrierTP);

		vector<GFX_API::RTSLOT_Description> RTSlots;
		//Create RT with Base RTSlotSet
		{
			GFX_API::GFXHandle FIRSTRT;
			GFX_API::Texture_Resource COLORRT;
			COLORRT.WIDTH = 1280;
			COLORRT.HEIGHT = 720;
			COLORRT.USAGE.isCopiableFrom = true;
			COLORRT.USAGE.isCopiableTo = false;
			COLORRT.USAGE.isRandomlyWrittenTo = false;
			COLORRT.USAGE.isRenderableTo = true;
			COLORRT.USAGE.isSampledReadOnly = false;
			COLORRT.Properties.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB;
			COLORRT.Properties.DIMENSION = GFX_API::TEXTURE_DIMENSIONs::TEXTURE_2D;
			COLORRT.Properties.MIPMAP_FILTERING = GFX_API::TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP;
			COLORRT.Properties.WRAPPING = GFX_API::TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
			GFXContentManager->Create_Texture(COLORRT, nullptr, GFX_API::IMAGEUSAGE::READWRITE_RTATTACHMENT, BarrierTP, FIRSTRT);

			GFX_API::RTSLOT_Description FirstRTSLOT;
			FirstRTSLOT.CLEAR_VALUE = vec4(0.1f, 0.5f, 0.5f, 1.0f);
			FirstRTSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			FirstRTSLOT.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
			FirstRTSLOT.TextureHandles[0] = FIRSTRT;
			FirstRTSLOT.TextureHandles[1] = FIRSTRT;
			FirstRTSLOT.SLOTINDEX = 0;
			RTSlots.push_back(FirstRTSLOT);
		}
		GFXContentManager->Create_RTSlotset(RTSlots, RTSlotSet_ID);

		GFX_API::RTSLOTUSAGE_Description IRTSlot;
		IRTSlot.IS_DEPTH = false;
		IRTSlot.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
		IRTSlot.SLOTINDEX = 0;
		GFX_API::GFXHandle ISLOTSET_ID;
		GFXContentManager->Inherite_RTSlotSet({ IRTSlot }, RTSlotSet_ID, ISLOTSET_ID);

		GFX_API::SubDrawPass_Description Subpass_desc;
		Subpass_desc.INHERITEDSLOTSET = ISLOTSET_ID;
		Subpass_desc.SubDrawPass_Index = 0;
		vector<GFX_API::SubDrawPass_Description> DESCS{ Subpass_desc };
		vector<GFX_API::GFXHandle> SPs_ofFIRSTDP;
		GFXRENDERER->Create_DrawPass(DESCS, RTSlotSet_ID, {}, "FirstDP", SPs_ofFIRSTDP, FIRSTDRAWPASS_ID);
		GFXRENDERER->Finish_RenderGraphConstruction();
	}


	/*
		Current Frame Structure:
		* Transfer Pass 1 doesn't wait for any of the current frame passes, it is a root TP. To make it more realistic(Tmimr), it is mesh buffer CPU->GPU transfer
		* Draw Pass A waits for TP-1, they should be executed serially. Tmimr, it is shadow rendering
		* Transfer Pass 2 doesn't wait for any of the current frame passes, it is a root TP (TransferPass). Tmimr, it is material texture CPU->GPU transfer
		* Draw Pass B waits for DP-A (DP-A = Draw Pass A). Tmimr, it's raster based deferred lighting
		* Transfer Pass 3 waits for Draw Pass B. I can't make it realistic, it's just a implementation dependent copy
		* Draw Pass C waits for TP-1 and TP-2. Tmimr, it is G-Buffer rendering
		* Draw Pass D waits for DP-B and DP-C. Tmimr, it is transparent rendering
		* Draw Pass E waits for DP-D and TP-3. Tmimr, it is a raster based swapchain post-proccessing
		* Draw Pass F waits for DP-E. Tmimr, it is a raster based swapchain post-proccessing
	TP - 2  ---- >> DP - C------------------------ -
		^ |
		|							|
		----------------							|
		|											!
		TP - 1  -----> > DP - A---- >> DP - B---- >> DP - D-----> > DP - E---- >> DP - F
		| ^
		|						  |
		|						  |
		---------> > TP - 3 ---------- -

		RPs:
	a) TP - 1
	b) DP - A < ->DP - B
	c) TP - 2
	d) DP - C
	e) DP - D
	f) TP - 2
	g) DP - E < ->DP - F
	Note : Root Passes don't have to be Transfer Passes but in practice, we need to pass GPU some data to process*/
	void RenderGraphConstruction_BasicUT(){
		GFXRENDERER->Start_RenderGraphConstruction();
		GFX_API::GFXHandle TP1, TP2, TP3, DPA, DPB, DPC, DPD, DPE, DPF;
		//TP1
		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "TP1", TP1);

		//TP2
		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "TP2", TP2);
		//TP3
		{
			GFX_API::PassWait_Description DPBdep;
			DPBdep.WaitLastFramesPass = false;
			DPBdep.WaitedStage.TRANSFERCMD = true;
			DPBdep.WaitedPass = &DPB;
			GFXRENDERER->Create_TransferPass({DPBdep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "TP3", TP3);
		}
		//DPA
		{
			GFX_API::PassWait_Description TP1dep;
			TP1dep.WaitLastFramesPass = false;
			TP1dep.WaitedStage.TRANSFERCMD = true;
			TP1dep.WaitedPass = &TP1;
			
			GFXRENDERER->Create_TransferPass({ TP1dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPA", DPA);
		}
		//DPB
		{
			GFX_API::PassWait_Description DPAdep;
			DPAdep.WaitLastFramesPass = false;
			DPAdep.WaitedStage.TRANSFERCMD = true;
			DPAdep.WaitedPass = &DPA;

			GFXRENDERER->Create_TransferPass({DPAdep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPB", DPB);
		}
		//DPC
		{
			GFX_API::PassWait_Description TP1dep;
			TP1dep.WaitLastFramesPass = false;
			TP1dep.WaitedStage.TRANSFERCMD = true;
			TP1dep.WaitedPass = &TP1;

			GFX_API::PassWait_Description TP2dep;
			TP2dep.WaitLastFramesPass = false;
			TP2dep.WaitedStage.TRANSFERCMD = true;
			TP2dep.WaitedPass = &TP2;

			GFXRENDERER->Create_TransferPass({TP1dep, TP2dep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPC", DPC);
		}
		//DPD
		{
			GFX_API::PassWait_Description DPBdep;
			DPBdep.WaitLastFramesPass = false;
			DPBdep.WaitedStage.TRANSFERCMD = true;
			DPBdep.WaitedPass = &DPB;


			GFX_API::PassWait_Description DPCdep;
			DPCdep.WaitLastFramesPass = false;
			DPCdep.WaitedStage.TRANSFERCMD = true;
			DPCdep.WaitedPass = &DPC;
			GFXRENDERER->Create_TransferPass({DPBdep, DPCdep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPD", DPD);
		}
		//DPE
		{
			GFX_API::PassWait_Description DPDdep;
			DPDdep.WaitLastFramesPass = false;
			DPDdep.WaitedStage.TRANSFERCMD = true;
			DPDdep.WaitedPass = &DPD;

			GFX_API::PassWait_Description TP3dep;
			TP3dep.WaitLastFramesPass = false;
			TP3dep.WaitedStage.TRANSFERCMD = true;
			TP3dep.WaitedPass = &TP3;

			GFXRENDERER->Create_TransferPass({DPDdep, TP3dep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPE", DPE);
		}
		//DPF
		{
			GFX_API::PassWait_Description DPEdep;
			DPEdep.WaitLastFramesPass = false;
			DPEdep.WaitedStage.TRANSFERCMD = true;
			DPEdep.WaitedPass = &DPE;

			GFXRENDERER->Create_TransferPass({DPEdep}, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "DPF", DPF);
		}
		GFXRENDERER->Finish_RenderGraphConstruction();
	}
}
