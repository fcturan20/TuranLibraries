#define GFXAPI

//This type of handles are unique RAM data pointers (to backend-specific objects)
//So don't serialize handles!
#define TGFXOBJHANDLE(object) typedef struct object##_TGFXOBJECT_DONTUSE* tgfx_##object;
//This type of handles aren't unique but invalid at outside of the functional scope
//Backends can implement each handle in two ways:
// 1) void* is enough to store all of the information, so just fill it
// 2) void* isn't enough, so create a list (list's lifetime etc is also backend-specific) then point to the element
//x86 platform may cause problems, so all datahandles are assumed to have limited lifetime.
//So please read the documentation of your data handle carefully, invalid access because of lifetimes may cause problems.
//Serializing data handles isn't recommended, because different version of the backend may change it so application may break.
//Functional scope: Some period between 2 function calls like StartRenderGraphConstruction-FinishRenderGraphConstruction or RENDERER->Run() - RENDERER->Run().
#define TGFXDATAHANDLE(object) typedef struct object##_TGFXDATA_DONTUSE* tgfx_##object;
#define TGFXHANDLELIST(HandleType) typedef tgfx_##HandleType* tgfx_##HandleType##_list;

//While coding GFX, we don't know the name of the handles, so we need to convert them according to the namespace preprocessor
#define TGFXLISTCOUNT(gfxcoreptr, listobject, countername) unsigned int countername = 0;  while (listobject[countername] != gfxcoreptr->INVALIDHANDLE) { countername++; }