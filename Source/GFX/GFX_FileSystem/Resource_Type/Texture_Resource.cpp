#include "Texture_Resource.h"
#include <flatbuffers/flatbuffers.h>

namespace GFX_API {
	TEXTUREUSAGEFLAG::TEXTUREUSAGEFLAG() {
		isCopiableFrom = false;
		isCopiableTo = false;
		isRandomlyWrittenTo = false;
		isRenderableTo = false;
		isSampledReadOnly = false;
	}


	Texture_Properties::Texture_Properties() {}
	Texture_Properties::Texture_Properties(TEXTURE_DIMENSIONs dimension, TEXTURE_MIPMAPFILTER mipmap_filtering, TEXTURE_WRAPPING wrapping, TEXTURE_CHANNELs channel_type)
		: DIMENSION(dimension), MIPMAP_FILTERING(mipmap_filtering), WRAPPING(wrapping), CHANNEL_TYPE(channel_type){}
}