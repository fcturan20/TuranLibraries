## TGFX

    An abstract GFX library. Library needs at least one supported graphics API (Vulkan, OpenGL etc.) on the active platform to compile and run. 
    
## Table of Contents
* [Acronyms](#acronyms)
* [Summary](#Summary)
* [Handles](#Handles)
* [VRAM Allocation and Management](#VRAM-Allocation-and-Management)
* [dear Imgui](#dear-Imgui)


## Acronyms

-   **GFX API**: Graphics API (Vulkan, OpenGL etc.)
-   **GFX Backend**: Sub-library that implements GFX functionality using **GFX API**
-   **dIMGUI**: dear Imgui by @ocornut
-   **RG**: RenderGraph
-   **TGFXCore**: TGFX libraries' main class
-   **Rasterization state**: States used by rasterization pipeline such as blending type, depth test function etc.


## Summary
-   TGFX works with a system called **RenderGraph**. You have to define your usage as a **RenderGraph** that's a graph of bunch of **RenderPass**s. You have to link the passs to each other and fill them with workload (GPU operations). Each **RenderPass** has a type depending on what it does (Transfer, Rasterize, Compute, Swapchain Display). [For more information about RenderPass creation and management.](RenderGraph_README.md)
-   GPU data management is handled with a system called **GPUDataManager**. It is responsible for creating, copying and deleting resources on GPU. [For more information about resources and data management, check here.](Resources_README.md)
-   TGFX doesn't manage any GPU memory itself, you have to manage GPU memory. [For more information about it](#VRAM-Allocation-and-Management)
-   If you want to use *dIMGUI*, **IMGUI Core** is the abstraction of it which GFX Backend implements. [For more information about IMGUI Core.](#dear-Imgui)
-   TGFX only returns and needs handles of datas. There are 2 types of handles; **Object Handle** and **Data Handle**. [For more information about them, check here](#Handles)
-   Window management is done in TGFXCore but window's swapchain texture management is harder and should be handled with RenderGraph (because synchronization is needed). [For more information about it, check here.](RenderGraph_README.md/#Window-Swapchain-Management)


## Handles
-   **Object Handle**: These type of datas should be created and deleted by the user, otherwise they're not deleted from VRAM/RAM except TGFX closes. GPUDataManager's DestroyXXX() functions should be called to destroy these resources.
-   **Data Handle**: These type of datas has a short lifetime. There is no DestroyXXX() functions for these resources, because they are deleted from VRAM/RAM by the TGFX when they exceed their lifetime.
-   Handles are C pointers, so both handle types are at size of a pointer of the compiled platform.
-   Handles can also be passed as a list. Instead of using an extra argument as item count, each list should end with an extra element equal to *Invalid Pointer*.
-   *Invalid Pointer* is created after first initialization stage of TGFX and can be accessed from TGFXCore's *INVALIDHANDLE* constant variable.


## VRAM Allocation and Management
-   Most GPUs' VRAM are designed as CPU can't access most of the VRAM (GFX APIs call it *Device Local*) and there is only a small VRAM region that's able to be accessed by CPU (they call it *Host Visible*).
-   New GPUs and GFX APIs support that a Device Local region can be Host Visible. This means CPU can directly access all of the VRAM. This is not supported by TGFX right now, [you can check the status here.](https://github.com/fcturan20/TuranLibraries/issues/1)
-   TGFX uses one memory allocation per memory region, so you should decide memory regions and their allocation size while initializing TGFX. You also have to obey rules like *buffer alignment*.
-   If you have complicated [*Data Resource Transfer*s](Resources_README.md/#Data-Resource-Transfers), you'll probably want to implement a memory management system.


## dear Imgui
-   dIMGUI's integrated into TGFX itself but its *GFX API* related files are integrated in the related *GFX backend*'s folder. 
-   dIMGUI needs a default window even on the docking branch, so this doesn't match 1:1 to TGFX's whole design.
-   If you use dIMGUI, you should create at least one window before finishing your first RG reconstruction and you should specify the *Draw Pass* and *Sub-Draw Pass* while [reconstructing RenderGraph](RenderGraph_README.md/#RG-Life-Cycle).
-   First created window will be the default one.
-   It's recommended to use a Draw Pass that has the RT SlotSet that has one color attachment and nothing else. Otherwise it may cause some problems depending on the GFX backend, GFX API and GPU driver.