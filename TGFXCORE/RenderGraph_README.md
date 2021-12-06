# RenderGraph

-   RenderGraph is the way to define your GPU side operations. Modern GFX APIs needs proper synchronization between GPU-CPU and RenderGraph is the best system to define them. Every GPU operation (even resource creation, upload etc) needs to start running at the scheduled time for best performance and minimal bug. To achieve that, this system uses *RenderPass*es to abstract GPU operation types and define execution dependencies between them.

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


# How-to-Use
    RenderGraph is essential for Vulkan because shader compilation requires the Subpass that uses the shader, so RenderGraph should be created before compiling-linking shaders. But RenderGraph is a collection of nodes that uses some global buffers and render targets, so render target textures (They can be changed later) and global buffers (They are also required for shader compilation) should be created before. That means your creation progress should be something like this; RT/Global Buffer Creations -> RenderGraph Construction -> Shader compiling/linking.
RenderGraph should cull nodes that doesn't have any operations. My solution is that if a nodes should be culled, other nodes that depends on this node should inherit the dependencies. Something like;
Node B waits for Node A's RT output and Node C waits for Node B's vertex shaders. If Node B doesn't have any operation to run, then Node C should wait for Node A's RT output.
This is a nice approach if you keep worst cases in mind and design around it. But if user thought that Node A and Node C operations will never overlap because of Node B (And this works on his device) and somehow forgot to give operations to Node B; then there maybe some problems. I want to allow users to define these dependencies in a different structure but can't right now.


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
-   You have 4 choices; CPU->GPU Transfer Pass (Upload TP), GPU<->GPU Transfer Pass (Copy TP), GPU->CPU Transfer Pass (Download TP) and GPU Operation ||| GPU Operation Transfer Pass (Barrier TP)
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