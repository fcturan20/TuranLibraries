# RenderGraph

RenderGraph is the way to define your GPU side operations. Modern GFX APIs needs proper synchronization between GPU-CPU and RenderGraph is the best system to define them. Every GPU operation (even resource creation, upload etc) needs to start running at the scheduled time for best performance and minimal bug. To achieve that, this system uses *RenderPass*es to abstract GPU operation types and define execution dependencies between them.

TODO DOC: This doc is very old, fix it.

# Table of Contents
* [Acronyms](#Acronyms)
* [Summary](#Summary)
* [Draw Pass](#Draw-Pass)
* [Compute Pass](#Compute-Pass)
* [Transfer Pass](#Transfer-Pass)
* [Window Pass](#Window-Pass)
* [RenderGraph Life-Cycle](#RG-Life-Cycle)
* [Static RenderGraph Optimizer](#Static-RG-Optimizer)
* [Dynamic RenderGraph Optimizer](#Dynamic-RG-Optimizer)
* [Recording Command Buffers]()
* [Window Swapchain Management](#Window-Swapchain-Management)

# Acronyms

-   **RG**: RenderGraph
-   **RP**: RenderPass
-   **workload**: Any type of operations that the *Pass* can run.



# Summary

-   As *RenderGraph* defined with nodes called *RenderPass*es, RG system needs to optimize the graph to find the best control flow. 
-   Optimizing RG is done in 2 steps; [*Static RG Optimizer*](#Static-RG-Optimizer) and [*Dynamic RG Optimizer*](#Dynamic-RG-Optimizer).
-   RG can go through only 3 phases in a given frame: *RG Destruction*, *RG Reconstruction* and *RG Execution*. [For more information, check RenderGraph Life-Cycle here.](#RG-Life-Cycle)
-   There are 4 types of *RenderPass*es; [*Draw Pass*](#Draw-Pass), [*Compute Pass*](#Compute-Pass), [*Transfer Pass*](#Transfer-Pass) and [*Window Pass*](#Window-Pass).

# Draw Pass
-   You can use only rasterization pipeline in this *Pass*.
-   A Draw Pass is a collection of SubDrawPasses and there should be at least one SubDrawPass to create a Draw Pass. 
-   SubDrawPass is the same concept with Vulkan's SubPasses, but I believe mine's interface is better :)
-   In a SubDrawPass, each draw call is executed asynchronously. If you want a sync, you should create a new SubDrawPass for it. 
-   SubDrawPasses that don't have operations are culled according to the Node Culling process.
-   You can define how you will draw your vertex buffer in Draw Call (Indexed, Shader Instance, Point-Line-Triangle etc). This allows GFX API to sort the draw calls for minimum state change.
-   DrawPass draws vertex buffers to texture(s), which are called Render Target. But DrawPass needs a concept called RTSlotSet, which is described in GFX/Renderer/tgfx_gpudatamanager.


# Compute Pass
-   You can only use compute pipeline in this *Pass*.
-   I didn't have any asynchronous compute capable hardware to test the RenderGraph, so I was more in to other parts. Now I have a GTX1650, so I will code Compute Pass as well.


# Transfer Pass
-   This is where data transfers happen.
-   You have 2 choices; CPU<->GPU or GPU<->GPU Transfer Pass (Copy TP) and GPU Operation ||| GPU Operation Transfer Pass (Barrier TP)
-   TP is typeful. Because it's more data transfers close to each other in API, it's harder to debugging. Also GPU pipelines may differ between types of data transfers, which this seperation may help in future!
-   Upload TPs use staging buffer for temporary data. When you call Upload_**Buffer(), the data is uploaded to the staging buffer, then is copied to the permanent VRAM location.
-   For now, Staging buffers are only for data transfers. I want to allow user to divide the buffer for datas that is frequently changed.
-   Download TP is complicated concept which I didn't decide how to implement because of multi-threading (CPU job system) and dynamic GPU operations.
-   Barrier TP is useful for resource layout changes (image or buffer). For example, you need to change textures' layouts before using them first time. 


# Window Pass
-   This is where display functions are used.
-   This is a Pass, because users may want to refresh some windows more frequently or they may want some fancy window functionality.
-   Also this concept helps in back-buffering of RTs by giving swapchain texture handles to the user.


# RG Life-Cycle
-   *RG Execution* first calls *Dynamic RG Optimizer* to optimize control flow and synchronization. Then it starts [*recording command buffers* ],

# Static RG Optimizer
-   *Dynamic RG Optimizer* has to run every frame and instead of managing all passes, grouping some of them and optimizing access patterns to passes helps Dynamic RG Optimizer.
-   After RG is reconstructed, it is easy to see which passes're gonna run asynchronously and which ones have to run sequentially if RG has the maximum workload.
-   Passes that has to run sequential to each other create a *Branch*, that can't be divided into further asynchronous passes with any type of workload.
-   Branches also helps to identify which *GPU Queue*s can be used for branches, and how many *GPU Queues* can be used.


# Dynamic RG Optimizer
-   Dynamic RG Optimizer has lots of complicated scenarios and they are told in the implementation of the used GFX backend.
-   Comments're written similarly.
-   *A Depends on B*: B should be executed first. A should be executed after B finishes executing.
-   There are types of branches, check the GFX backend you use.


# Recording Command Buffers
-   Command buffer recording is


# Window Swapchain Management

-   GLFW is used for window management in OpenGL and Vulkan. 
-   GLFW stalls application while re-positioning and resizing windows (these operations called *WindowChange*), there is nothing TGFX can do about it.
-   TGFX gets device inputs and *WindowChange*s while executing Take_Inputs(). 
-   If multiple *WindowChange*s happen in a frame, Take_Inputs() will call all related callbacks of the windows.
-   You should manage swapchains yourself because only you know the next frame's RG.
