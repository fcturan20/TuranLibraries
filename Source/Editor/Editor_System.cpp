#include "Main.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"

namespace TuranEditor {
	Editor_System::Editor_System(TuranAPI::Threading::JobSystem* JobSystem) {
		std::cout << "Editor System Constructor is started!\n";
		Vulkan::Vulkan_Core* VK = new Vulkan::Vulkan_Core(Monitors, GPUs, JobSystem);
		VK->Start_SecondStage(0, 1024 * 1024 * 20, 1024 * 1024 * 20, 0, 0);
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
			GFXRENDERER->Create_TransferPass({WPAdep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "TP1", TP1);
		}

		//TP2
		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "TP2", TP2);

		//TP3
		{
			GFX_API::PassWait_Description DPBdep;
			DPBdep.WaitLastFramesPass = false;
			DPBdep.WaitedStage.TRANSFERCMD = true;
			DPBdep.WaitedPass = &DPB;
			GFXRENDERER->Create_TransferPass({DPBdep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "TP3", TP3);
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
			
			GFXRENDERER->Create_TransferPass({TP1dep, DPDdep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPA", DPA);
		}
		//DPB
		{
			GFX_API::PassWait_Description DPAdep;
			DPAdep.WaitLastFramesPass = false;
			DPAdep.WaitedStage.TRANSFERCMD = true;
			DPAdep.WaitedPass = &DPA;
			GFXRENDERER->Create_TransferPass({DPAdep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPB", DPB);
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

			GFXRENDERER->Create_TransferPass({ TP1dep, TP2dep }, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPC", DPC);
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

			GFXRENDERER->Create_TransferPass({DPBdep, DPCdep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPD", DPD);
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

			GFXRENDERER->Create_TransferPass({DPDdep, TP3dep}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPE", DPE);
		}
		//DPF
		{
			GFX_API::PassWait_Description DPEdep;
			DPEdep.WaitLastFramesPass = false;
			DPEdep.WaitedStage.TRANSFERCMD = true;
			DPEdep.WaitedPass = &DPE;
			GFXRENDERER->Create_TransferPass({ DPEdep }, GFX_API::TRANFERPASS_TYPE::TP_COPY, "DPF", DPF);
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
	void RenderGraphConstruction_DrawPassed(GFX_API::GFXHandle SWPCHT0, GFX_API::GFXHandle SWPCHT1, GFX_API::GFXHandle& SubpassID, GFX_API::GFXHandle& IRTSlotSetID, GFX_API::GFXHandle& WindowPassHandle
		, GFX_API::GFXHandle& FirstBarrierTPHandle, GFX_API::GFXHandle& UploadTPHandle, GFX_API::GFXHandle& FinalBarrierTPHandle) {
		//Handles that're not used outside of the function
		GFX_API::GFXHandle FIRSTDRAWPASS_ID, RTSlotSet_ID;

		GFXRENDERER->Start_RenderGraphConstruction();

		GFXRENDERER->Create_TransferPass({}, GFX_API::TRANFERPASS_TYPE::TP_COPY, "Uploader", UploadTPHandle);

		//First Barrier TP
		//This pass depends on both the uploader (changes the layouts of the uploaded textures)
		//and also 2 frames ago's swapchain display (because this'll change the layout from SWPCHN_DSPLY to RTCOLORATTACHMENT)
		{
			GFX_API::PassWait_Description Upload_dep;
			Upload_dep.WaitedPass = &UploadTPHandle;
			Upload_dep.WaitedStage.TRANSFERCMD = true;
			Upload_dep.WaitLastFramesPass = false;

			GFX_API::PassWait_Description WP_dep;
			WP_dep.WaitedPass = &WindowPassHandle;
			WP_dep.WaitedStage.SWAPCHAINDISPLAY = true;
			WP_dep.WaitLastFramesPass = false;

			GFXRENDERER->Create_TransferPass({ Upload_dep, WP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "First Barrier TP", FirstBarrierTPHandle);
		}

		vector<GFX_API::RTSLOT_Description> RTSlots;
		//Create RT, Base RTSlotSet and the inherited one!
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
			GFXContentManager->Create_Texture(COLORRT, GFX_API::SUBALLOCATEBUFFERTYPEs::DEVICELOCAL, GFX_API::IMAGEUSAGE::READWRITE_RTATTACHMENT, FIRSTRT);

			GFX_API::RTSLOT_Description FirstRTSLOT;
			FirstRTSLOT.CLEAR_VALUE = vec4(0.1f, 0.5f, 0.5f, 1.0f);
			FirstRTSLOT.LOADOP = GFX_API::DRAWPASS_LOAD::CLEAR;
			FirstRTSLOT.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
			FirstRTSLOT.TextureHandles[0] = SWPCHT0;
			FirstRTSLOT.TextureHandles[1] = SWPCHT1;
			FirstRTSLOT.SLOTINDEX = 0;
			RTSlots.push_back(FirstRTSLOT);

			GFXContentManager->Create_RTSlotset(RTSlots, RTSlotSet_ID);


			GFX_API::RTSLOTUSAGE_Description IRTSlot;
			IRTSlot.IS_DEPTH = false;
			IRTSlot.OPTYPE = GFX_API::OPERATION_TYPE::WRITE_ONLY;
			IRTSlot.SLOTINDEX = 0;
			GFXContentManager->Inherite_RTSlotSet({ IRTSlot }, RTSlotSet_ID, IRTSlotSetID);
		}

		//Create Draw Pass
		{
			GFX_API::SubDrawPass_Description Subpass_desc;
			Subpass_desc.INHERITEDSLOTSET = IRTSlotSetID;
			Subpass_desc.SubDrawPass_Index = 0;
			vector<GFX_API::SubDrawPass_Description> DESCS{ Subpass_desc };
			vector<GFX_API::GFXHandle> SPs_ofFIRSTDP;


			GFX_API::PassWait_Description FirstBarrierTP_dep;
			FirstBarrierTP_dep.WaitedPass = &FirstBarrierTPHandle;
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
			GFXRENDERER->Create_TransferPass({ DP_dep }, GFX_API::TRANFERPASS_TYPE::TP_BARRIER, "Final Barrier TP", FinalBarrierTPHandle);
		}

		//Create Window Pass
		{
			GFX_API::PassWait_Description FTP_ID;
			FTP_ID.WaitedPass = &FinalBarrierTPHandle;
			FTP_ID.WaitedStage.TRANSFERCMD = true;
			FTP_ID.WaitLastFramesPass = false;
			GFXRENDERER->Create_WindowPass({ FTP_ID }, "First WP", WindowPassHandle);
		}

		GFXRENDERER->Finish_RenderGraphConstruction();
	}


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
