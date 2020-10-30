#include "Main.h"
#include "Vulkan/VulkanSource/Vulkan_Core.h"

namespace TuranEditor {
	Editor_System::Editor_System() {
		new Vulkan::Vulkan_Core;
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
	
	void RenderGraphConstruction_LastFrameUT() {
		GFXRENDERER->Start_RenderGraphConstruction();
		GFX_API::GFXHandle TP1, TP2, TP3, DPA, DPB, DPC, DPD, DPE, DPF;
		//TP1
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP1";
			TP1 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//TP2
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP2";
			TP2 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//TP3
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP3";
			{
				GFX_API::PassWait_Description DPBdep;
				DPBdep.WaitLastFramesPass = false;
				DPBdep.WaitedStage.TRANSFERCMD = true;
				DPBdep.WaitedPass = &DPB;
				TPDesc.WaitDescriptions.push_back(DPBdep);
			}
			TP3 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPA
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPA";
			{
				GFX_API::PassWait_Description TP1dep;
				TP1dep.WaitLastFramesPass = false;
				TP1dep.WaitedStage.TRANSFERCMD = true;
				TP1dep.WaitedPass = &TP1;
				TPDesc.WaitDescriptions.push_back(TP1dep);
			}
			DPA = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPB
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPB";
			{
				GFX_API::PassWait_Description DPAdep;
				DPAdep.WaitLastFramesPass = false;
				DPAdep.WaitedStage.TRANSFERCMD = true;
				DPAdep.WaitedPass = &DPA;
				TPDesc.WaitDescriptions.push_back(DPAdep);
			}
			DPB = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPC
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPC";
			{
				GFX_API::PassWait_Description TP1dep;
				TP1dep.WaitLastFramesPass = false;
				TP1dep.WaitedStage.TRANSFERCMD = true;
				TP1dep.WaitedPass = &TP1;
				TPDesc.WaitDescriptions.push_back(TP1dep);

				GFX_API::PassWait_Description TP2dep;
				TP2dep.WaitLastFramesPass = false;
				TP2dep.WaitedStage.TRANSFERCMD = true;
				TP2dep.WaitedPass = &TP2;
				TPDesc.WaitDescriptions.push_back(TP2dep);
			}
			DPC = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPD
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPD";
			{
				GFX_API::PassWait_Description DPBdep;
				DPBdep.WaitLastFramesPass = false;
				DPBdep.WaitedStage.TRANSFERCMD = true;
				DPBdep.WaitedPass = &DPB;
				TPDesc.WaitDescriptions.push_back(DPBdep);


				GFX_API::PassWait_Description DPCdep;
				DPCdep.WaitLastFramesPass = false;
				DPCdep.WaitedStage.TRANSFERCMD = true;
				DPCdep.WaitedPass = &DPC;
				TPDesc.WaitDescriptions.push_back(DPCdep);
			}
			DPD = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPE
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPE";
			{
				GFX_API::PassWait_Description DPDdep;
				DPDdep.WaitLastFramesPass = true;
				DPDdep.WaitedStage.TRANSFERCMD = true;
				DPDdep.WaitedPass = &DPD;
				TPDesc.WaitDescriptions.push_back(DPDdep);

				GFX_API::PassWait_Description TP3dep;
				TP3dep.WaitLastFramesPass = false;
				TP3dep.WaitedStage.TRANSFERCMD = true;
				TP3dep.WaitedPass = &TP3;
				TPDesc.WaitDescriptions.push_back(TP3dep);
			}
			DPE = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPF
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPF";
			{
				GFX_API::PassWait_Description DPEdep;
				DPEdep.WaitLastFramesPass = false;
				DPEdep.WaitedStage.TRANSFERCMD = true;
				DPEdep.WaitedPass = &DPE;
				TPDesc.WaitDescriptions.push_back(DPEdep);
			}
			DPF = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		GFXRENDERER->Finish_RenderGraphConstruction();
		LOG_CRASHING_TAPI("Application is successful, this is just to stop!");
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
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP1";
			TP1 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//TP2
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP2";
			TP2 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//TP3
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "TP3";
			{
				GFX_API::PassWait_Description DPBdep;
				DPBdep.WaitLastFramesPass = false;
				DPBdep.WaitedStage.TRANSFERCMD = true;
				DPBdep.WaitedPass = &DPB;
				TPDesc.WaitDescriptions.push_back(DPBdep);
			}
			TP3 = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPA
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPA";
			{
				GFX_API::PassWait_Description TP1dep;
				TP1dep.WaitLastFramesPass = false;
				TP1dep.WaitedStage.TRANSFERCMD = true;
				TP1dep.WaitedPass = &TP1;
				TPDesc.WaitDescriptions.push_back(TP1dep);
			}
			DPA = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPB
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPB";
			{
				GFX_API::PassWait_Description DPAdep;
				DPAdep.WaitLastFramesPass = false;
				DPAdep.WaitedStage.TRANSFERCMD = true;
				DPAdep.WaitedPass = &DPA;
				TPDesc.WaitDescriptions.push_back(DPAdep);
			}
			DPB = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPC
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPC";
			{
				GFX_API::PassWait_Description TP1dep;
				TP1dep.WaitLastFramesPass = false;
				TP1dep.WaitedStage.TRANSFERCMD = true;
				TP1dep.WaitedPass = &TP1;
				TPDesc.WaitDescriptions.push_back(TP1dep);

				GFX_API::PassWait_Description TP2dep;
				TP2dep.WaitLastFramesPass = false;
				TP2dep.WaitedStage.TRANSFERCMD = true;
				TP2dep.WaitedPass = &TP2;
				TPDesc.WaitDescriptions.push_back(TP2dep);
			}
			DPC = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPD
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPD";
			{
				GFX_API::PassWait_Description DPBdep;
				DPBdep.WaitLastFramesPass = false;
				DPBdep.WaitedStage.TRANSFERCMD = true;
				DPBdep.WaitedPass = &DPB;
				TPDesc.WaitDescriptions.push_back(DPBdep);


				GFX_API::PassWait_Description DPCdep;
				DPCdep.WaitLastFramesPass = false;
				DPCdep.WaitedStage.TRANSFERCMD = true;
				DPCdep.WaitedPass = &DPC;
				TPDesc.WaitDescriptions.push_back(DPCdep);
			}
			DPD = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPE
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPE";
			{
				GFX_API::PassWait_Description DPDdep;
				DPDdep.WaitLastFramesPass = false;
				DPDdep.WaitedStage.TRANSFERCMD = true;
				DPDdep.WaitedPass = &DPD;
				TPDesc.WaitDescriptions.push_back(DPDdep);

				GFX_API::PassWait_Description TP3dep;
				TP3dep.WaitLastFramesPass = false;
				TP3dep.WaitedStage.TRANSFERCMD = true;
				TP3dep.WaitedPass = &TP3;
				TPDesc.WaitDescriptions.push_back(TP3dep);
			}
			DPE = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		//DPF
		{
			GFX_API::TransferPass_Description TPDesc;
			TPDesc.TP_TYPE = GFX_API::TRANFERPASS_TYPE::TP_BARRIER;
			TPDesc.NAME = "DPF";
			{
				GFX_API::PassWait_Description DPEdep;
				DPEdep.WaitLastFramesPass = false;
				DPEdep.WaitedStage.TRANSFERCMD = true;
				DPEdep.WaitedPass = &DPE;
				TPDesc.WaitDescriptions.push_back(DPEdep);
			}
			DPF = GFXRENDERER->Create_TransferPass(TPDesc);
		}
		GFXRENDERER->Finish_RenderGraphConstruction();
	}
}
