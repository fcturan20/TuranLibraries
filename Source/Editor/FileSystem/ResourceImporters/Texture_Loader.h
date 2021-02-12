#pragma once
#include "Editor/Editor_Includes.h"
#include "GFX/IMGUI/IMGUI_WINDOW.h"
#include "Editor/FileSystem/ResourceTypes/Resource_Identifier.h"

namespace TuranEditor {
	class Texture_Loader {
	public:
		static TAPIResult Import_Texture(const char* PATH,
			GFX_API::Texture_Description& desc, void*& DATA, const bool& flip_vertically = false);
	};
}