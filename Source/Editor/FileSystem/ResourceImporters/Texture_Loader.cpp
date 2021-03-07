#include "Texture_Loader.h"
#include "Editor/FileSystem/EditorFileSystem_Core.h"
//To show import status
#include "Editor/Editors/Status_Window.h"
//To send textures to GPU memory
#include "GFX/GFX_Core.h"
#include "Editor/FileSystem/ResourceTypes/ResourceTYPEs.h"

using namespace TuranAPI;

//To import textures from 3rd party data formats
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>

namespace TuranEditor {
	//Output Path should be Directory + Name like "C:/dev/Content/FirstTexture". Every Texture has .texturecont extension!
	TAPIResult Texture_Loader::Import_Texture(const char* PATH,
			GFX_API::Texture_Description& desc, void*& DATA, const bool& flip_vertically) {
		int WIDTH, HEIGHT, CHANNELs;
		stbi_set_flip_vertically_on_load(flip_vertically);
		DATA = stbi_load(PATH, &WIDTH, &HEIGHT, &CHANNELs, 4);
		if (!DATA) {
			LOG_ERROR_TAPI("Texture isn't loaded from source! Data is nullptr!");
			return TAPI_FAIL;
		}
		//If application arrives here, loading is successful!
		desc.CHANNEL_TYPE = GFX_API::TEXTURE_CHANNELs::API_TEXTURE_RGBA8UB;
		desc.WIDTH = WIDTH;
		desc.HEIGHT = HEIGHT;

		LOG_STATUS_TAPI("Texture is loaded successfully!");
		return TAPI_SUCCESS;
	}
}