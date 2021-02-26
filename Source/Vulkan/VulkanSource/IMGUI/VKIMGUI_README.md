# How to Use
dear IMGUI uses a default window, so you need to create at least one before calling GFXRENDERER->Finish_RenderGraphConstruction() (First created window will be the default one). Also dear IMGUI uses a Vulkan RenderPass and Subpass to render in. So you have to specify which SubDrawPass you will use to render in GFXRENDERER->Finish_RenderGraphConstruction() call, but the DrawPass' RTSlotSet should only have one color attachment. You can call IMGUI functions after GFXRENDERER->Finish_RenderGraphConstruction(). 

# What to do if IMGUI is not used
In GFXRENDERER->Finish_RenderGraphConstruction() call, you can specify nullptr as IMGUI_SubpassHandle. You shouldn't call any of the IMGUI functions.