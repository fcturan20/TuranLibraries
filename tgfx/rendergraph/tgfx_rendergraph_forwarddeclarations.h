

// RenderGraph types

typedef struct tgfx_rendergraph_drawpass_obj*        drawpass_tgfx_handle;
typedef struct tgfx_rendergraph_computepass_obj*     computepass_tgfx_handle;
typedef struct tgfx_rendergraph_transferpass_obj*    transferpass_tgfx_handle;
typedef struct tgfx_rendergraph_windowpass_obj*      windowpass_tgfx_handle;
typedef struct tgfx_rendergraph_subdrawpass_obj*     subdrawpass_tgfx_handle;
typedef struct tgfx_rendergraph_subcomputepass_obj*  subcomputepass_tgfx_handle;
typedef struct tgfx_rendergraph_subtransferpass_obj* subtransferpass_tgfx_handle;

typedef struct tgfx_waitsignaldescription_data*      waitsignaldescription_tgfx_handle;
typedef struct tgfx_passwaitdescription_data*        passwaitdescription_tgfx_handle;
typedef struct tgfx_subdrawpassdescription_data*     subdrawpassdescription_tgfx_handle;
typedef struct tgfx_subcomputepassdescription_data*  subcomputepassdescription_tgfx_handle;
typedef struct tgfx_subtransferpassdescription_data* subtransferpassdescription_tgfx_handle;

typedef waitsignaldescription_tgfx_handle* waitsignaldescription_tgfx_listhandle;
typedef passwaitdescription_tgfx_handle*   passwaitdescription_tgfx_listhandle;

typedef subdrawpassdescription_tgfx_handle*     subdrawpassdescription_tgfx_listhandle;
typedef subcomputepassdescription_tgfx_handle*  subcomputepassdescription_tgfx_listhandle;
typedef subtransferpassdescription_tgfx_handle* subtransferpassdescription_tgfx_listhandle;

typedef subdrawpass_tgfx_handle*     subdrawpass_tgfx_listhandle;
typedef subcomputepass_tgfx_handle*  subcomputepass_tgfx_listhandle;
typedef subtransferpass_tgfx_handle* subtransferpass_tgfx_listhandle;