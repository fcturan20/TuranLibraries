# How-to-Use
RenderGraph is a graph, so there should be some nodes. These are called RenderNodes (Some times just Pass or Render Pass). There are 4 types of RenderNodes; Draw Pass, Compute Pass, Transfer Pass and Window Pass.
RenderGraph is essential for Vulkan because shader compilation requires the Subpass that uses the shader, so RenderGraph should be created before compiling-linking shaders. But RenderGraph is a collection of nodes that uses some global buffers and render targets, so render target textures (They can be changed later) and global buffers (They are also required for shader compilation) should be created before. That means your creation progress should be something like this; RT/Global Buffer Creations -> RenderGraph Construction -> Shader compiling/linking.
RenderGraph should cull nodes that doesn't have any operations. My solution is that if a nodes should be culled, other nodes that depends on this node should inherit the dependencies. Something like;
Node B waits for Node A's RT output and Node C waits for Node B's vertex shaders. If Node B doesn't have any operation to run, then Node C should wait for Node A's RT output.
This is a nice approach if you keep worst cases in mind and design around it. But if user thought that Node A and Node C operations will never overlap because of Node B (And this works on his device) and somehow forgot to give operations to Node B; then there maybe some problems. I want to allow users to define these dependencies in a different structure but can't right now.

# Draw Pass
1) This is where GPU Rasterization Pipeline is used. 
2) A Draw Pass is a collection of SubDrawPasses and there should be at least one SubDrawPass to create a Draw Pass. 
3) SubDrawPass is the same concept with Vulkan's SubPasses, but I believe mine's interface is better :)
4) In a SubDrawPass, each draw call is executed asynchronously. If you want a sync, you should create a new SubDrawPass for it. 
5) SubDrawPasses that don't have operations are culled according to the Node Culling process.
6) You can define how you will draw your vertex buffer in Draw Call (Indexed, Shader Instance, Point-Line-Triangle etc). This allows GFX API to sort the draw calls for minimum state change.
7) DrawPass draws vertex buffers to texture(s), which are called Render Target. But DrawPass needs a concept called RTSlotSet, which is described in GFX/Renderer/GPU_ContentManager.h.

# Compute Pass
1) This is where GPU Compute Pipeline is used.
2) I didn't have any asynchronous compute capable hardware to test the RenderGraph, so I was more in to other parts. Now I have a GTX1650, so I will code Compute Pass as well.

# Transfer Pass
1) This is where data transfers happen.
2) You have 4 choices; CPU->GPU Transfer Pass (Upload TP), GPU<->GPU Transfer Pass (Copy TP), GPU->CPU Transfer Pass (Download TP) and GPU Operation | GPU Operation Transfer Pass (Barrier TP)
3) TP is typeful. Because it's more data transfers close to each other in API, it's harder to debugging. Also GPU pipelines may differ between types of data transfers, which this seperation may help in future!
4) Upload TPs use staging buffer for temporary data. When you call Upload_**Buffer(), the data is uploaded to the staging buffer, then is copied to the permanent VRAM location.
5) For now, Staging buffers are only for data transfers. I want to allow user to divide the buffer for datas that is frequently changed.
6) Download TP is complicated concept which I didn't decide how to implement because of multi-threading (CPU job system) and dynamic GPU operations.
7) Barrier TP is useful for resource layout changes (image or buffer). For example, you need to change textures' layouts before using them first time. 

# Window Pass
1) This is where display functions are used.
2) This is a Pass, because users may want to refresh some windows more frequently or they may want some fancy window functionality.
3) Also this concept helps in back-buffering of RTs by giving swapchain texture handles to the user.