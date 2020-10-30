#include "Vulkan_Renderer_Core.h"
#include "VK_GPUContentManager.h"
#include "Vulkan/VulkanSource/Renderer/Vulkan_Resource.h"
#include "TuranAPI/Profiler_Core.h"
#define VKContentManager ((Vulkan::GPU_ContentManager*)GFXContentManager)
#define VKGPU ((Vulkan::GPU*)GFX->GPU_TO_RENDER)
#define VKWINDOW ((Vulkan::WINDOW*)GFX->Main_Window)

namespace Vulkan {
	struct VK_GeneralPass {
		GFX_API::GFXHandle Handle;
		bool is_DrawPass;	//If it is true, this is a DrawPass. Otherwise, it is a TransferPass
		vector<GFX_API::PassWait_Description>* WAITs;
		string* PASSNAME;
	};

	struct VK_GeneralPassLinkedList {
		VK_GeneralPass* CurrentGP = nullptr;
		VK_GeneralPassLinkedList* NextElement = nullptr;
	};
	VK_GeneralPassLinkedList* Create_GPLinkedList() {
		return new VK_GeneralPassLinkedList;
	}
	void PushBack_ToGPLinkedList(VK_GeneralPass* GP, VK_GeneralPassLinkedList* GPLL) {
		//Go to the last element in the list!
		while (GPLL->NextElement) {
			GPLL = GPLL->NextElement;
		}
		//If element points to a GP, create new element
		if (GPLL->CurrentGP) {
			VK_GeneralPassLinkedList* newelement = Create_GPLinkedList();
			GPLL->NextElement = newelement;
			newelement->CurrentGP = GP;
		}
		//Otherwise, make element point to the GP
		else {
			GPLL->CurrentGP = GP;
		}
	}
	//Return the new header if the header changes!
	//If header isn't found, given header is returned
	//If header is the last element and should be deleted, then nullptr is returned
	//If header isn't the last element but should be deleted, then next element is returned
	VK_GeneralPassLinkedList* DeleteGP_FromGPLinkedList(VK_GeneralPassLinkedList* Header, VK_GeneralPass* ElementToDelete) {
		VK_GeneralPassLinkedList* CurrentCheckItem = Header;
		VK_GeneralPassLinkedList* LastChecked_Item = nullptr;
		while (CurrentCheckItem->CurrentGP != ElementToDelete) {
			if (!CurrentCheckItem->NextElement) {
				LOG_STATUS_TAPI("VulkanRenderer: DeleteGP_FromGPLinkedList() couldn't find the GP in the list!");
				return Header;
			}
			LastChecked_Item = CurrentCheckItem;
			CurrentCheckItem = CurrentCheckItem->NextElement;
		}
		if (!LastChecked_Item) {
			CurrentCheckItem->CurrentGP = nullptr;
			if (CurrentCheckItem->NextElement) {
				return CurrentCheckItem->NextElement;
			}
			return nullptr;
		}
		LastChecked_Item->NextElement = CurrentCheckItem->NextElement;
		CurrentCheckItem->CurrentGP = nullptr;
		return Header;
	}

	//This structure is to create RGBranches easily
	struct VK_RenderingPath {
		unsigned int ID;	//For now, I give each RenderingPath an ID to debug the algorithm
		//This passes are executed in the First-In-First-Executed order which means element 0 is executed first, then 1...
		vector<VK_GeneralPass*> ExecutionOrder;
		vector<VK_RenderingPath*> CFDependentRPs, LFDependentRPs, CFLaterExecutedRPs, NFExecutedRPs;
		VK_QUEUEFLAG QueueFeatureRequirements;
	};


	//Please use the QUEUEs directly from GPU class like (&VKGPU->QUEUE[0])! Be careful not to point to reference!
	bool IsQueueAvailable_forRP(const VK_QUEUE* QUEUE, const VK_RenderingPath* RP) {
		const VK_QUEUEFLAG& RPQ = RP->QueueFeatureRequirements;
		const VK_QUEUEFLAG& ARQ = QUEUE->SupportFlag;

		if (RPQ.is_COMPUTEsupported && !ARQ.is_COMPUTEsupported) {
			return false;
		}
		if (RPQ.is_GRAPHICSsupported && !ARQ.is_GRAPHICSsupported) {
			return false;
		}
		if (RPQ.is_TRANSFERsupported && !ARQ.is_TRANSFERsupported) {
			return false;
		}
		return true;
	}
	//If anything (PRESENTATION is ignored) is false, that means this pass doesn't require its existence
	//If everything (PRESENTATION is ignored) is false, that means every queue type can call this pass
	VK_QUEUEFLAG FindRequiredQueue_ofGP(const VK_GeneralPass* GP) {
		VK_QUEUEFLAG flag;
		flag.is_COMPUTEsupported = false;
		flag.is_GRAPHICSsupported = false;
		flag.is_TRANSFERsupported = false;
		if (GP->is_DrawPass) {
			flag.is_GRAPHICSsupported = true;
		}
		else {
			VK_TransferPass* TP = GFXHandleConverter(VK_TransferPass*, GP->Handle);
			if (TP->TYPE == GFX_API::TRANFERPASS_TYPE::TP_BARRIER) {
				//All is false means any queue can call this pass!
			}
			else {
				//All of UPLOAD, COPY and DOWNLOAD TPs needs transfer commands!
				flag.is_TRANSFERsupported = true;
			}
		}
		return flag;
	}

	unsigned int Generate_RPID() {
		static unsigned int Next_RPID = 0;
		Next_RPID++;
		return Next_RPID;
	}

	void PrintRPSpecs(VK_RenderingPath* RP) {
		std::cout << ("RP ID: " + to_string(RP->ID)) << std::endl;
		for (unsigned int GPIndex = 0; GPIndex < RP->ExecutionOrder.size(); GPIndex++) {
			std::cout << ("Execution Order: " + to_string(GPIndex) + " and Pass Name: " + *RP->ExecutionOrder[GPIndex]->PASSNAME) << std::endl;
		}
		for (unsigned int CFDependentRPIndex = 0; CFDependentRPIndex < RP->CFDependentRPs.size(); CFDependentRPIndex++) {
			std::cout << ("Current Frame Dependent RP ID: " + to_string(RP->CFDependentRPs[CFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int LFDependentRPIndex = 0; LFDependentRPIndex < RP->LFDependentRPs.size(); LFDependentRPIndex++) {
			std::cout << ("Last Frame Dependent RP ID: " + to_string(RP->LFDependentRPs[LFDependentRPIndex]->ID)) << std::endl;
		}
		for (unsigned int CFLaterExecuteRPIndex = 0; CFLaterExecuteRPIndex < RP->CFLaterExecutedRPs.size(); CFLaterExecuteRPIndex++) {
			std::cout << ("Current Frame Later Executed RP ID: " + to_string(RP->CFLaterExecutedRPs[CFLaterExecuteRPIndex]->ID)) << std::endl;
		}
		for (unsigned int LFLaterExecuteRPIndex = 0; LFLaterExecuteRPIndex < RP->NFExecutedRPs.size(); LFLaterExecuteRPIndex++) {
			std::cout << ("Next Frame Executed RP ID: " + to_string(RP->NFExecutedRPs[LFLaterExecuteRPIndex]->ID)) << std::endl;
		}
	}


	//Check if Pass is already in a RP, if it is not then Create New RP
	//Why there is such a check? Because algorithm
	//Create New RP: Set dependent RPs of the new RP as DependentRPs and give newRP to them as LaterExecutedRPs element
	VK_RenderingPath* Create_NewRP(VK_GeneralPass* Pass, vector<VK_RenderingPath*>& RPs) {
		VK_RenderingPath* RP;
		//Check if the pass is in a RP
		for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
			VK_RenderingPath* RP_Check = RPs[RPIndex];
			for (unsigned int RPPassIndex = 0; RPPassIndex < RP_Check->ExecutionOrder.size(); RPPassIndex++) {
				VK_GeneralPass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
				if (GP_Check->Handle == Pass->Handle) {
					return nullptr;
				}
			}
		}
		RP = new VK_RenderingPath;
		//Create dependencies for all
		for (unsigned int PassWaitIndex = 0; PassWaitIndex < Pass->WAITs->size(); PassWaitIndex++) {
			GFX_API::PassWait_Description& Wait_desc = (*Pass->WAITs)[PassWaitIndex];
			for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
				VK_RenderingPath* RP_Check = RPs[RPIndex];
				bool is_alreadydependent = false;
				for (unsigned int DependentRPIndex = 0; DependentRPIndex < RP->CFDependentRPs.size() && !is_alreadydependent; DependentRPIndex++) {
					if (RP_Check == RP->CFDependentRPs[DependentRPIndex]) {
						is_alreadydependent = true;
						break;
					}
				}
				if (is_alreadydependent) {
					continue;
				}
				for (unsigned int RPPassIndex = 0; RPPassIndex < RP_Check->ExecutionOrder.size(); RPPassIndex++) {
					VK_GeneralPass* GP_Check = RP_Check->ExecutionOrder[RPPassIndex];
					if (!Wait_desc.WaitLastFramesPass &&
						*Wait_desc.WaitedPass == GP_Check->Handle) {
						RP->CFDependentRPs.push_back(RP_Check);
						RP_Check->CFLaterExecutedRPs.push_back(RP);
					}
				}
			}
		}
		RP->ExecutionOrder.clear();
		RP->ExecutionOrder.push_back(Pass);
		RP->ID = Generate_RPID();
		RP->QueueFeatureRequirements = FindRequiredQueue_ofGP(Pass);

		RPs.push_back(RP);
		return RP;
	}
	void AttachGPTo_RPBack(VK_GeneralPass* Pass, VK_RenderingPath* RP) {
		RP->ExecutionOrder.push_back(Pass);
		VK_QUEUEFLAG FLAG = FindRequiredQueue_ofGP(Pass);
		RP->QueueFeatureRequirements.is_COMPUTEsupported |= FLAG.is_COMPUTEsupported;
		RP->QueueFeatureRequirements.is_GRAPHICSsupported |= FLAG.is_GRAPHICSsupported;
		RP->QueueFeatureRequirements.is_TRANSFERsupported |= FLAG.is_TRANSFERsupported;
	}

	bool is_AnyWaitPass_Common(VK_GeneralPass* gp1, VK_GeneralPass* gp2) {
		for (unsigned int gp1waitIndex = 0; gp1waitIndex < gp1->WAITs->size(); gp1waitIndex++) {
			const GFX_API::PassWait_Description& gp1Wait = (*gp1->WAITs)[gp1waitIndex];
			for (unsigned int gp2waitIndex = 0; gp2waitIndex < gp2->WAITs->size(); gp2waitIndex++) {
				const GFX_API::PassWait_Description& gp2Wait = (*gp2->WAITs)[gp2waitIndex];
				if (*gp1Wait.WaitedPass == *gp2Wait.WaitedPass &&
					!gp1Wait.WaitLastFramesPass && !gp2Wait.WaitLastFramesPass
					//gp1Wait.WaitLastFramesPass == gp2Wait.WaitLastFramesPass
					)
				{
					return true;
				}
			}
		}
		return false;
	}

	//Return created LastFrameDependentRPs in LFD_RPs argument
	VK_GeneralPassLinkedList* Create_LastFrameDependentRPs(VK_GeneralPassLinkedList* ListHeader, vector<VK_RenderingPath*>& RPs, vector<VK_RenderingPath*>& LFD_RPs) {
		//LFD_RPs.clear();
		VK_GeneralPassLinkedList* CurrentCheck = ListHeader;
		while (CurrentCheck) {
			VK_GeneralPass* GP = CurrentCheck->CurrentGP;
			bool is_RPCreated = false;
			for (unsigned int WaitIndex = 0; WaitIndex < GP->WAITs->size() && !is_RPCreated; WaitIndex++) {
				GFX_API::PassWait_Description& Waitdesc = (*GP->WAITs)[WaitIndex];
				if (Waitdesc.WaitLastFramesPass) {
					LFD_RPs.push_back(Create_NewRP(GP, RPs));
					is_RPCreated = true;
					ListHeader = DeleteGP_FromGPLinkedList(ListHeader, GP);
					break;
				}
			}

			CurrentCheck = CurrentCheck->NextElement;
		}
		return ListHeader;
	}

	void Link_LastFrameDependentRPs(vector<VK_RenderingPath*>& LFD_RPs, vector<VK_RenderingPath*>& RPs) {
		for (unsigned int LFDRPIndex = 0; LFDRPIndex < LFD_RPs.size(); LFDRPIndex++) {
			VK_RenderingPath* LFDRP = LFD_RPs[LFDRPIndex];
			vector<GFX_API::PassWait_Description>* WAITs = LFDRP->ExecutionOrder[0]->WAITs;
			for (unsigned int WaitIndex = 0; WaitIndex < WAITs->size(); WaitIndex++) {
				GFX_API::PassWait_Description& Wait = (*WAITs)[WaitIndex];
				for (unsigned int MainListIndex = 0; MainListIndex < RPs.size(); MainListIndex++) {
					VK_RenderingPath* MainRP = RPs[MainListIndex];
					if (LFDRP == MainRP) {
						continue;
					}
					if (*Wait.WaitedPass == MainRP->ExecutionOrder[MainRP->ExecutionOrder.size() - 1]->Handle) {
						if (Wait.WaitLastFramesPass) {
							LFDRP->LFDependentRPs.push_back(MainRP);
							MainRP->NFExecutedRPs.push_back(LFDRP);
						}
						else {
							LFDRP->CFDependentRPs.push_back(MainRP);
							MainRP->CFLaterExecutedRPs.push_back(LFDRP);
						}
					}
				}
			}
		}
	}


	//This is a Recursive Function that calls itself to find or create a Rendering Path for each of the RPless_passes
	void FindPasses_dependentRPs(VK_GeneralPassLinkedList* RPless_passes, vector<VK_RenderingPath*>& RPs) {
		if (!RPless_passes->CurrentGP) {
			return;
		}
		//These are the passes that all of their wait passes are in RPs, so store them to create new RPs
		VK_GeneralPassLinkedList* AttachableGPs_ListHeader = Create_GPLinkedList();

		VK_GeneralPassLinkedList* MainLoop_Element = RPless_passes;
		while (MainLoop_Element) {
			VK_GeneralPass* GP_Find = MainLoop_Element->CurrentGP;
			{
				bool DoesAnyWaitPass_RPless = false;
				bool should_addinnextiteration = false;
				for (unsigned int WaitIndex = 0; WaitIndex < GP_Find->WAITs->size() && !DoesAnyWaitPass_RPless; WaitIndex++) {
					GFX_API::PassWait_Description& Wait_Desc = (*GP_Find->WAITs)[WaitIndex];

					VK_GeneralPassLinkedList* WaitLoop_Element = RPless_passes;
					while (WaitLoop_Element && !DoesAnyWaitPass_RPless) {
						VK_GeneralPass* GP_WaitCheck = WaitLoop_Element->CurrentGP;
						if (WaitLoop_Element == MainLoop_Element) {
							//Here is empty because we don't to do anything if wait and main loop elements are the same, please don't change the code!
						}
						else 
						if (*Wait_Desc.WaitedPass == GP_WaitCheck->Handle
							&& !Wait_Desc.WaitLastFramesPass
							) 
						{
							DoesAnyWaitPass_RPless = true;
						}

						WaitLoop_Element = WaitLoop_Element->NextElement;
					}

					VK_GeneralPassLinkedList* AttachableGPs_CheckLoopElement = AttachableGPs_ListHeader;
					while (AttachableGPs_CheckLoopElement && !should_addinnextiteration && !DoesAnyWaitPass_RPless) {
						if (AttachableGPs_CheckLoopElement->CurrentGP) {
							VK_GeneralPass* AttachableGP_WaitCheck = AttachableGPs_CheckLoopElement->CurrentGP;
							if (*Wait_Desc.WaitedPass == AttachableGP_WaitCheck->Handle
								&& !Wait_Desc.WaitLastFramesPass
								) {
								DoesAnyWaitPass_RPless = true;
								should_addinnextiteration = true;
							}
						}

						AttachableGPs_CheckLoopElement = AttachableGPs_CheckLoopElement->NextElement;
					}
				}

				//That means all of the wait passes are already in RPs, so just store it to create or directly attach to a RP
				if (!DoesAnyWaitPass_RPless && !should_addinnextiteration) {
					PushBack_ToGPLinkedList(GP_Find, AttachableGPs_ListHeader);	//We don't care storing the last element because AttachableGPs_CheckLoop should check all the elements
					RPless_passes = DeleteGP_FromGPLinkedList(RPless_passes, GP_Find);
				}
			}

			MainLoop_Element = MainLoop_Element->NextElement;
		}

		//Attach GPs to RPs or Create RPs for GPs
		VK_GeneralPassLinkedList* AttachableGP_MainLoopElement = AttachableGPs_ListHeader;
		while (AttachableGP_MainLoopElement) {
			VK_GeneralPass* GP_Find = AttachableGP_MainLoopElement->CurrentGP;
			bool doeshave_anycommonWaitPass = false;
			VK_GeneralPassLinkedList* CommonlyWaited_Passes = Create_GPLinkedList();
			VK_GeneralPassLinkedList* AttachableGP_CheckLoopElement = AttachableGPs_ListHeader;
			while (AttachableGP_CheckLoopElement) {
				VK_GeneralPass* GP_Check = AttachableGP_CheckLoopElement->CurrentGP;
				if (AttachableGP_CheckLoopElement == AttachableGP_MainLoopElement) {

				}
				else if (is_AnyWaitPass_Common(GP_Find, GP_Check)) {
					doeshave_anycommonWaitPass = true;
					PushBack_ToGPLinkedList(GP_Check, CommonlyWaited_Passes);
				}

				AttachableGP_CheckLoopElement = AttachableGP_CheckLoopElement->NextElement;
			}
			if (doeshave_anycommonWaitPass) {
				PushBack_ToGPLinkedList(GP_Find, CommonlyWaited_Passes);
				VK_GeneralPassLinkedList* CommonlyWaitedPasses_CreateElement = CommonlyWaited_Passes;
				while (CommonlyWaitedPasses_CreateElement->NextElement) {
					Create_NewRP(CommonlyWaitedPasses_CreateElement->CurrentGP, RPs);
					CommonlyWaitedPasses_CreateElement = CommonlyWaitedPasses_CreateElement->NextElement;
				}
			}
			else {
				bool break_allloops = false;
				bool shouldattach_toaRP = false;
				VK_RenderingPath* Attached_RP = nullptr;
				for (unsigned int WaitPassIndex = 0; WaitPassIndex < GP_Find->WAITs->size() && !break_allloops; WaitPassIndex++) {
					GFX_API::PassWait_Description& Waitdesc = (*GP_Find->WAITs)[WaitPassIndex];
					
					if (Waitdesc.WaitLastFramesPass) {
						continue;
					}
					for (unsigned int CheckedRPIndex = 0; CheckedRPIndex < RPs.size() && !break_allloops; CheckedRPIndex++) {
						VK_RenderingPath* CheckedRP = RPs[CheckedRPIndex];
						for (unsigned int CheckedPassIndex = 0; CheckedPassIndex < CheckedRP->ExecutionOrder.size() && !break_allloops; CheckedPassIndex++) {
							VK_GeneralPass* CheckedExecutionGP = CheckedRP->ExecutionOrder[CheckedPassIndex];
							if (*Waitdesc.WaitedPass == CheckedExecutionGP->Handle) {
								if (shouldattach_toaRP) {
									if (Attached_RP != CheckedRP) {
										Create_NewRP(GP_Find, RPs);
										break_allloops = true;
										continue;	//You may ask why there is a continue instead of break, because we set break_allloops to true so it will
									}
								}
								else {
									shouldattach_toaRP = true;
									Attached_RP = CheckedRP;
								}
							}
						}
					}
				}
				//All loops before isn't broken because there is only one RP that GP should be attached
				if (!break_allloops & shouldattach_toaRP) {
					AttachGPTo_RPBack(GP_Find, Attached_RP);
				}
				else if (!shouldattach_toaRP) {
					std::cout << ("Didn't attached to an RP and deleted from RPless list!") << std::endl;
				}
			}


			AttachableGP_MainLoopElement = AttachableGP_MainLoopElement->NextElement;
		}

		if (!RPless_passes) {
			return;
		}
		if (RPless_passes->NextElement || RPless_passes->CurrentGP) {
			FindPasses_dependentRPs(RPless_passes, RPs);
		}
	}

	void Create_VkFrameBuffers(VK_BranchPass* DrawPassInstance, unsigned int FrameGraphIndex) {
		if (DrawPassInstance->is_TransferPass) {
			return;
		}
		VK_DrawPassInstance* RenderPassInstance = GFXHandleConverter(VK_DrawPassInstance*, DrawPassInstance->Handle);

		LOG_NOTCODED_TAPI("VulkanRenderer: Create_DrawPassVkFrameBuffers() has failed because Color Slot creation should support back-buffered RT SLOTs! More information in FGAlgorithm.cpp line 410.", true);
		/*
		VkFramebuffer generation has failed now because users should be able to use different RTs for different frames
		For example: Swapchain's one image should be used for FrameIndex = 0, other image should be used for FrameIndex = 1
		This information should be passed at SlotSet creation process
		*/
		vector<VkImageView> Attachments;
		for (unsigned int i = 0; i < RenderPassInstance->BasePass->SLOTSET->COLORSLOTs_COUNT; i++) {
			VK_Texture* VKTexture = RenderPassInstance->BasePass->SLOTSET->COLOR_SLOTs[i].RT;
			Attachments.push_back(VKTexture->ImageView);
		}
		if (RenderPassInstance->BasePass->SLOTSET->DEPTHSTENCIL_SLOT) {
			Attachments.push_back(RenderPassInstance->BasePass->SLOTSET->DEPTHSTENCIL_SLOT->RT->ImageView);
		}
		VkFramebufferCreateInfo FrameBuffer_ci = {};
		FrameBuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FrameBuffer_ci.renderPass = RenderPassInstance->BasePass->RenderPassObject;
		FrameBuffer_ci.attachmentCount = Attachments.size();
		FrameBuffer_ci.pAttachments = Attachments.data();
		FrameBuffer_ci.width = RenderPassInstance->BasePass->SLOTSET->COLOR_SLOTs[0].RT->WIDTH;
		FrameBuffer_ci.height = RenderPassInstance->BasePass->SLOTSET->COLOR_SLOTs[0].RT->HEIGHT;
		FrameBuffer_ci.layers = 1;
		FrameBuffer_ci.pNext = nullptr;
		FrameBuffer_ci.flags = 0;

		if (vkCreateFramebuffer(VKGPU->Logical_Device, &FrameBuffer_ci, nullptr, &RenderPassInstance->Framebuffer) != VK_SUCCESS) {
			LOG_CRASHING_TAPI("Renderer::Create_DrawPassFrameBuffers() has failed!");
			return;
		}
	}
	void Create_FrameGraphs(const vector<VK_DrawPass*>& DrawPasses, const vector<VK_TransferPass*>& TransferPasses, VK_FrameGraph* FrameGraphs) {
		vector<VK_GeneralPass> AllPasses;
		vector<VK_RenderingPath*> RPs;
		{
			TURAN_PROFILE_SCOPE("RenderGraph Construction Algorithm");

			//Passes in this vector means
			VK_GeneralPassLinkedList* GPHeader = Create_GPLinkedList();

			//Fill both AllPasses and RPless_passes vectors!
			//Also create root Passes
			for (unsigned int DPIndex = 0; DPIndex < DrawPasses.size(); DPIndex++) {
				VK_DrawPass* DP = DrawPasses[DPIndex];
				VK_GeneralPass GP;
				GP.is_DrawPass = true;
				GP.Handle = DP;
				GP.WAITs = &DP->WAITs;
				GP.PASSNAME = &DP->NAME;
				AllPasses.push_back(GP);
			}
			for (unsigned int DPIndex = 0; DPIndex < DrawPasses.size(); DPIndex++) {
				VK_DrawPass* DP = DrawPasses[DPIndex];
				//Check if it is root DP and if it is not, then pass it to the RPless_passes
				bool is_RootDP = false;
				if (DP->WAITs.size() == 0) {
					is_RootDP = true;
				}
				for (unsigned int WaitIndex = 0; WaitIndex < DP->WAITs.size(); WaitIndex++) {
					if (!DP->WAITs[WaitIndex].WaitLastFramesPass) {
						is_RootDP = false;
					}
				}
				if (is_RootDP) {
					Create_NewRP(&AllPasses[DPIndex], RPs);
				}
				else {
					PushBack_ToGPLinkedList(&AllPasses[DPIndex], GPHeader);
				}
			}
			for (unsigned int TPIndex = 0; TPIndex < TransferPasses.size(); TPIndex++) {
				VK_TransferPass* TP = TransferPasses[TPIndex];
				VK_GeneralPass GP;
				GP.is_DrawPass = false;
				GP.Handle = TP;
				GP.WAITs = &TP->WAITs;
				GP.PASSNAME = &TP->NAME;
				AllPasses.push_back(GP);
			}
			for (unsigned int TPIndex = 0; TPIndex < TransferPasses.size(); TPIndex++) {
				VK_TransferPass* TP = TransferPasses[TPIndex];
				//Check if it is root TP and if it is not, then pass it to the RPless_passes
				bool is_RootTP = false;
				if (TP->WAITs.size() == 0) {
					is_RootTP = true;
				}
				for (unsigned int WaitIndex = 0; WaitIndex < TP->WAITs.size(); WaitIndex++) {
					if (!TP->WAITs[WaitIndex].WaitLastFramesPass) {
						is_RootTP = false;
					}
				}
				if (is_RootTP) {
					Create_NewRP(&AllPasses[TPIndex + DrawPasses.size()], RPs);
				}
				else {
					PushBack_ToGPLinkedList(&AllPasses[TPIndex + DrawPasses.size()], GPHeader);
				}
			}

			vector<VK_RenderingPath*> LastFrameDependentRPs;
			GPHeader = Create_LastFrameDependentRPs(GPHeader, RPs, LastFrameDependentRPs);
			FindPasses_dependentRPs(GPHeader, RPs);
			Link_LastFrameDependentRPs(LastFrameDependentRPs, RPs);

			for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
				VK_RenderingPath* RP = RPs[RPIndex];
				bool DoesAnyQueueSupport_theRP = false;
				for (unsigned int QUEUEIndex = 0; QUEUEIndex < VKGPU->QUEUEs.size(); QUEUEIndex++) {
					if (IsQueueAvailable_forRP(&VKGPU->QUEUEs[QUEUEIndex], RP)) {
						DoesAnyQueueSupport_theRP = true;
						//std::cout << "Your Queue Family Index: " << to_string(VKGPU->QUEUEs[QUEUEIndex].QueueFamilyIndex) << " and supported RP ID : " << RP->ID << std::endl;
					}
				}
				if (!DoesAnyQueueSupport_theRP) {
					LOG_NOTCODED_TAPI("VulkanRenderer: GFXRenderer->Finish_RenderGraphCreation() has failed because one of the Rendering Paths isn't supported by any Vulkan Queue that your GPU supports! GFX API should be able to split Rendering Paths to different smaller paths but not supported for now!", true);
					return;
				}
			}
		}


		if (RPs.size()) {
			LOG_STATUS_TAPI("Started to print RP specs!");
			for (unsigned int RPIndex = 0; RPIndex < RPs.size(); RPIndex++) {
				std::cout << "Next RP!\n";
				PrintRPSpecs(RPs[RPIndex]);
			}

		}
		else {
			LOG_STATUS_TAPI("There is no RP to print!");
		}


		//Create RBs
		for (unsigned int FGIndex = 0; FGIndex < 2; FGIndex++) {
			FrameGraphs[FGIndex].BranchCount = RPs.size();
			FrameGraphs[FGIndex].FrameGraphTree = new VK_RGBranch[RPs.size()];

			for (unsigned int RBIndex = 0; RBIndex < FrameGraphs[FGIndex].BranchCount; RBIndex++) {
				VK_RGBranch& Branch = FrameGraphs[FGIndex].FrameGraphTree[RBIndex];
				VK_RenderingPath* RP = RPs[RBIndex];
				
				VK_BranchPass* &PassLL_Element = Branch.Passes;
				for (unsigned int CreatedPassCount = 0; CreatedPassCount < RP->ExecutionOrder.size(); CreatedPassCount++) {
					VK_GeneralPass* GP = RP->ExecutionOrder[CreatedPassCount];
					PassLL_Element = new VK_BranchPass;

					if (GP->is_DrawPass) {
						VK_DrawPassInstance* DPInstance = new VK_DrawPassInstance;
						DPInstance->BasePass = GFXHandleConverter(VK_DrawPass*, RP->ExecutionOrder[0]->Handle);

						PassLL_Element->is_TransferPass = false;
						PassLL_Element->Handle = DPInstance;
						Create_VkFrameBuffers(PassLL_Element);
					}
					else {
						PassLL_Element->is_TransferPass = true;
						PassLL_Element->Handle = GP->Handle;	//Which means Handle is VK_TransferPass*
					}

					PassLL_Element = PassLL_Element->NextItem;	//This means nullptr, but each loop will create new Element automatically as can be seen above!
				}
			}
		}
	}
}