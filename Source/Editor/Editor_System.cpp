#include "Main.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"

namespace TuranEditor {
	Editor_System::Editor_System(TuranAPI::Threading::JobSystem* JobSystem) : LOGGING("C:/dev/VulkanRenderer") {
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
