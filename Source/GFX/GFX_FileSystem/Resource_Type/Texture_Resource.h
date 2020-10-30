#pragma once
#include "GFX/GFX_Includes.h"

namespace GFX_API {
	struct GFXAPI Texture_Properties {
		TEXTURE_DIMENSIONs DIMENSION = TEXTURE_DIMENSIONs::TEXTURE_2D;
		TEXTURE_MIPMAPFILTER MIPMAP_FILTERING = TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP;
		TEXTURE_WRAPPING WRAPPING = TEXTURE_WRAPPING::API_TEXTURE_REPEAT;
		TEXTURE_CHANNELs CHANNEL_TYPE = TEXTURE_CHANNELs::API_TEXTURE_RGB8UB;
		Texture_Properties();
		Texture_Properties(TEXTURE_DIMENSIONs dimension, TEXTURE_MIPMAPFILTER mipmap_filtering = TEXTURE_MIPMAPFILTER::API_TEXTURE_LINEAR_FROM_1MIP,
			TEXTURE_WRAPPING wrapping = TEXTURE_WRAPPING::API_TEXTURE_REPEAT, TEXTURE_CHANNELs channel_type = TEXTURE_CHANNELs::API_TEXTURE_RGB8UB);
	};

	struct GFXAPI TEXTUREUSAGEFLAG {
		//bool hasMipMaps			: 1;	//I don't support it for now!
		bool isCopiableFrom			: 1;	//If it is true, other textures or buffers are able to copy some from this texture
		bool isCopiableTo			: 1;	//If it is true, this texture may copy data from other buffers or textures
		bool isRenderableTo			: 1;	//If it is true, it is a Render Target for at least one DrawPass
		bool isSampledReadOnly		: 1;	//If it is true, it is accessed as a uniform texture that you're not able to write to it in the shader
		bool isRandomlyWrittenTo	: 1;	//If it is true, compute and draw pipeline shaders are able to write to it (Render Target isn't considered here)
		TEXTUREUSAGEFLAG();
	};

	/*
		Texture Resource Specifications:
			1) You can use textures to just read on GPU (probably for object material rendering), read-write on GPU (using compute shader to write an image), render target (framebuffer attachment)
			2) Modern APIs let you use a texture in anyway, it's left for your care. 
			But OpenGL doesn't let this situation, you are defining your texture's type in creation proccess
			So for now, GFXAPI uses Modern APIs' way and you can use a texture in anyway you want
			But you should use UsageBarrier for this kind of usage and specify it in RenderGraph
			3) You can't use Staging Buffers to store and access textures, they are just for transfer operations
	*/
	struct GFXAPI Texture_Description {
	public:
		Texture_Properties Properties;
		unsigned int WIDTH, HEIGHT;
		TEXTUREUSAGEFLAG USAGE;
	};
}

