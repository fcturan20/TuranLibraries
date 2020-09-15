#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX_Resource.h"
#include "GFX_RenderCommands.h"
#include "GFX_RenderNodes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"

namespace GFX_API {
	class GFXAPI Renderer {
	protected:
		//RenderGraph States
		bool Record_RenderGraphCreation = false;

		//Renderer States
		float LINEWIDTH = 1;

		//Render Node
		vector<DrawPass> DrawPasses;

		Bitset DrawPassID_BITSET;
		unsigned int Create_DrawPassID();
		void Delete_DrawPassID(unsigned int ID);

		Bitset SubDrawPassID_BITSET;
		unsigned int Create_SubDrawPassID();
		void Delete_SubDrawPassID(unsigned int ID);
	public:
		friend class GFX_Core;

		Renderer();
		~Renderer();
		Renderer* SELF;


		//RenderGraph FUNCTIONs
		virtual void Start_RenderGraphCreation() = 0;
		virtual void Create_DrawPass(vector<SubDrawPass_Description>& SubDrawPasses, unsigned int RTSLOTSET_ID) = 0;
		virtual void Finish_RenderGraphCreation() = 0;

		void Register_DrawCall(DrawCall drawcall);

		/*
			In a GL specific GFX API (Vulkan, OpenGL4 etc), you should specify functions like Find_DrawPass_byID to access draw passes on other systems
			You shouldn't define here, because that would any user to access these structures which isn't safe.
		*/
	};
}